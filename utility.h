#include "types.h"

#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

DWORD LBAFromCHS(DWORD c, DWORD h, DWORD s, DWORD hpc, DWORD spt);
void SetCHSFromLBA(DWORD LBA, WORD *Cyl, WORD *Head, WORD *Sec, WORD hpc, WORD spt);
BYTE prep(WORD val, int mode);

#endif