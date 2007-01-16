/***********************************************************************************/
/* DESTROY                                                                         */
/* Copyright (C) 1999-2007 by The Australian Software Company Pty Ltd              */
/* All rights reserved.                                                            */
/* This source code is proprietary to The Australian Software Company and must     */
/* not be distributed to any unauthorised person or organisation.                  */
/*                                                                                 */
/* ide.h - December 2006-January 2007 - Developed by Jonathan Boles                */
/*                                                                                 */
/***********************************************************************************/
#include "types.h"

#ifndef IDE_H_INCLUDED
#define IDE_H_INCLUDED

#define COMBINE_LBA(lbahi,lbalo) ((((double)lbahi)*pow(2,32))+(double)lbalo)
#define MIN(a,b) (a<b? a : b)

int detect_ide_drives(IDEChannel far *, int, Drive far *, int);
int populate_drive_info(int dev, Drive *);
int attempt_max_multi_mode(int);

void ActivateDrive(Drive *drive);
WORD ResetController(Drive *);
WORD LBARead(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far *bufferPtr, int attemptDMA);
WORD LBAWrite(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far *bufferPtr, int attemptDMA);
WORD CHSRead(Drive *drive, unsigned int c, unsigned int h, unsigned int s, unsigned int sc, char far *bufferPtr, int attemptDMA);
WORD CHSWrite(Drive *drive, unsigned int c, unsigned int h, unsigned int s, unsigned int sc, char far *bufferPtr, int attemptDMA);

int do_ide_read_lba(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far * bufferPtr, int attemptDMA);
int do_ide_write_lba(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far * bufferPtr, int attemptDMA);
int do_ide_read_chs(Drive *drive, unsigned int c, unsigned int h, unsigned int s, unsigned int sc, char far * bufferPtr, int attemptDMA);
int do_ide_write_chs(Drive *drive, unsigned int c, unsigned int h, unsigned int s, unsigned int sc, char far * bufferPtr, int attemptDMA);
void add_lba(unsigned long *hi, unsigned long *lo, long amount);
int cmp_lba(unsigned long hi1, unsigned long lo1, unsigned long hi2, unsigned long lo2);
int diff_lba(unsigned long hi1, unsigned long lo1, unsigned long hi2, unsigned long lo2, long *result);

int max_supported_UDMA_mode(WORD);
int selected_UDMA_mode(WORD);
void GetDriveManufacturer(char *model, char *output);


#endif