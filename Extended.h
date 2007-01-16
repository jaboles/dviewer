#ifndef EXTENDED_H_INCLUDED
#define EXTENDED_H_INCLUDED

#include "types.h"

// Extended Functions (INT13 Extensions)
WORD IsINT13ExtensionInstalled(Drive *, ExtensionInfo *);
WORD ExtendedSeek(Drive *, DWORD, DWORD);
WORD ExtendedLBARead(Drive *d, DWORD, DWORD, WORD, char *);
WORD ExtendedLBAWrite(Drive *d, DWORD, DWORD, WORD, char *);
WORD ExtendedLBAWriteUsinigCHS(Drive *d, WORD, WORD, WORD, WORD, char *);
WORD ExtendedGetDiskInfo(Drive *d, ExtDiskParam *param);

#endif