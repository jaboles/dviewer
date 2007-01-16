/***********************************************************************************/
/* DESTROY                                                                         */
/* Copyright (C) 1999-2002 by The Australian Software Company Pty Ltd              */
/* All rights reserved.                                                            */
/* This source code is proprietary to The Australian Software Company and must     */
/* not be distributed to any unauthorised person or organisation.                  */
/*                                                                                 */
/* January - July 2002 - Developed by David Quail & Scott Tierney                  */
/* Version 2.01                                                                    */
/*                                                                                 */
/***********************************************************************************/

#include <mem.h>
#include <alloc.h>

#include "standard.h"
#include "extended.h"
#include "utility.h"
#include "constant.h"
#include "ataio.h"

// ============================================================================
// Name: IsINT13ExtensionInstalled
// Description: Check to the see whether the INT13 extensions are installed
//              on the specified disk.
// Inputs:  BYTE drive  Drive
// Returns: WORD        0 if Successful
//                      Other Value Corresponding to register ah.
// If Call successful, extensioninfo.extensioninstalled will be set
// to 1 if INT13 Extensions are installed
// ============================================================================

WORD IsINT13ExtensionInstalled(Drive *d, ExtensionInfo *extInfo)
{
    union REGS regs;
    struct SREGS sregs;

    // initialise data structures
    setmem(&regs, sizeof(regs), 0);
    setmem(&sregs, sizeof(sregs), 0);
    setmem(extInfo, sizeof(ExtensionInfo), 0);

    // Setup Interrupt Service and any required data
    regs.h.ah = INT13_EXT_INSTALL_CHECK;
    regs.h.dl = d->diskId;
    regs.x.bx = INT13_41H_BX;

    // do it
    int86x(INT13, &regs, &regs, &sregs);

    // Check return code, and setup structure with info if successful
    if (regs.x.cflag == 0)
    {
        extInfo->BXregister = regs.x.bx;
        extInfo->majorversion = regs.h.ah;
        extInfo->apisupportbitmap = regs.x.cx;
        extInfo->extensionversion = regs.h.dh;

        if (extInfo->BXregister == INT13_41H_BX_SWAPPED)
            extInfo->extensioninstalled = 1;

        return 0;
    }
    else
    {
        return regs.h.ah;
    }
}


// ============================================================================
// Name: ExtendedSeek
// Description: Jump Directly to a sector on the disk using the LBA Address
// Inputs:  BYTE    Drive
//          DWORD   LBA Sector Address
// Returns: WORD    0 if Successful
//                  Other Value Corresponding to register ah.
// ============================================================================

WORD ExtendedSeek(Drive *d, DWORD LBAAddressHi, DWORD LBAAddressLo)
{
    union REGS regs;
    struct SREGS sregs;
    char *dbuf;
    WORD returncode = 0;
    ExtTransferBuffer readb;

    // Initialise Data Structures
    dbuf = malloc(d->bps);

    // Zero out memory
    setmem(&regs, sizeof(regs), 0);
    setmem(&sregs, sizeof(sregs), 0);
    setmem(dbuf, d->bps, 0);
    setmem(&readb, sizeof(readb), 0);

    // Set Interrupt Service, and required data
    regs.h.ah = INT13_EXT_DISK_SEEK;
    regs.h.dl = d->diskId;

    // Setup the data structure for reading data. Required by the call
    readb.blocks = 1;
    readb.sz = sizeof(readb);
    readb.buffer = dbuf;
    readb.startblock.lowerword = LBAAddressLo;
    readb.startblock.upperword = LBAAddressHi;

    // Set Memory Segment and Offset for call
    sregs.ds = FP_SEG(&readb);
    regs.x.si = FP_OFF(&readb);

    // Seek !!
    int86x(INT13, &regs, &regs, &sregs);

    free(dbuf);

    if (regs.x.cflag != 0)
        returncode = regs.h.ah;

    return(returncode);
}

// ============================================================================
// Name: ExtendedLBARead
// Description: Read Data from the Disk using Extended Read Function
// Inputs:  WORD  drv                   Drive Number
//          DWORD lowerstartsector      Starting Sector to Read From (lower word)
//          DWORD upperstartsector      Starting Sector to Read From (upper word)
//          WORD sectorstoread          Number of Sectors to Read
//          BYTE cleanbuffer            Clean Read Buffer before use
// Returns: WORD    0 if Successful
//                  Other Value Corresponding to register ah.
// ============================================================================

