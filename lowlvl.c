/***********************************************************************************/
/* DESTROY                                                                         */
/* Copyright (C) 1999-2002 by The Australian Software Company Pty Ltd              */
/* All rights reserved.                                                            */
/* This source code is proprietary to The Australian Software Company and must     */
/* not be distributed to any unauthorised person or organisation.                  */
/*                                                                                 */
/* January - July 2002 - Developed by David Quail & Scott Tierney                  */
/* Version 2.01                                                                    */
/*                                                                                 */
/***********************************************************************************/
#pragma warn -pin

#include <stdio.h>
#include <mem.h>
#include "types.h"
#include "PCI_LIB.h"
#include "lowlvl.h"
#include <dos.h>
#include <string.h>
#include "standard.h"
#include "constants.h"
#include "ide.h"
#include "utility.h"
#include "util.h"

int find_disk(int far *bus_st, int far *dev_st, int far *func_st, PCIcfg far *cfg)
{
   int i;
   int bus, firstbus;
   int device, firstdev;
   int func, firstfunc;
   int cmb;
   byte hi;
   int maxbus;

   maxbus = pcibios_present();
   if (maxbus == -1)
      return(-1);

   if (bus_st == NULL)
      firstbus = 0; else firstbus = *bus_st;
   if (dev_st == NULL)
      firstdev = 0; else firstdev = *dev_st;
   if (func_st == NULL)
      firstfunc = 0; else firstfunc = *func_st;

   for (bus = firstbus ; bus <= maxbus ; bus++, device = 0, func = 0)
      for (device = firstdev ; device < 32 ; device++, func = 0)
         for (func = firstfunc ; func < 8 ; func++)
         {
            cmb = (device<<3) | (func & 0x07);
            for (i = 0 ; i < 70 ; i++)
            {
               if (read_configuration_byte(bus,cmb,i,&hi) != SUCCESSFUL)
                  return -2;
               ((byte*)cfg)[i] = hi ;
            }
            if ((cfg->vendorID != 0xFFFF) && (cfg->classcode == 0x01))
            {
               *bus_st = bus;
               *dev_st = device;
               *func_st = func;

               return(cfg->subclass);
            }
         }
   return(-1);
}


// looks at each PCI device, specifically looking for the storage devices
// if it finds one or more then sets the device array which must be
// an array of 6 integers

int scan_devices(IDEChannel far *ide_channels, int far *ideChannelsFound)
{
   int bus = 0;
   int device = 0;
   int func = 0;
   int diskcontroller_subclass = 0;
   int found_devices = 0;
   int found_ide_devices = 0;
   PCIcfg cfg;

   if (ide_channels == NULL)
      return(-1);

   while (diskcontroller_subclass >= 0)
   {
      memset(&cfg, 0, sizeof(cfg));
      diskcontroller_subclass = find_disk(&bus, &device, &func, &cfg);
      func++;
      if (diskcontroller_subclass > 4) {
         printf("Unknown disk controller type found! (subclass = %d)\n", diskcontroller_subclass);
      } else if (diskcontroller_subclass < 0) {
         // We got to the end of the list, ignore it.
      } else if (diskcontroller_subclass == 1) {
         // Found an IDE controller.
         //printf("Found ide controller. IRQ from PCI CFG: %d\n", cfg.nonbridge.interrupt_line);
         // Store the primary channel's info
         ide_channels[found_ide_devices].pci_bus = bus;
         ide_channels[found_ide_devices].pci_device = device;
         ide_channels[found_ide_devices].pci_function = func;
         ide_channels[found_ide_devices].bmcr = cfg.nonbridge.base_address4 & 0xfffe; // 4 LSBs must be zero.
         ide_channels[found_ide_devices].bmaddr = (cfg.nonbridge.base_address4 & 0xfffe) + 2;
         if ((cfg.progIF) & 0x01) {
            // Primary channel is operating in native-PCI mode
            //printf("Primary channel is operating in 'native PCI' mode\n");
            ide_channels[found_ide_devices].command_reg = cfg.nonbridge.base_address0 & 0xfffe;
            ide_channels[found_ide_devices].control_reg = (cfg.nonbridge.base_address1 & 0xfffe) - 4;
            ide_channels[found_ide_devices].irq = cfg.nonbridge.interrupt_line;
         } else {
            // Primary channel is operating in compatibility mode
            //printf("Primary channel is operating in 'compatibility' mode\n");
            ide_channels[found_ide_devices].command_reg = 0x1F0;
            ide_channels[found_ide_devices].control_reg = 0x3F0;
            ide_channels[found_ide_devices].irq = 14;
         }
         found_ide_devices++;
         found_devices++;

         // Store the secondary channel's info.
         ide_channels[found_ide_devices].pci_bus = bus;
         ide_channels[found_ide_devices].pci_device = device;
         ide_channels[found_ide_devices].pci_function = func;
         ide_channels[found_ide_devices].bmcr = (cfg.nonbridge.base_address4 & 0xfffe) + 8; // 4 LSBs must be zero.
         ide_channels[found_ide_devices].bmaddr = (cfg.nonbridge.base_address4 & 0xfffe) + 10;
         if ((cfg.progIF) & 0x04) {
            // Secondary channel is operating in native PCI mode
            //printf("Secondary channel is operating in 'native PCI' mode.\n");
            ide_channels[found_ide_devices].command_reg = cfg.nonbridge.base_address2 & 0xfffe;
            ide_channels[found_ide_devices].control_reg = (cfg.nonbridge.base_address3 & 0xfffe) - 4;
            ide_channels[found_ide_devices].irq = cfg.nonbridge.interrupt_line;
         } else {
            // Secondary channel is operating in compatibility mode
            //printf("Secondary channel is operating in 'compatibility' mode\n");
            ide_channels[found_ide_devices].command_reg = 0x170;
            ide_channels[found_ide_devices].control_reg = 0x370;
            ide_channels[found_ide_devices].irq = 15;
        }
         found_ide_devices++;
         found_devices++;
      } else {
         printf("found some other disk controller, ignoring it. subclass=%d\n", diskcontroller_subclass);
      }
   }
   *ideChannelsFound = found_ide_devices;
   return(found_devices);
}

