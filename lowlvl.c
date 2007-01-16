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

#include <stdio.h>
#include <mem.h>
#include <alloc.h>
#include <dos.h>
#include <string.h>
#include <math.h>
#include "types.h"
#include "PCI_LIB.h"
#include "lowlvl.h"
#include "ataio.h"
#include "constant.h"
#include "standard.h"
#include "extended.h"
#include "ide.h"

int hasNonIDEController = -1;

static int trim(char *src, char *dest) {
   int startIndex;
   int endIndex;
   for (startIndex = 0; src[startIndex] == ' ' && startIndex < strlen(src); startIndex++);
   for (endIndex = strlen(src)-1; src[endIndex] == ' ' && endIndex >= startIndex; endIndex--);
   strncpy(dest, src+startIndex, endIndex-startIndex+1);
   dest[endIndex-startIndex+1] = '\0';
   return strlen(dest);
}

char *getascii (unsigned short in_data [], unsigned short off_start, unsigned short off_end)
{
  static char ret_val [255];
  int loop, loop1;

  for (loop = off_start, loop1 = 0; loop <= off_end; loop++)
    {
      ret_val [loop1++] = (char) (in_data [loop] / 256);  /* Get High byte */
      ret_val [loop1++] = (char) (in_data [loop] % 256);  /* Get Low byte */
    }
  ret_val [loop1] = '\0';  /* Make sure it ends in a NULL character */
  return (ret_val);
}

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

   for (bus = firstbus ; bus <= (maxbus) ; bus++, firstdev = 0, firstfunc = 0)
      for (device = firstdev ; device < 32 ; device++, firstfunc = 0)
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
         ide_channels[found_ide_devices].bmcr = (WORD)cfg.nonbridge.base_address4 & 0xfffe; // 4 LSBs must be zero.
         ide_channels[found_ide_devices].bmaddr = ((WORD)cfg.nonbridge.base_address4 & 0xfffe) + 2;
         if ((cfg.progIF) & 0x01) {
            // Primary channel is operating in native-PCI mode
            //printf("Primary channel is operating in 'native PCI' mode\n");
            ide_channels[found_ide_devices].command_reg = (WORD)cfg.nonbridge.base_address0 & 0xfffe;
            ide_channels[found_ide_devices].control_reg = ((WORD)cfg.nonbridge.base_address1 & 0xfffe) - 4;
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
         ide_channels[found_ide_devices].bmcr = ((WORD)cfg.nonbridge.base_address4 & 0xfffe) + 8; // 4 LSBs must be zero.
         ide_channels[found_ide_devices].bmaddr = ((WORD)cfg.nonbridge.base_address4 & 0xfffe) + 10;
         if ((cfg.progIF) & 0x04) {
            // Secondary channel is operating in native PCI mode
            //printf("Secondary channel is operating in 'native PCI' mode.\n");
            ide_channels[found_ide_devices].command_reg = (WORD)cfg.nonbridge.base_address2 & 0xfffe;
            ide_channels[found_ide_devices].control_reg = ((WORD)cfg.nonbridge.base_address3 & 0xfffe) - 4;
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
         // Found some other disk controller, ignore it.
      }
   }
   *ideChannelsFound = found_ide_devices;
   return(found_devices);
}

int HasNonIDEController() {
   PCIcfg *cfg = farmalloc(sizeof(PCIcfg));
   int i, cmb, bus, device, func;
   byte hi;
   int maxbus;

   if (!pcibios_present()) {
      // If we can't query the PCI bus, assume there is some other controller there.
      hasNonIDEController = 1;
   } else if (hasNonIDEController < 0) {
      hasNonIDEController = 0;
      maxbus = pcibios_present();
      for (bus = 0 ; bus <= maxbus ; bus++)
         for (device = 0 ; device < 32; device++)
            for (func = 0 ; func < 8; func++) {
               cmb = (device<<3) | (func & 0x07);
               for (i = 0 ; i < 70 ; i++)
               {
                  if (read_configuration_byte(bus,cmb,i,&hi) != SUCCESSFUL) {
                     free(cfg);
                     return -1;
                  }

                  ((byte*)cfg)[i] = hi ;
               }
               if ((cfg->vendorID != 0xFFFF) && (cfg->classcode == 0x01) &&
                  (cfg->subclass != 0x01 && cfg->subclass != 0x02))
                     hasNonIDEController = 1;
         }

   }
   free(cfg);
   return hasNonIDEController;
}

