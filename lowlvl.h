/*
   Copyright (c) The Australian Software Company 2002
*/
#ifndef LOWLVL_H_INCLUDED
#define LOWLVL_H_INCLUDED

#include "types.h"

#define HIGH_BYTE(ax) (ax >> 8)
#define LOW_BYTE(ax) (ax & 0xff)


int scan_devices(IDEChannel far *, int far *);
int HasNonIDEController(void);
int bios_detect(Drive* drives, int startId, int maxCount, int offset);
int bios_ata_identify_device(int diskId, ATA_IDENTIFY_DEVICE_Info *info);
int get_IDE_info(unsigned int base, int priSec, ATA_IDENTIFY_DEVICE_Info *buf);


#endif


