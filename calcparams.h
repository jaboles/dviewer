#include "types.h"

#ifndef CALCPARAMS_H_INCLUDED
#define CALCPARAMS_H_INCLUDED

void SigDivZeroHandler(int sig, int code, int *regs);
WORD VerifyLastSector(Drive *, DWORD *, DWORD *);
WORD GetRealLastSector(Drive *, DWORD *, DWORD *, int);
WORD VerifyLastCHS(Drive *, DWORD *, DWORD *, DWORD *);
WORD GetRealLastCHS(Drive *, DWORD *, DWORD *, DWORD *, int);
WORD CalculateDiskParams(Drive *);


#endif