int bios_detect(Drive* drives, int startId, int maxCount, int offset) {
   int i, rc;
   Drive d;
   int foundcount = 0;

   int device = 0;

   ATA_IDENTIFY_DEVICE_Info *ideInfo = farmalloc(sizeof(ATA_IDENTIFY_DEVICE_Info));
   ExtDiskParam edp;
   StdDiskParam sdp;
   ExtensionInfo extInfo;

   // Check each location from startId up to maxCount (e.g. 0x80 to 0x87)
   for (i = 0; i < maxCount; i++) {
      memset(&d, 0, sizeof(Drive));
      d.diskId = startId + i;

      if (d.diskId < 0x80) {
         // It's a floppy -- call StandardGetDiskInfo on it.
         rc = StandardGetDiskInfo(d.diskId, &d.stdParams);
         if (rc) continue; // StdGetDiskInfo errored there isn't a disk there.
         // If params are 0, drive is configured in BIOS but isn't actually there.
         if (d.stdParams.cylinders == 0 &&
             d.stdParams.heads == 0 &&
             d.stdParams.spt == 0) continue;

         d.type = DISK_TYPE_FLOPPY;
         d.accessMode = ACCESS_MODE_STANDARD;
         d.bps = STD_BYTES_PER_SECTOR;
      } else if (d.diskId >= 0x80) {
         // It's a hard disk.
         rc = StandardGetDiskInfo(d.diskId, &sdp);
         if (rc) continue; // No disk there.
         memcpy(&d.stdParams, &sdp, sizeof(sdp));
         rc = IsINT13ExtensionInstalled(&d, &extInfo);

         // Check whether int13x is installed, and we can get the int13x info.
         if (rc == 0 && extInfo.extensioninstalled>0 && (rc = ExtendedGetDiskInfo(&d, &edp)) == 0) {
            //printf("\nDisk %02x supports int13ex\n", d.diskId); getch();
            // Is it master or slave??
            device = ((*((WORD *)edp.edd + 2) & 0x10) > 0);
            d.accessMode = ACCESS_MODE_EXTENDED; // INT13extensions supported, we can use extended access
            memcpy(&d.extInfo, &extInfo, sizeof(extInfo));
            memcpy(&d.extParams, &edp, sizeof(edp));

            if ((d.extInfo.apisupportbitmap & 0x01) && d.extParams.bytespersector > 0) {
               d.bps = d.extParams.bytespersector;
            } else {
               d.bps = STD_BYTES_PER_SECTOR;
            }
            /*printf("int13x major version: %02Xh\n", d.extInfo.majorversion);
            printf("reported bytes per sector: %d\n", d.extParams.bytespersector);
            printf("api support bitmap: %02Xh\n", d.extInfo.apisupportbitmap);
            printf("edp info flags: %04X\n", d.extParams.info);
            printf("reported max chs: %lu %lu %lu\n", d.extParams.cyls, d.extParams.heads, d.extParams.sectorstrack);
            printf("base address in EDP: %04X\n", d.extParams.edd);
            getch();*/
            if (extInfo.majorversion >= 0x20) {
               // If INT13X version is at least 2, then we can get the base address to try and
               // do an ATA IDENTIFY DEVICE.
               rc = get_IDE_info(*(WORD *)edp.edd, device, ideInfo);
               //printf("get_IDE_info returned: %d\n", rc); getch();
               if (rc != 0) {
                  // ATA IDENTIFY errored. It must be a SCSI disk.
                  d.type = DISK_TYPE_SCSI;
               } else {
                  // Get the ATA IDENTIFY info and save it.
                  d.type = DISK_TYPE_IDE;
                  trim(ideInfo->model, d.ide.model);
                  trim(ideInfo->serialno, d.ide.serial_no);
                  trim(ideInfo->firmware, d.ide.firmware);
                  d.ide.supports_lba48 = ((ideInfo->fe_2 & 0x0400) > 0)? 1 : 0;
                  d.ide.device = device;
                  if (d.ide.supports_lba48 != 0) {
                     d.queriedConfig.totalSectorsHi = ideInfo->lba48_totalsectors[1];
                     d.queriedConfig.totalSectorsLo = ideInfo->lba48_totalsectors[0];
                  } else {
                     d.queriedConfig.totalSectorsHi = 0;
                     d.queriedConfig.totalSectorsLo = ideInfo->totalsectors;
                  }
                  d.queriedConfig.Heads = d.realConfig.Heads = ideInfo->log_heads + 1;
                  d.queriedConfig.Sectors = d.realConfig.Sectors = ideInfo->log_sectors;
               }
            } else {
               // Only INT13X version 1 is supported. We can't get the base address to do
               // an IDENTIFY DEVICE command, and we don't have any other information about
               // how the disk is connected. Therefore, we cannot tell whether it's
               // a SCSI or an IDE disk.
               // However, we can still use Extended LBA mode.
               d.type = DISK_TYPE_UNKNOWN_HDD;
            }

         } else {
            // INT13 extensions are not supported at all by this disk, so we'll use
            // CHS mode.
            //printf("\nDisk %02x doesn't support int13ex\n", d.diskId); getch();

            d.type = DISK_TYPE_UNKNOWN_HDD;
            d.accessMode = ACCESS_MODE_STANDARD;
            d.bps = STD_BYTES_PER_SECTOR;
         }
         //printf(" %.2f MB capacity\n", ((double)d.lbalo) / 2048.0f);
      }
      memcpy(&drives[foundcount+offset], &d, sizeof(Drive));
      foundcount++;
   }
   free(ideInfo);
   return foundcount;
}


