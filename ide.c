/***********************************************************************************/
/* DESTROY                                                                         */
/* Copyright (C) 1999-2007 by The Australian Software Company Pty Ltd              */
/* All rights reserved.                                                            */
/* This source code is proprietary to The Australian Software Company and must     */
/* not be distributed to any unauthorised person or organisation.                  */
/*                                                                                 */
/* ide.c - December 2006-January 2007 - Developed by Jonathan Boles                */
/*                                                                                 */
/***********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "types.h"
#include "ide.h"
#include "ataio.h"
#include "constant.h"
#include "standard.h"
#include "extended.h"
#include "utility.h"

static int trim(char *src, char *dest) {
   int startIndex;
   int endIndex;
   for (startIndex = 0; src[startIndex] == ' ' && startIndex < strlen(src); startIndex++);
   for (endIndex = strlen(src)-1; src[endIndex] == ' ' && endIndex >= startIndex; endIndex--);
   strncpy(dest, src+startIndex, endIndex-startIndex+1);
   dest[endIndex-startIndex+1] = '\0';
   return strlen(dest);
}

void ActivateDrive(Drive *drive) {
   if (drive->accessMode == ACCESS_MODE_ATADRVR) {
      // Set up ATA driver
      reg_buffer_size = BUFFER_SIZE;
      pio_xfer_width = 32;
      tmr_time_out = 5; // 5 sec. timeout on commands.

      pio_set_iobase_addr(drive->ide.command_reg, drive->ide.control_reg, drive->ide.bmcr);
      reg_config();
      reg_reset(0, drive->ide.device);
      attempt_max_multi_mode(drive->ide.device);
      int_enable_irq(0, drive->ide.irq, drive->ide.bmaddr, drive->ide.command_reg + 7);
      dma_pci_config(drive->ide.bmcr);
   } else {
      StandardControllerReset(drive->diskId);
   }
}

int detect_ide_drives(IDEChannel far *ic, int ide_channel_count, Drive far *ide_drives, int driveInfoOffset) {
   int i, foundcount = 0;

   for (i = 0; i < ide_channel_count; i++) {
      //printf( "\nUsing I/O base addresses %04X and %04X with bmcr %04x and irq %d.\n",
      //     ic[i].command_reg, ic[i].control_reg, ic[i].bmcr, ic[i].irq );

      // Set up ATA driver
      reg_buffer_size = BUFFER_SIZE;
      pio_xfer_width = 32;

      // Set up the driver to use this IDE channel
      pio_set_iobase_addr(ic[i].command_reg, ic[i].control_reg, ic[i].bmcr);
      reg_config();

      if (reg_config_info[0] == 2) {
         //printf("Found something as master on controller %d\n", i); getch();
         if (populate_drive_info(0, &ide_drives[foundcount+driveInfoOffset]) == 0) {
            //printf("Found %s, master on controller %d.\n", ide_drives[foundcount+driveInfoOffset].ide.model, i); getch();
            ide_drives[foundcount+driveInfoOffset].ide.command_reg = ic[i].command_reg;
            ide_drives[foundcount+driveInfoOffset].ide.control_reg = ic[i].control_reg;
            ide_drives[foundcount+driveInfoOffset].ide.bmcr = ic[i].bmcr;
            ide_drives[foundcount+driveInfoOffset].ide.bmaddr = ic[i].bmaddr;
            ide_drives[foundcount+driveInfoOffset].ide.irq = ic[i].irq;
            foundcount++;
         }
      }
      if (reg_config_info[1] == 2) {
         //printf("Found something as slave on controller %d\n", i); getch();
         if (populate_drive_info(1, &ide_drives[foundcount+driveInfoOffset]) == 0) {
            //printf("Found %s, slave on controller %d.\n", ide_drives[foundcount+driveInfoOffset].ide.model, i); getch();
            ide_drives[foundcount+driveInfoOffset].ide.command_reg = ic[i].command_reg;
            ide_drives[foundcount+driveInfoOffset].ide.control_reg = ic[i].control_reg;
            ide_drives[foundcount+driveInfoOffset].ide.bmcr = ic[i].bmcr;
            ide_drives[foundcount+driveInfoOffset].ide.bmaddr = ic[i].bmaddr;
            ide_drives[foundcount+driveInfoOffset].ide.irq = ic[i].irq;
            foundcount++;
         }
      }
   }
   return foundcount;
}

int populate_drive_info(int dev, Drive *d) {
   char buffer[512];
   ATA_IDENTIFY_DEVICE_Info rawInfo;
   int rc;
   char tempSerialNo[21];
   char tempFirmware[9];
   char tempModel[41];

   memset(buffer, 0, 512);
   ActivateDrive(d);
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
   swab(rawInfo.serialno, tempSerialNo, 20);
   swab(rawInfo.firmware, tempFirmware, 8);
   swab(rawInfo.model, tempModel, 40);
   tempSerialNo[20] = 0x0;
   tempFirmware[8] = 0x0;
   tempModel[40] = 0x0;
   trim(tempSerialNo, d->ide.serial_no);
   trim(tempFirmware, d->ide.firmware);
   trim(tempModel, d->ide.model);

   // Copy out the other stuff.
   d->type = DISK_TYPE_IDE;
   d->accessMode = ACCESS_MODE_ATADRVR;
   d->bps = 512;
   d->ide.device = dev;
   d->ide.supports_lba48 = ((rawInfo.fe_2 & 0x0400) > 0)? 1 : 0;
   if (d->ide.supports_lba48) {
      d->queriedConfig.totalSectorsHi = rawInfo.lba48_totalsectors[1];
      d->queriedConfig.totalSectorsLo = rawInfo.lba48_totalsectors[0];
   } else {
      d->queriedConfig.totalSectorsHi = 0;
      d->queriedConfig.totalSectorsLo = rawInfo.capsectors;
   }
   d->queriedConfig.Heads = d->realConfig.Heads = rawInfo.log_heads + 1;
   d->queriedConfig.Sectors = d->realConfig.Sectors = rawInfo.log_sectors;
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

WORD ResetController(Drive *d) {
   if (d->accessMode == ACCESS_MODE_ATADRVR) {
      return reg_reset(0, d->ide.device);
   } else {
      return StandardControllerReset(d->diskId);
   }
}

WORD LBARead(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   int c = 0, h = 0, s = 0;
   int sectorsRead = 0;
   int sectorsToRead = 0;
   int sectorsRemainingInCylinder;
   int rc;
   
   //printf("reading %d sectors from %lu,%lu\n", sc, lbahi, lbalo);// getch();
   if (drive->accessMode == ACCESS_MODE_ATADRVR) {
      return do_ide_read_lba(drive, lbahi, lbalo, sc, bufferPtr, attemptDMA);
   } else if (drive->accessMode == ACCESS_MODE_EXTENDED) {
      return ExtendedLBARead(drive, lbalo, lbahi, sc, bufferPtr);
   } else {
      while (sectorsRead < sc) {
         SetCHSFromLBA(lbalo + sectorsRead, &c, &h, &s, drive->queriedConfig.Heads, drive->stdParams.spt);
         sectorsRemainingInCylinder = ((drive->queriedConfig.Heads - h) * drive->stdParams.spt) - (s - 1);
         sectorsToRead = MIN(sc, sectorsRemainingInCylinder);
         //printf("Sectors remaining in cyl: %d  sectors to read: %d\n", sectorsRemainingInCylinder, sectorsToRead);
         //getch();

         //rc = StandardRead(drive->diskId, c, h, s, sectorsToRead, bufferPtr + (sectorsRead * 512));
         rc = CHSRead(drive, c, h, s, sectorsToRead, bufferPtr + (sectorsRead*512), 1);
         if (rc) return rc;
         sectorsRead += sectorsToRead;
      }
      return 0;
   }
}

WORD LBAWrite(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   if (drive->accessMode == ACCESS_MODE_ATADRVR) {
      return do_ide_write_lba(drive, lbahi, lbalo, sc, bufferPtr, attemptDMA);
   } else {
      return ExtendedLBAWrite(drive, lbalo, lbahi, sc, bufferPtr);
   }
}

WORD CHSRead(Drive *drive, unsigned int c, unsigned int h, unsigned int s, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   if (drive->accessMode == ACCESS_MODE_ATADRVR) {
      return do_ide_read_chs(drive, c, h, s, sc, bufferPtr, attemptDMA);
   } else {
      return StandardRead(drive->diskId, c, h, s, sc, bufferPtr);
   }
}

WORD CHSWrite(Drive *drive, unsigned int c, unsigned int h, unsigned int s, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   if (drive->accessMode == ACCESS_MODE_ATADRVR) {
      return do_ide_write_chs(drive, c, h, s, sc, bufferPtr, attemptDMA);
   } else {
      return StandardWrite(drive->diskId, c, h, s, sc, bufferPtr);
   }
}

/**Reads from an IDE drive using LBA, through the ATA driver.
   @param drive The drive to read from
   @param lbahi The high dword of the start sector
   @param lbalo The low dword of the start sector
   @param sc The number of sectors to write
   @param bufferPtr The data to write
   @param attemptDMA Whether or not to attempt writing in DMA mode.
   @return 0 on success, ATADRVR's error code otherwise. */