int bios_detect(Drive* drives, int startId, int maxCount, int offset) {
   int i, rc;
   int diskId;
   Drive d;
   int foundcount = 0;

   for (i = 0; i < maxCount; i++) {
      diskId = startId + i;
      rc = StandardGetDiskInfo(diskId, &d);

      // If return code nonzero, there's no drive there.
      if (rc) continue;

      // If params are 0, drive is configured in BIOS but isn't actually there.
      if (d.cylinders == 0 &&
          d.heads == 0 &&
          d.spt == 0) continue;

      // We found a valid drive. Save the data.
      if (diskId <= 0x01) {
         d.type = DISK_TYPE_FLOPPY;
         d.bps = 512;
         d.cylinders++; // Returns highest cylinder number, but we want # of cylinders.
         d.heads++; // Returns highest head number, but we want # of heads.
         d.lbahi = 0;
         d.lbalo = d.cylinders * d.heads * d.spt;
      } else if (diskId >= 0x80 && startId+i <= 0x87) {
         printf("bios_detect() found a hard disk -- don't know what to do with it yet!\n");
         exit(1);
      } else {
         printf("bios_detect() found an unknown disk -- don't know what to do!\n");
         exit(1);
      }
      d.diskId = diskId;
      memcpy(&drives[foundcount+offset], &d, sizeof(Drive));
      foundcount++;
   }
   return foundcount;
}

void activate_drive(Drive *drive) {
   if (drive->type == DISK_TYPE_FLOPPY) {
      // Uses bios calls, so no activation is necessary.
   } else if (drive->type == DISK_TYPE_IDE) {
      activate_ide_drive(drive);
   } else if (drive->type == DISK_TYPE_SCSI) {
      printf("Called activate() on a scsi disk -- don't know what to do yet!\n");
      exit(1);
   } else {
      printf("Cannot activate() -- disk type is unknown!\n");
      exit(1);
   }
}