int get_IDE_info(unsigned int base, int priSec, ATA_IDENTIFY_DEVICE_Info *buf)
{
   unsigned long  wait_loop;  /* Timeout loop */
   unsigned short in_val;
   unsigned short bigbuf[256];
   unsigned short dd_off;     /* DiskData offset */

      /* Wait for controller not busy */
      wait_loop = 100000L;
      while (--wait_loop > 0)
        {
        in_val = inp (base + 7);
        if (((in_val & 0x40) == 0x40) || ((in_val & 0x00) == 0x00))
           break;
        }

      if (wait_loop < 1)
        return(-1);

      outp (base + 6, (priSec == 0 ? 0xA0 : 0xB0)); /* Get Master/Slave drive */
      delay(10);
      outp (base + 7, 0xEC);          /* Get drive info data */
      delay(10);
      /* Wait for data ready */
      wait_loop = 1000000L;
      while (--wait_loop > 0)
        {
        in_val = inp (base + 7);
        if ((in_val & 0x48) == 0x48) /* drive ready and needs service */
          break;
        if ((in_val & 0x01) == 0x01) /* drive error */
          break;
        }

      if ((wait_loop < 1) || ((in_val & 0x01) == 0x01)) /* Timed Out or Error */
        return(1);

      for (dd_off = 0; dd_off != 256; dd_off++) /* Read "sector" */
        bigbuf[dd_off] = inpw (base);

        memcpy(buf, bigbuf, 256); // the bigbuf is not quite the full buf-but it will do
        strcpy(buf->serialno, getascii(bigbuf, 10, 19));
        strcpy(buf->firmware, getascii(bigbuf, 23, 26));
        strcpy(buf->model, getascii(bigbuf, 27, 46));
        buf->firmware[7] = 0x0;
        //if (buf->gen_config & 0x8000)  // is it an atapi drive (CD)
        //   return(-2);
   return(0);
}