int do_ide_read_lba(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   int totalSectorsToRead = sc;
   int sectorsToRead = 0;
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
                           FP_SEG(bufferPtr+(lbaOffset*drive->bps)), FP_OFF(bufferPtr+(lbaOffset*drive->bps)),
                           sectorsToRead);
         } else {
            rc = dma_pci_lba28(drive->ide.device, CMD_READ_DMA,
                           0, (int)sectorsToRead,
                           lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*drive->bps)), FP_OFF(bufferPtr+(lbaOffset*drive->bps)),
                           sectorsToRead);
         }
      }
      if (rc > 0) {
         ResetController(drive);
      }
      if (attemptDMA <= 0 || rc > 0) {
         if (drive->ide.supports_lba48) {
            rc = reg_pio_data_in_lba48(drive->ide.device, CMD_READ_SECTORS_EXT,
                           0, (int)sectorsToRead,
                           lbaStartHi, lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*drive->bps)), FP_OFF(bufferPtr+(lbaOffset*drive->bps)),
                           sectorsToRead, 0);
         } else {
            rc = reg_pio_data_in_lba28(drive->ide.device, CMD_READ_SECTORS,
                           0, (int)sectorsToRead,
                           lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*drive->bps)), FP_OFF(bufferPtr+(lbaOffset*drive->bps)),
                           sectorsToRead, 0);
         }
      }
      totalSectorsToRead -= sectorsToRead;
   }
   return rc;
}

