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

#include "standard.h"
#include "constant.h"
#include "utility.h"
#include "ataio.h"

// Standard Functions (Non INT13 Extension Functions)

// ============================================================================
// Name: ControllerReset
// Description: Reset the Harddisk Controller
// Inputs:      WORD drv    Drive Number
// Returns:        WORD        0 if Successful
//                            Other Value Corresponding to register ah.
// ============================================================================

WORD StandardControllerReset(WORD drv)
{
    union REGS regs;
    struct SREGS sregs;
    WORD returncode = 0;

    // Zero out memory
    setmem(&regs, sizeof(regs), 0);
    setmem(&sregs, sizeof(sregs), 0);

    // Set Interrupt Service, and required data
    regs.h.ah = INT13_CONTROLLER_RESET;
    regs.h.dl = drv;
    // Do it
    int86x(INT13, &regs, &regs, &sregs);
    if (regs.x.cflag != 0)
         returncode = regs.h.ah;

    return(returncode);
}

// ============================================================================
// Name: StandardRead
// Description: Read Data from the Disk using Standard Read Function
// Inputs:      WORD drv                Drive Number
//              WORD cyl                Cylinder to Read From
//              WORD head               Head to Read From
//              WORD sector             Sector to Read From
//              WORD sectorstoread      Number of Sectors to Read
//              BYTE cleanbuffer        Clean Read Buffer before use
// Returns:        WORD         0 if Successful
//                            Other Value Corresponding to register ah.
// ============================================================================

WORD StandardRead(WORD drv, 
                  WORD cyl,
                  WORD head, 
                  WORD sector,
                  WORD sectorstoread, 
                  char far *dbuf)
{
    union REGS regs;
    struct SREGS sregs;
    WORD returncode = 0;

    //printf("entered standardRead\a\n"); getch();

    // Zero out memory
    setmem(&regs, sizeof(regs), 0);
    setmem(&sregs, sizeof(sregs), 0);

    // Set Interrupt Service, and required data
    regs.h.ah = INT13_STD_DISK_READ;

    // Maximum Number of Sectors to read is 63
    // sector can't exceed 63 anyway prep wont let it
    // number of sectors to read (must be nonzero)
    regs.h.al = sectorstoread;

    // high two bits of cylinder (bits 6-7, hard disk only)
    // sector number 1-63 (bits 0-5)
    regs.h.cl = (prep(cyl,2) << 6) ^ prep((sector > 63) ? 63 : sector,0);
    regs.h.ch = prep(cyl,1);      // CH = low eight bits of cylinder number
    regs.h.dh = head;
    regs.h.dl = drv;

    // Set Memory Segment and Offset for call
    sregs.es = FP_SEG(dbuf);
    regs.x.bx = FP_OFF(dbuf);

    //printf("calling interrupt\a\n"); getch();

    // Do it
    int86x(INT13, &regs, &regs, &sregs);
    //printf("returned from interrupt\a\n"); getch();

    if (regs.x.cflag != 0)
        returncode = regs.h.ah;

    return(returncode);
}

// ============================================================================
// Name: StandardWrite
// Description: Writes Data from the Disk using Standard Write Function
// Inputs:  WORD drv                Drive Number
//          WORD cyl                Cylinder to Write From
//          WORD head               Head to Write From
//          WORD sector             Sector to Write From
//          WORD sectorstowrite     Number of Sectors to write
// Returns: WORD   0 if Successful
//                 Other Value Corresponding to register ah.
// Assumes that the writebuf structure has been populated with data.
// ============================================================================

WORD StandardWrite(WORD drv, WORD cyl, WORD head, WORD sector, WORD sectorstowrite, char far *dbuf)
{
    union REGS regs;
    struct SREGS sregs;
    WORD returncode = 0;

    // Zero out memory
    setmem(&regs, sizeof(regs), 0);
    setmem(&sregs, sizeof(sregs), 0);

    // Set Interrupt Service, and required data
    regs.h.ah = INT13_STD_DISK_WRITE;

    // number of sectors to write (must be nonzero)
    // sector can't exceed 63 anyway prep wont let it
    regs.h.al = sectorstowrite;

    // high two bits of cylinder (bits 6-7, hard disk only)
    // sector number 1-63 (bits 0-5)
    regs.h.cl = (prep(cyl,2) << 6) ^ prep((sector > 63) ? 63 : sector,0);

    // CH = low eight bits of cylinder number
    regs.h.ch = prep(cyl, 1);
    regs.h.dh = head;
    regs.h.dl = drv;

    // Set Memory Segment and Offset for call
    sregs.es = FP_SEG(dbuf);
    regs.x.bx = FP_OFF(dbuf);

    // Do It!!
    int86x(INT13, &regs, &regs, &sregs);

    if (regs.x.cflag != 0)
        returncode = regs.h.ah;

    return(returncode);
}

// ============================================================================
// Name: StandardGetDiskInfo
// Description: Get Disk Information using standard interrupt call
// Inputs:  WORD drive      Drive Number
// Returns: WORD        0 if Successful
//                      Other Value Corresponding to register ah.
// ============================================================================

WORD StandardGetDiskInfo(WORD drive, StdDiskParam *sdp)
{
    union REGS regs;
    struct SREGS sregs;

    setmem(&regs, sizeof(regs), 0);
    setmem(&sregs, sizeof(sregs), 0);
    setmem(sdp, sizeof(StdDiskParam), 0);

    regs.h.ah = INT13_STD_GET_DISK_PARAMS;
    regs.h.dl = drive;
    int86x(INT13, &regs, &regs, &sregs);

    if (regs.x.cflag != 0)
    {
        return regs.h.ah;
    }
    else
    {
        sdp->cylinders  = (prep(regs.h.cl, 3) << 8) | regs.h.ch;
        sdp->spt        = prep(regs.h.cl,0); // kill the high order stuff from cl
        sdp->heads      = regs.h.dh;
        sdp->totaldrives = regs.h.dl;
        return 0;
    }
}

// ============================================================================
// Name: StandardSeek
// Description: Jump Directly to a sector on the disk using the LBA Address
// Inputs:      BYTE  Drive     Drive Number
//              DWORD Cyl       Cylinder to Seek to
//              DWORD Head      Head to Seek to
//              DWORD Sec       Sector to Seek to
// Returns:     WORD        0 if Successful
//                          Other Value Corresponding to register ah.
// ============================================================================

WORD StandardSeek(BYTE drv, DWORD Cyl, DWORD Head, DWORD Sec)
{
    union REGS regs;
    struct SREGS sregs;
    WORD returncode = 0;

    setmem(&regs, sizeof(regs), 0);
    setmem(&sregs, sizeof(sregs), 0);

    // Set Interrupt Service, and required data
    regs.h.ah = INT13_STD_DISK_SEEK;

    // high two bits of cylinder (bits 6-7, hard disk only)
    // sector number 1-63 (bits 0-5)
    regs.h.cl = (prep((WORD)Cyl,2) << 6) ^ prep((Sec > 63) ? 63 : (WORD)Sec,0);
    regs.h.ch = prep((WORD)Cyl,1);      // CH = low eight bits of cylinder number
    regs.h.dh = (WORD)Head;
    regs.h.dl = drv;

    // Do it
    int86x(INT13, &regs, &regs, &sregs);

    if (regs.x.cflag != 0)
        returncode = regs.h.ah;

    return(returncode);
}
