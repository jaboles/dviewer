#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "types.h"
#include "ide.h"
#include "ataio.h"
#include "constants.h"

void activate_ide_drive(Drive *drive) {
   pio_set_iobase_addr(drive->ide.command_reg, drive->ide.control_reg, drive->ide.bmcr);
   reg_config();
   reg_reset(0, drive->ide.device);
   attempt_max_multi_mode(drive->ide.device);
   int_enable_irq(0, drive->ide.irq, drive->ide.bmaddr, drive->ide.command_reg + 7);
   dma_pci_config(drive->ide.bmcr);
}

int detect_ide_drives(IDEChannel far *ic, int ide_channel_count, Drive far *ide_drives, int driveInfoOffset) {
   int i, foundcount = 0;

   for (i = 0; i < ide_channel_count; i++) {
      //printf( "\nUsing I/O base addresses %04X and %04X with bmcr %04x and irq %d.\n",
      //     ic[i].command_reg, ic[i].control_reg, ic[i].bmcr, ic[i].irq );

      // Set up the driver to use this IDE channel
      pio_set_iobase_addr(ic[i].command_reg, ic[i].control_reg, ic[i].bmcr);
      reg_config();

      if (reg_config_info[0] == 2) {
         if (populate_drive_info(0, &ide_drives[foundcount+driveInfoOffset]) == 0) {
            //printf("Found %s, master on controller %d.\n", ide_drives[foundcount].model, i);
            ide_drives[foundcount+driveInfoOffset].ide.command_reg = ic[i].command_reg;
            ide_drives[foundcount+driveInfoOffset].ide.control_reg = ic[i].control_reg;
            ide_drives[foundcount+driveInfoOffset].ide.bmcr = ic[i].bmcr;
            ide_drives[foundcount+driveInfoOffset].ide.bmaddr = ic[i].bmaddr;
            ide_drives[foundcount+driveInfoOffset].ide.irq = ic[i].irq;
            foundcount++;
         }
      }
      if (reg_config_info[1] == 2) {
         if (populate_drive_info(1, &ide_drives[foundcount+driveInfoOffset]) == 0) {
            //printf("Found %s, slave on controller %d.\n", ide_drives[foundcount].model, i);
            ide_drives[foundcount+driveInfoOffset].ide.command_reg = ic[i].command_reg;
            ide_drives[foundcount+driveInfoOffset].ide.control_reg = ic[i].control_reg;
            ide_drives[foundcount+driveInfoOffset].ide.bmcr = ic[i].bmcr;
            ide_drives[foundcount+driveInfoOffset].ide.bmaddr = ic[i].bmaddr;
            ide_drives[foundcount+driveInfoOffset].ide.irq = ic[i].irq;
            foundcount++;
         }
      }
   }
   //printf("Found %d drives in total.\n", foundcount);
   return foundcount;
}

int populate_drive_info(int dev, Drive *d) {
   char buffer[512];
   ATA_IDENTIFY_DEVICE_Info rawInfo;
   int rc;

   memset(buffer, 0, 512);
   rc = reg_pio_data_in_lba28(
               dev, CMD_IDENTIFY_DEVICE,
               0, 0,
               0L,
               FP_SEG( buffer ), FP_OFF( buffer ),
               1, 0
               );
   if (rc) return rc;

   // Put the buffer into a raw ATA IDENTIFY DEVICE struct.
   memcpy(&rawInfo, &buffer, sizeof(ATA_IDENTIFY_DEVICE_Info));

   // Copy out the string data.
   swab(rawInfo.serialno, d->ide.serial_no, 20);
   swab(rawInfo.firmware, d->ide.firmware, 8);
   swab(rawInfo.model, d->ide.model, 40);
   d->ide.serial_no[20] = 0x0;
   d->ide.firmware[8] = 0x0;
   d->ide.model[40] = 0x0;

   // Copy out the other stuff.
   d->type = DISK_TYPE_IDE;
   d->bps = 512;
   d->ide.device = dev;
   d->ide.supports_lba48 = ((rawInfo.fe_2 & 0x0400) > 0)? 1 : 0;
   if (d->ide.supports_lba48) {
      d->lbahi = rawInfo.lba48_totalsectors[1];
      d->lbalo = rawInfo.lba48_totalsectors[0];
   } else {
      d->lbahi = 0;
      d->lbalo = rawInfo.capsectors;
   }
   d->ide.max_udma_mode = max_supported_UDMA_mode(rawInfo.udma);
   d->ide.selected_udma_mode = selected_UDMA_mode(rawInfo.udma);

   return 0; // All finished ok.
}

int attempt_max_multi_mode(int dev) {
   int rc;
   // Try setting block size of 32
   rc = reg_non_data_lba28( dev, CMD_SET_MULTIPLE_MODE, 0, 32, 0L );
   // If failed, try 16.
   if (rc > 0) {
      //printf("   ...failed, trying 16.\n");
      rc = reg_non_data_lba28( dev, CMD_SET_MULTIPLE_MODE, 0, 16, 0L );
      // If failed again, try 8.
      if (rc > 0) {
         //printf("   ...failed again, trying 8.\n");
         rc = reg_non_data_lba28( dev, CMD_SET_MULTIPLE_MODE, 0, 8, 0L );
         if (rc > 0) {
            //printf("   ...failed yet again. I give up :(\n");
            return(-1);
         } else {
            return 8;
         }
      } else {
         return 16;
      }
   } else {
      return 32;
   }
}