/**Writes to an IDE drive using LBA, through the ATA driver.
   @param drive The drive to write to
   @param lbahi The high dword of the start sector.
   @param lbalo The low dword of the start sector.
   @param sc The number of sectors to write
   @param bufferPtr The data to write
   @param attemptDMA Whether or not to attempt writing in DMA mode.
   @return 0 on success, ATADRVR's error code otherwise. */
int do_ide_write_lba(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   int totalSectorsToWrite = sc;
   int sectorsToWrite = 0;
   int lbaOffset = 0;
   unsigned long lbaStartHi;
   unsigned long lbaStartLo;
   int rc;

   while (totalSectorsToWrite > 0) {
      sectorsToWrite = totalSectorsToWrite>256? 256 : totalSectorsToWrite;
      lbaOffset = sc - totalSectorsToWrite;

      lbaStartHi = lbahi;                           //
      lbaStartLo = lbalo;                           //  i.e. lbaStart = lba + lbaOffset
      add_lba(&lbaStartHi, &lbaStartLo, lbaOffset); //
      if (attemptDMA > 0) {
         if (drive->ide.supports_lba48 > 0) {
            rc = dma_pci_lba48(drive->ide.device, CMD_WRITE_DMA_EXT,
                           0, (int)sectorsToWrite,
                           lbaStartHi, lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*drive->bps)), FP_OFF(bufferPtr+(lbaOffset*drive->bps)),
                           sectorsToWrite);
         } else {
            rc = dma_pci_lba28(drive->ide.device, CMD_WRITE_DMA,
                           0, (int)sectorsToWrite,
                           lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*drive->bps)), FP_OFF(bufferPtr+(lbaOffset*drive->bps)),
                           sectorsToWrite);
         }
      }
      if (rc > 0) {
         ResetController(drive);
      }
      if (attemptDMA <= 0 || rc > 0) {
         if (drive->ide.supports_lba48) {
            rc = reg_pio_data_in_lba48(drive->ide.device, CMD_WRITE_SECTORS_EXT,
                           0, (int)sectorsToWrite,
                           lbaStartHi, lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*drive->bps)), FP_OFF(bufferPtr+(lbaOffset*drive->bps)),
                           sectorsToWrite, 0);
         } else {
            rc = reg_pio_data_in_lba28(drive->ide.device, CMD_WRITE_SECTORS,
                           0, (int)sectorsToWrite,
                           lbaStartLo,
                           FP_SEG(bufferPtr+(lbaOffset*drive->bps)), FP_OFF(bufferPtr+(lbaOffset*drive->bps)),
                           sectorsToWrite, 0);
         }
      }
      totalSectorsToWrite -= sectorsToWrite;
   }
   return rc;
}

