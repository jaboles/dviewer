#include <mem.h>
#include <string.h>
#include "utility.h"
#include "globals.h"
#include "pci_lib.h"
#include "lowlvl.h"
#include "ide.h"
#include "constant.h"

// ============================================================================
// Name: LBAFromCHS
// Description: Calculate LBAAddress Given CHS parameters
// Inputs:		WORD c			Cylinder
//				WORD h			Head
//				WORD s	        Sector
//				WORD hpc		Heads Per Cylinder
//				WORD spt		Sectors Per Track
// Returns: DWORD - LBA Translated Address
// ============================================================================

DWORD LBAFromCHS(DWORD c, DWORD h, DWORD s, DWORD hpc, DWORD spt)
{
    DWORD LBAAddress = 0;

    LBAAddress = s - 1;
    LBAAddress += (spt * (h + (c * hpc)));
    return(LBAAddress);
}

// ============================================================================
// Name: LBAFromCHS
// Description: Calculate CHS parameters given LBA Address.
// Inputs:      DWORD LBA       LBA Address
//	            WORD *Cyl       Value of Cylinder from LBA
//	            WORD *Head      Value of Head from LBA
//	            WORD *Cyl       Value of Sector from LBA
// Returns: None
// ============================================================================

void SetCHSFromLBA(DWORD LBA, WORD *Cyl, WORD *Head, WORD *Sec, WORD hpc, WORD spt)
{
   *Sec = (WORD)((LBA % spt) + 1);
   *Head = (WORD)((LBA / spt) % hpc);
   *Cyl = (WORD)((LBA / spt) / hpc);
}

// ============================================================================
// Name: prep
// Description: Take an Integer Value and Break into desired components. Used
//					 primarily in decoding CHS values from INT13/08h Call
// Inputs:		WORD val			Word Value
//					int  mode		Decode Type
//										0 for Sector Decoding
//										1 is Lo-Value of Cylinder
//										2 is Hi-Value of Cylinder
// Returns: BYTE - Value
// ============================================================================

BYTE prep(WORD val, int mode)
{
   struct bytes
   {
      unsigned int lo : 6;
      unsigned int hi : 2;
   };
   struct bitints
   {
      unsigned lo : 8;
      unsigned hi : 2;
      unsigned ig : 6;
   };
   struct words
   {
      WORD val;
   };
   union breakit
   {
      struct words  i;
      struct bitints bi;
      struct bytes b;
   };

   union breakit v;
   v.i.val = val;
   if (mode == 0)
      return(v.b.lo);
   if (mode == 1)
      return(v.bi.lo);
   if (mode == 2)
      return(v.bi.hi);
   if (mode == 3)
      return(v.b.hi);
   return(0);
}

// ============================================================================
// Name: EnumerateHardDisks
// Description: Setup Global Buffers with all Hard Disks Information in System
// Returns:     WORD - 0 if save was successful
//                     other if error
// ============================================================================

int EnumerateHardDisks(int forceCHS)
{
    // We need to check the number of drives on the system. But to check that
    // we need to check the Parameters for the first drive. Query using Standard
    // means the parameter information for the First Disk.
    int i;
    int ideChannelsFound = 0;

    if (pcibios_present() && scan_devices(ideChannels, &ideChannelsFound) >= 0) {
       // Do ATA IDE detect
       //printf("%d IDE channels were found.\n", ideChannelsFound); getch();
       NumberOfIDEDrives = detect_ide_drives(ideChannels, ideChannelsFound, ideDriveList, 0);
    } else {
       //printf("Scan for IDE controllers returned: %d\n", ret); getch();
       NumberOfIDEDrives = 0;
    }

    // Do BIOS detect
    NumberOfBIOSDrives = bios_detect(biosDriveList, 0x80, 8, 0);
    NumberOfFloppyDrives = bios_detect(floppyDriveList, 0x00, 2, 0);

    // If CHS mode is forced on, set the appropriate access mdoe.
    for (i = 0; i < NumberOfBIOSDrives; i++) {
        if (forceCHS) {
            biosDriveList[i].accessMode = ACCESS_MODE_STANDARD;
        }
    }
    return(0);
}

int PrepareWipeList() {
   int i, driveCount, j;
   driveCount = 0;
   if (HasNonIDEController()) {
      // Add all the floppy drives to the main list.
      for (i = 0; i < NumberOfFloppyDrives; i++) {
         memcpy(&driveList[driveCount], &floppyDriveList[i], sizeof(Drive));
         driveCount++;
      }
      // Add all the drives appearing in the IDE list to the main list.
      for ( i = 0; i < NumberOfIDEDrives; i++) {
         memcpy(&driveList[driveCount], &ideDriveList[i], sizeof(Drive));
         //printf("IDE Drive Model: %s\n", ideDriveList[i].ide.model);
         //printf("IDE Drive Serial: %s\n", ideDriveList[i].ide.serial_no);
         driveCount++;
      }
      // Add any BIOS list drive that doesn't also appear in the IDE list to the main list.
      for (i = 0; i < NumberOfBIOSDrives; i++) {
         for (j = 0; j < NumberOfIDEDrives; j++) {
            //printf("comparing |%s| with |%s|\n", biosDriveList[i].ide.model, ideDriveList[j].ide.model);
            //getch();
            if (biosDriveList[i].type == DISK_TYPE_IDE
                  && strcmp(biosDriveList[i].ide.model, ideDriveList[j].ide.model) == 0) {
               break;
            }
         }
         // If we fully iterated thru the drive list and got to the end, we didn't find this bios
         // drive in the IDE list. So treat it as another drive.
         if (j == NumberOfIDEDrives) {
            memcpy(&driveList[driveCount], &biosDriveList[i], sizeof(Drive));
            driveCount++;
         }
      }
   } else {
      //printf("\nthere are no non-ide controllers. %d ide and %d bios.\n", NumberOfIDEDrives,NumberOfBIOSDrives); getch();
      // Add all the floppy drives to the main list.
      for (i = 0; i < NumberOfFloppyDrives; i++) {
         memcpy(&driveList[driveCount], &floppyDriveList[i], sizeof(Drive));
         driveCount++;
      }
      if (NumberOfIDEDrives >= NumberOfBIOSDrives) {
         // There are only IDE drives in the system, and the IDE list
         // contains more than the BIOS list. Use the IDE list.
         for (i = 0; i < NumberOfIDEDrives; i++) {
            memcpy(&driveList[driveCount], &ideDriveList[i], sizeof(Drive));
            driveCount++;
         }
      } else {
         // There are only IDE drives in the system, and the BIOS list
         // contains more than the IDE list. Use the BIOS list.
         for (i = 0; i < NumberOfBIOSDrives; i++) {
            memcpy(&driveList[driveCount], &biosDriveList[i], sizeof(Drive));
            driveCount++;
         }
      }
   }
   //getch();
   return driveCount;
}
