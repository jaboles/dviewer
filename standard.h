#ifndef STANDARD_H_INCLUDED
#define STANDARD_H_INCLUDED

#include "types.h"

WORD DriveLock(WORD drv, WORD LockDrive);
WORD ControllerReset(WORD);
WORD StandardRead(WORD, WORD, WORD, WORD, WORD, char far *);
WORD StandardWrite(WORD, WORD, WORD, WORD, WORD, char far *);
WORD StandardGetDiskInfo(WORD, Drive far *driveInfo);
WORD StandardSeek(BYTE, DWORD, DWORD, DWORD);

#endif