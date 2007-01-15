#include "types.h"

#ifndef IDE_H_INCLUDED
#define IDE_H_INCLUDED


#define IDE_COMPATIBILITY 3
#define IDE_NATIVE 4

void activate_ide_drive(Drive *drive);
int detect_ide_drives(IDEChannel far *, int, Drive far *, int);
int populate_drive_info(int dev, Drive *);
int attempt_max_multi_mode(int);
int do_ide_read(Drive *drive, unsigned long lbahi, unsigned long lbalo, unsigned int sc, char far * bufferPtr, int attemptDMA);
void add_lba(unsigned long *hi, unsigned long *lo, long amount);
int cmp_lba(unsigned long hi1, unsigned long lo1, unsigned long hi2, unsigned long lo2);
int diff_lba(unsigned long hi1, unsigned long lo1, unsigned long hi2, unsigned long lo2, long *result);

int max_supported_UDMA_mode(WORD);
int selected_UDMA_mode(WORD);


#endif