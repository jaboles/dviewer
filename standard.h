#ifndef STANDARD_H_INCLUDED
#define STANDARD_H_INCLUDED

#include "types.h"

WORD DriveLock(WORD, WORD LockDrive);
WORD StandardControllerReset(WORD);
WORD StandardRead(WORD, WORD, WORD, WORD, WORD, char far *dbuf);
WORD StandardWrite(WORD, WORD, WORD, WORD, WORD, char far *dbuf);
WORD StandardGetDiskInfo(WORD, StdDiskParam *);
WORD StandardSeek(BYTE, DWORD, DWORD, DWORD);

#endif