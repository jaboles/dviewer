#include "utility.h"

// ============================================================================
// Name: LBAFromCHS
// Description: Calculate LBAAddress Given CHS parameters
// Inputs:		WORD c			Cylinder
//				WORD h			Head
//				WORD s	        Sector
//				WORD hpc		Heads Per Cylinder
//				WORD spt		Sectors Per Track
// Returns: DWORD - LBA Translated Address
// ============================================================================

DWORD LBAFromCHS(DWORD c, DWORD h, DWORD s, DWORD hpc, DWORD spt)
{
    DWORD LBAAddress = 0;

    LBAAddress = s - 1;
    LBAAddress += (spt * (h + (c * hpc)));
    return(LBAAddress);
}

// ============================================================================
// Name: LBAFromCHS
// Description: Calculate CHS parameters given LBA Address.
// Inputs:      DWORD LBA       LBA Address
//	            WORD *Cyl       Value of Cylinder from LBA
//	            WORD *Head      Value of Head from LBA
//	            WORD *Cyl       Value of Sector from LBA
// Returns: None
// ============================================================================

void SetCHSFromLBA(DWORD LBA, WORD *Cyl, WORD *Head, WORD *Sec, WORD hpc, WORD spt)
{
   *Sec = (WORD)((LBA % spt) + 1);
   *Head = (WORD)((LBA / spt) % hpc);
   *Cyl = (WORD)((LBA / spt) / hpc);
}

// ============================================================================
// Name: prep
// Description: Take an Integer Value and Break into desired components. Used
//					 primarily in decoding CHS values from INT13/08h Call
// Inputs:		WORD val			Word Value
//					int  mode		Decode Type
//										0 for Sector Decoding
//										1 is Lo-Value of Cylinder
//										2 is Hi-Value of Cylinder
// Returns: BYTE - Value
// ============================================================================

BYTE prep(WORD val, int mode)
{
   struct bytes
   {
      unsigned int lo : 6;
      unsigned int hi : 2;
   };
   struct bitints
   {
      unsigned lo : 8;
      unsigned hi : 2;
      unsigned ig : 6;
   };
   struct words
   {
      WORD val;
   };
   union breakit
   {
      struct words  i;
      struct bitints bi;
      struct bytes b;
   };

   union breakit v;
   v.i.val = val;
   if (mode == 0)
      return(v.b.lo);
   if (mode == 1)
      return(v.bi.lo);
   if (mode == 2)
      return(v.bi.hi);
   if (mode == 3)
      return(v.b.hi);
   return(0);
}