/**Reads from an IDE drive using CHS, through the ATA driver.
   @param drive The drive to read from
   @param c The cylinder location at which the read will start
   @param h The head at which the read will start
   @param s The sector at which the read will start
   @param sc The number of sectors to read
   @param bufferPtr Where to store the read data
   @param attemptDMA Whether or not to attempt reading in DMA mode.
   @return 0 on success, ATADRVR's error code otherwise. */
int do_ide_read_chs(Drive *drive, unsigned int c, unsigned int h, unsigned int s, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   int rc;

   if (attemptDMA > 0) {
      rc = dma_pci_chs(drive->ide.device, CMD_READ_DMA_EXT,
            0, sc,
            c, h, s,
            FP_SEG(bufferPtr), FP_OFF(bufferPtr),
            sc);
   }
   if (rc > 0) {
      ResetController(drive);
   }
   if (attemptDMA <= 0 || rc > 0) {
      rc = reg_pio_data_in_chs(drive->ide.device, CMD_READ_SECTORS_EXT,
         0, sc,
         c, h, s,
         FP_SEG(bufferPtr), FP_OFF(bufferPtr),
         sc, 0);
   }
   return rc;
}

/**Writes to an IDE drive using CHS, through the ATA driver.
   @param drive The drive to write to
   @param c The cylinder location at which the write will start
   @param h The head at which the write will start
   @param s The sector at which the write will start
   @param sc The number of sectors to write
   @param bufferPtr The data to write
   @param attemptDMA Whether or not to attempt writing in DMA mode.
   @return 0 on success, ATADRVR's error code otherwise. */
int do_ide_write_chs(Drive *drive, unsigned int c, unsigned int h, unsigned int s, unsigned int sc, char far * bufferPtr, int attemptDMA) {
   int rc;

   if (attemptDMA > 0) {
      rc = dma_pci_chs(drive->ide.device, CMD_WRITE_DMA_EXT,
            0, sc,
            c, h, s,
            FP_SEG(bufferPtr), FP_OFF(bufferPtr),
            sc);
   }
   if (rc > 0) {
      ResetController(drive);
   }
   if (attemptDMA <= 0 || rc > 0) {
      rc = reg_pio_data_in_chs(drive->ide.device, CMD_WRITE_SECTORS_EXT,
         0, sc,
         c, h, s,
         FP_SEG(bufferPtr), FP_OFF(bufferPtr),
         sc, 0);
   }
   return rc;
}

// Adds the value 'amount' to the 64-bit value [hi:lo]. Amount can
// be positive or negative.
void add_lba(unsigned long *hi, unsigned long *lo, long amount) {
   if (amount > 0) {
      // if there is going to be overflow, then increment the high word.
      if (*lo + (unsigned long)amount < *lo) {
         (*hi)++;
      }
      *lo += amount;
   } else {
      if (*lo + amount > *lo) {
         (*hi)--;
      }
      *lo += amount;
   }
}

// Compares two lba values.
// Returns 1 if lba1 > lba2
// Returns 0 if lba1 == lba2
// Returns -1 if lba1 < lba2
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

// Calculates the difference between two 64-bit LBA values [hi1:lo1] and [hi2:lo2], in other words,
// lba1-lba2. If an overflow occurs (the result of the subtraction is too large for the 'result' variable)
// then 1 is returned. If an underflow occurs, -1 is returned. Otherwise, 0 will be returned and
// 'result' will contain the difference between the two LBA values.
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
   return 0;
}

// Given the UDMA word from the data returned by an ATA IDENTIFY DEVICE
// command, returns the maximum-supported Ultra DMA mode.
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

// Given the UDMA word from the data returned by an ATA IDENTIFY DEVICE
// command, returns the currently-selected Ultra DMA mode.
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

// Given a hard drive model number, finds the appropriate
// manufacturer. The only point to this is that sometimes the model number
// does not contain the manufacturer name.
void GetDriveManufacturer(char *model, char *output) {
   if (strncmpi(model, "ST", 2) == 0) {
      strcpy(output, "Seagate");
   } else if (strncmpi(model, "WD", 2) == 0) {
      strcpy(output, "Western Digital");
   } else if (strncmpi(model, "IC", 2) == 0) {
      strcpy(output, "IBM");
   } else if (strncmpi(model, "HDS", 3) == 0) {
      strcpy(output, "Hitachi");
   } else {
      strcpy(output, "");
   }
}