WORD ExtendedLBARead(Drive *d,
                     DWORD lowerstartsector,
                     DWORD upperstartsector,
                     WORD sectorstoread,
                     char *dbuf)
{
    union REGS regs;
    struct SREGS sregs;
    ExtTransferBuffer readb;

    // Zero out memory
    setmem(&regs, sizeof(regs), 0x0);
    setmem(&sregs, sizeof(sregs), 0x0);
    setmem(&readb, sizeof(readb), 0x0);

    readb.buffer = dbuf;
    readb.sz = sizeof(readb);
    readb.blocks = sectorstoread;
    readb.startblock.lowerword = lowerstartsector;
    readb.startblock.upperword = upperstartsector;
    // Set Interrupt Service, and required data
    regs.h.ah = INT13_EXT_DISK_READ;
    regs.h.dl = d->diskId;

    // Setup the data structure for reading data. Required by the call

    // Set Memory Segment and Offset for call
    sregs.ds = FP_SEG(&readb);
    regs.x.si = FP_OFF(&readb);

    int86x(INT13, &regs, &regs, &sregs);

    if (regs.x.cflag != 0)
        return regs.h.ah;
    else
         return 0;
}

// ============================================================================
// Name: ExtendedLBAWrite
// Description: Write Data from the Disk using Extended Write Function
// Inputs:  WORD  drv               Drive Number
//          DWORD startsector       Starting Sector to Read From
//          WORD sectorstoread      Number of Sectors to Read
// Returns: WORD   0 if Successful
//                 Otherwise Value Corresponding to register ah.
// Notes: Assumes that writebuf has been setup, and data is in that structure
// ============================================================================

WORD ExtendedLBAWrite(Drive *d,
                      DWORD lowerstartsector,
                      DWORD upperstartsector,
                      WORD sectorstowrite,
                      char *dbuf)
{
   union REGS regs;
   struct SREGS sregs;
   WORD returncode = 0;
   ExtTransferBuffer writeb;

   // Zero out memory
   setmem(&regs, sizeof(regs), 0x0);
   setmem(&sregs, sizeof(sregs), 0x0);
   setmem(&writeb, sizeof(writeb), 0x0);

   // Set Interrupt Service, and required data
   regs.h.ah = INT13_EXT_DISK_WRITE;
   regs.h.dl = d->diskId;

   // Setup the data structure for reading data. Required by the call
   writeb.buffer = dbuf;
   writeb.sz = sizeof(writeb);
   writeb.blocks = sectorstowrite;
   writeb.startblock.lowerword = lowerstartsector;
   writeb.startblock.upperword = upperstartsector;

   // Set Memory Segment and Offset for call
   sregs.ds = FP_SEG(&writeb);
   regs.x.si = FP_OFF(&writeb);

   int86x(INT13, &regs, &regs, &sregs);

   if (regs.x.cflag != 0)
      returncode = regs.h.ah;



   return(returncode);
}

// ============================================================================
// Name: ExtendedLBAWriteUsingCHS
// Description: Write Data tpo the Disk using Extended Write Functions based on
//              CHS Value, for given number of Sectors
// Inputs: WORD drv                 Drive Number
//         WORD cyl                 Cylinder to Write To
//         WORD head                Head to Write To
//         WORD sector              Sector to Write To
//         WORD sectorstoread       Number of Sectors to Write
// Returns: WORD    0 if Successful
//                  Other Value Corresponding to register ah.
// Notes: Assumes that writebuf has been setup, and data is in that structure
// ============================================================================

WORD ExtendedLBAWriteUsinigCHS(Drive *d,
                               WORD cyl,
                               WORD head,
                               WORD sector,
                               WORD sectorstowrite,
                               char *dbuf)
{
    return(ExtendedLBAWrite(d,
                            LBAFromCHS(cyl,
                                       head,
                                       sector,
                                       d->realConfig.Heads,
                                       d->realConfig.Sectors),
                            0,
                            sectorstowrite,
                            dbuf));
}

// ============================================================================
// Name: ExtendedGetDiskInfo
// Description: Get Disk Information using standard interrupt call
// Inputs:  WORD drive  Drive Number
// Returns: WORD    0 if Successful
//                  Other Value Corresponding to register ah.
// ============================================================================

WORD ExtendedGetDiskInfo(Drive *drive, ExtDiskParam *params)
{
    union REGS regs;
    struct SREGS sregs;
    WORD returncode = 0;

    // Initialse Structures
    setmem(&regs, sizeof(regs), 0x0);
    setmem(&sregs, sizeof(sregs), 0x0);
    setmem(params, sizeof(ExtDiskParam), 0x0);

    // Set Interrupt Service, and required data
    regs.h.ah = INT13_EXT_GET_DISK_PARAMS;
    regs.h.dl = drive->diskId;
    sregs.ds = FP_SEG(params);
    regs.x.si = FP_OFF(params);

    // Setup the data structure. Required by the call
    params->sz = 0x1E; //<- changed due to issue when setting size too large //sizeof(extdiskparam);

    // Do it
    int86x(INT13, &regs, &regs, &sregs);

    // Check return and get other info in structure setup
    if (regs.x.cflag != 0)
        returncode = regs.h.ah;
    else
    {
        if (params->sz <= 0x1A)
            params->versionsupported++;

        if (params->sz > 0x1A && params->sz <= 0x1E)
            params->versionsupported++;

        if (params->sz > 0x1E)
            params->versionsupported++;
    }

    return(returncode);
}
