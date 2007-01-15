/*
   Copyright (c) The Australian Software Company 2002
*/
#ifndef LOWLVL_H_INCLUDED
#define LOWLVL_H_INCLUDED

#include "types.h"
#include "ide.h"


//int get_IDE_info(int drive, ATA_IDENTIFY_DEVICE_Info *buf);
int scan_devices(IDEChannel far *, int far *);
void activate_drive(Drive *drive);
//int numberofIDEdrives(void);
//unsigned long DMA_IDE_Sectors(int drive, ATA_IDENTIFY_DEVICE_Info *buf);
//int kill_IDE_max(int drive, ATA_IDENTIFY_DEVICE_Info *buf);
int bios_detect(Drive* drives, int startId, int maxCount, int driveInfoOffset);
int do_read(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far *bufferPtr);


#endif