int do_read(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far *bufferPtr) {
   int rc = 0;
   int c = 0, h = 0, s = 0;
   int sectorsToRead = 0;
   int sectorsRead = 0;
   int sectorsRemainingInCylinder = 0;
   int i;
   
   if (drive->type == DISK_TYPE_FLOPPY) {
      //printf("\n lba:%lu c:%d h:%d s:%d  \n", lbalo, c, h, s);
      while (sectorsRead < sc) {
         SetCHSFromLBA(lbalo + sectorsRead, &c, &h, &s, drive->heads, drive->spt);
         sectorsRemainingInCylinder = ((drive->heads - h) * drive->spt) - (s - 1);
         sectorsToRead = MIN(sc, sectorsRemainingInCylinder);
         //printf("Sectors remaining in cyl: %d  sectors to read: %d\n", sectorsRemainingInCylinder, sectorsToRead);
         //getch();

         rc = StandardRead(drive->diskId, c, h, s, sectorsToRead, bufferPtr + (sectorsRead * 512));
         if (rc) return rc;
         sectorsRead += sectorsToRead;
      }
      return 0;
   } else if (drive->type == DISK_TYPE_IDE) {
      return do_ide_read(drive, lbahi, lbalo, sc, bufferPtr, 1);
   } else {
      printf("Unknown drive type -- don't know how to read from it!\n");
      exit(1);
   }
}

/*int kill_IDE_max(int drive, ATA_IDENTIFY_DEVICE_Info *buf)
{
   unsigned long  wait_loop;  // Timeout loop
   unsigned short base;       // Base address of drive controller
   unsigned short in_val;
   //unsigned short bigbuf[256];
   //unsigned short dd_off;     // DiskData offset
   //int sc, sn, cl, ch, dh;
   int sn, cl, ch, dh;
   unsigned long disksz;
   unsigned char lba48[8];

   {
      // Get IDE Drive info

      switch (drive / 2)
        {
        case 0: base = 0x1f0;
                break;
        case 1: base = 0x170;
                break;
        case 2: base = 0x1e8;
                break;
        case 3: base = 0x168;
                break;
        }

      // Wait for controller not busy
      wait_loop = 100000L;
      while (--wait_loop > 0)
        {
        in_val = inp (base + 7);
        if (((in_val & 0x40) == 0x40) || ((in_val & 0x00) == 0x00))
           break;
        }

      if (wait_loop < 1)
        return(-2);

//      outp (base + 6, ((drive % 2) == 0 ? 0xA0 : 0xB0)); // Get Master/Slave drive
//      delay(1);
      outp (base + 7, 0xF8);          // Get Native size
      delay(1);
      // Wait for data ready
      wait_loop = 100000L;
      while (--wait_loop > 0)
        {
        in_val = inp (base + 7);
        if ((in_val & 0x48) == 0x48) // drive ready and needs service
          break;
        if ((in_val & 0x01) == 0x01) // drive error
          break;
        }

      if ((wait_loop < 1) || ((in_val & 0x01) == 0x01)) // Timed Out or Error
        return(-1);

      //sc = pio_inbyte( base + 2 );
      sn = pio_inbyte( base + 3 );
      cl = pio_inbyte( base + 4 );
      ch = pio_inbyte( base + 5 );
      dh = pio_inbyte( base + 6 );
      disksz = dh & 0x0f;
      disksz = disksz << 8;
      disksz = disksz | ch;
      disksz = disksz << 8;
      disksz = disksz | cl;
      disksz = disksz << 8;
      disksz = disksz | sn;

      // Wait for controller not busy
      wait_loop = 100000L;
      while (--wait_loop > 0)
        {
        in_val = inp (base + 7);
        if (((in_val & 0x40) == 0x40) || ((in_val & 0x00) == 0x00))
           break;
        }

      if (wait_loop < 1)
        return(-1);

      // in ATA LBA28 mode
     * (unsigned long *) ( lba48 + 0 ) = disksz;

      pio_outbyte( base + 1, 0 );
      pio_outbyte( base + 2, 0 );
      pio_outbyte( base + 3, lba48[0] );
      pio_outbyte( base + 4, lba48[1] );
      pio_outbyte( base + 5, lba48[2] );


      outp (base + 6, ((drive % 2) == 0 ? 0xA0 : 0xB0)); // Get Master/Slave drive
      delay(1);
      outp (base + 7, 0xF9);          // Get Native size
      delay(1);
      // Wait for data ready
      wait_loop = 100000L;
      while (--wait_loop > 0)
        {
        in_val = inp (base + 7);
        if ((in_val & 0x48) == 0x48) //drive ready and needs service
          break;
        if ((in_val & 0x01) == 0x01) // drive error
          break;
        }

      if ((wait_loop < 1) || ((in_val & 0x01) == 0x01)) // Timed Out or Error
        return(-1);
   }
   return(0);
}*/