int do_ide_read(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   long totalSectorsToRead = sc;
   long sectorsToRead = 0;
   int lbaOffset = 0;
   unsigned long lbaStartHi;
   unsigned long lbaStartLo;
   int rc;

   while (totalSectorsToRead > 0) {
      sectorsToRead = totalSectorsToRead>256? 256 : totalSectorsToRead;
      lbaOffset = sc - totalSectorsToRead;

      lbaStartHi = lbahi;                           //
      lbaStartLo = lbalo;                           //  i.e. lbaStart = lba + lbaOffset
      add_lba(&lbaStartHi, &lbaStartLo, lbaOffset); //
      if (attemptDMA > 0) {
         if (drive->ide.supports_lba48 > 0) {
            rc = dma_pci_lba48(drive->ide.device, CMD_READ_DMA_EXT,
                           0, (int)sectorsToRead,
                           lbaStartHi, lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*512)), FP_OFF(bufferPtr+(lbaOffset*512)),
                           sectorsToRead);
         } else {
            rc = dma_pci_lba28(drive->ide.device, CMD_READ_DMA,
                           0, (int)sectorsToRead,
                           lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*512)), FP_OFF(bufferPtr+(lbaOffset*512)),
                           sectorsToRead);
         }
      }
      if (attemptDMA <= 0 || rc > 0) {
         //if (rc > 0) printf(" DMA %lu,%lu had error ", lbaStartHi, lbaStartLo);
         if (drive->ide.supports_lba48) {
            rc = reg_pio_data_in_lba48(drive->ide.device, CMD_READ_SECTORS_EXT,
                           0, (int)sectorsToRead,
                           lbaStartHi, lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*512)), FP_OFF(bufferPtr+(lbaOffset*512)),
                           sectorsToRead, 0);
         } else {
            rc = reg_pio_data_in_lba28(drive->ide.device, CMD_READ_SECTORS,
                           0, (int)sectorsToRead,
                           lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*512)), FP_OFF(bufferPtr+(lbaOffset*512)),
                           sectorsToRead, 0);
         }
      }

      /*if (rc >0) {
         printf(" PIO %lu,%lu had error\n", lbaStartHi, lbaStartLo);
      } else {
         printf("read ok\n");
      }*/

      totalSectorsToRead -= sectorsToRead;
   }

   return 0;
}

void add_lba(unsigned long *hi, unsigned long *lo, long amount) {
   if (amount > 0) {
      // if there is going to be overflow, then increment the high word.
      if (*lo + amount < *lo) {
         *hi++;
      }
      *lo += amount;
   } else {
      if (*lo + amount > *lo) {
         *hi--;
      }
      *lo += amount;
   }
}

int cmp_lba(unsigned long hi1, unsigned long lo1, unsigned long hi2, unsigned long lo2) {
   if (hi1 > hi2) {
      return 1;
   } else if (hi1 < hi2) {
      return -1;
   } else {
      if (lo1 > lo2) {
         return 1;
      } else if (lo1 < lo2) {
         return -1;
      } else {
         return 0;
      }
   }
}

int diff_lba(unsigned long hi1, unsigned long lo1, unsigned long hi2, unsigned long lo2, long *result) {
   *result = 0;
   if (hi1 == hi2) {
      *result = lo1 - lo2;
      return 0;
   } else if (hi1 == hi2 - 1) {
      if (lo1 > lo2) {
         *result = (long)((double)lo1 - (double)lo2 - pow((double)2,(double)32));
         return 0;
      } else {
         // difference grossly exceeds the range of a long.
         return -1;
      }
   } else if (hi1 == hi2 + 1) {
      if (lo1 < lo2) {
         *result = (long)((double)lo1 - (double)lo2 + pow((double)2,(double)32));
         return 0;
      } else {
         // difference grossly exceeds the range of a long.
         return 1;
      }
   } else if (hi1 > hi2) {
      return 1;
   } else if (hi1 < hi2) {
      return -1;
   }
}

int max_supported_UDMA_mode(WORD w) {
   if (w & 0x0040) {
      return 6;
   } else if (w & 0x0020) {
      return 5;
   } else if (w & 0x0010) {
      return 4;
   } else if (w & 0x0008) {
      return 3;
   } else if (w & 0x0004) {
      return 2;
   } else if (w & 0x0002) {
      return 1;
   } else if (w & 0x0001) {
      return 0;
   }
   return -1;
}

int selected_UDMA_mode(WORD w) {
   if (w & 0x4000) {
      return 6;
   } else if (w & 0x2000) {
      return 5;
   } else if (w & 0x1000) {
      return 4;
   } else if (w & 0x0800) {
      return 3;
   } else if (w & 0x0400) {
      return 2;
   } else if (w & 0x0200) {
      return 1;
   } else if (w & 0x0100) {
      return 0;
   }
   return -1;
}
