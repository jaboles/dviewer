#include <signal.h>
#include <setjmp.h>
#include <alloc.h>
#include <ctype.h>
#include <math.h>
#include <mem.h>
#include <float.h>
#include "types.h"
#include "ide.h"
#include "constant.h"
#include "calcparams.h"
#include "standard.h"
#include "globals.h"

jmp_buf DiskReadJump;



// ============================================================================
// Name: SigDivZeroHandler
// Description: Signal Handler to Catch Divide By Zero Errors during reads.
//              Came about due to issues with HP Vectra VC6/350 machines.
//              When read exectured where sector (LBA) was over the end of
//              disk, caused program crash.
// Inputs:		int sig     Signal Caught
//              int code    Signal Sub Code
//              int *regs   List of Register Values
// Returns: char *	Data Buffer Address
// ============================================================================

void SigDivZeroHandler(int sig, int code, int *regs)
{
   if (code == FPE_INTDIV0)
   {
       longjmp(DiskReadJump, DISKREAD_DETECTFAIL);
   }
   else
   {
      signal(SIGFPE, SIG_DFL);
   }
}


WORD VerifyLastSector(Drive * d, DWORD *RealLastSectorHi, DWORD *RealLastSectorLo)
{
    // This is a method introduced after some testing with Hard Disks.
    // It was found on one particular disk, that doing a check though
    // INT13/AH=48h gave TotalSectors of X. Doing a physical check gave
    // Y. But the real last sector, Z, was defined as : X < Z < Y. This
    // indicated some sort of disk error. After running a tool, spinrite,
    // it appeared to fix the erroring Hard Disk. But we can not insist
    // that customers run spinrite. So, given the RealLastSector, do a Read.
    // if it Fails, then so a slow, but accurate check of the disk to get the
    // end of the disk.
    WORD error;
    DWORD SlowRealLastSectorHi = 0;
    DWORD SlowRealLastSectorLo = 0;
    DWORD tempHi = 0;
    DWORD tempLo = 0;
    WORD ReadSignalError;
    char *buffer = farmalloc(d->bps);

#ifdef DISKWIPE_DIAGNOSTIC
    txtline(1, 1, 1, "Verify Last Sector %.0lf", COMBINE_LBA(*RealLastSectorHi,*RealLastSectorLo));
#endif

    ResetController(d);

    signal(SIGFPE, SigDivZeroHandler);

    ReadSignalError = 0;
    ReadSignalError = setjmp(DiskReadJump);

    if (ReadSignalError == 0)
    {
        tempHi = *RealLastSectorHi;
        tempLo = *RealLastSectorLo;
        add_lba(&tempHi, &tempLo, -1);
        error = LBARead(d, tempHi, tempLo, 1, buffer, 1);
        signal(SIGFPE, SIG_DFL);
    }
    else
    {
        error = 4;   // Error generated from SIGFPE Handler
    }

    if (!error)
    {
        // This is good. Last Sector was re-read successfully after a controller
        // reset.
#ifdef DISKWIPE_DIAGNOSTIC
        txtline(1, 1, 1, "Verify Last Sector %.0lf Successful", COMBINE_LBA(*RealLastSectorHi,*RealLastSectorLo));
#endif
        farfree(buffer);
        return(0);
    }

    // OK - this is not so good. It's is from experience an indication that the
    // disk controller may be a little bit flakey. So re-query using HDD
    // controller resets before each read.

    GetRealLastSector(d, &SlowRealLastSectorHi, &SlowRealLastSectorLo, 1);

    // try the verification of this sector
    ResetController(d);

    signal(SIGFPE, SigDivZeroHandler);

    ReadSignalError = 0;
    ReadSignalError = setjmp(DiskReadJump);

    if (ReadSignalError == 0)
    {
        tempHi = SlowRealLastSectorHi;
        tempLo = SlowRealLastSectorLo;
        add_lba(&tempHi, &tempLo, -1);
        error = LBARead(d, tempHi, tempLo, 1, buffer, 1);
        signal(SIGFPE, SIG_DFL);
    }
    else
    {
        error = 4;   // Error generated from SIGFPE Handler
    }

    if (!error)
    {
        // This is good. Last Sector was re-read successfully after a controller
        // reset.
        *RealLastSectorHi = SlowRealLastSectorHi;
        *RealLastSectorLo = SlowRealLastSectorLo;
        farfree(buffer);
        return(0);
    }

    // ok - well it just looks like the disk is completely screwed. So error
    farfree(buffer);
    return(error);
}

WORD GetRealLastSector(Drive *d, DWORD *RealLastSectorHi, DWORD *RealLastSectorLo, int ClearController)
{
    // now that we got the total number of sectors on the disk (LBA Addresses)
    // query and continue to query the disk sectors until we get an error.
    // this is essentially required because some BIOS disk configurations dont
    // report hidden cylinders through calls to INT13/AH=08h and INT13/AH=48h
    // 22/10/2001 - ST - Originally I had coded this functionality as a seek.
    // But on a Dell Bios with a 60GB drive, I found the Seek mechanism report
    // more sectors than actual. So Go back to a read, which is more precise,
    // just a slight bit slower, as the data actually has to be read off the disk.
    // 26/10/2001 - ST - More Testing on different drives found another "issue"
    // with an older Seagate ST32122A. For some reason it reported the size of the
    // drive in sectors a lot higher when using the below code. When you then used
    // the viewing software, or called wipe on the disk, you got disk errors. After
    // some experimentation, it was found that the controller appeared to be
    // dodgy. So perform a ControllerReset before doing any reading. This solved the
    // issue.
    // 31/10/2001 - ST - Changed Code back to old method, but added parameter
    // to function prototype to do the controller reset. So read as per normal,
    // and before returning the sector number, do a verify read of the calculated
    // sector. This will ensure then that the Last sector is really there.
    long Jump = 1000000l;
    WORD error;
    WORD ReadSignalError;
    double currentMBPosition = 0;
    char *buffer = farmalloc(d->bps);
    DWORD dodgyControllerSectorHi = 0;
    DWORD dodgyControllerSectorLo = 0;
    WORD dodgyControllerError = 0;
    int i;

    *RealLastSectorHi = 0;
    *RealLastSectorLo = 0;
    //if (ClearController)
        ResetController(d);

    // Test - LBA Check
#if defined(DESTROY_MAD_DEBUGGING) || defined(DISKWIPE_DIAGNOSTIC)
    txtline(1, 20, 1, "Drv:0x%02x Sector:%.0f CC:%d", d->diskId, COMBINE_LBA(*RealLastSectorHi,*RealLastSectorLo), ClearController);
#ifndef DISKWIPE_DIAGNOSTIC
    getch();
#endif
#endif

    signal(SIGFPE, SigDivZeroHandler);

    ReadSignalError = 0;
    ReadSignalError = setjmp(DiskReadJump);

    if (ReadSignalError == 0)
    {
       error = LBARead(d, *RealLastSectorHi, *RealLastSectorLo, 1, buffer, 1);
    }
    else
    {
        error = 4;
    }

    while (error == 0)
    {
        //*RealLastSector += Jump;
        add_lba(RealLastSectorHi, RealLastSectorLo, Jump);
        if (ClearController)
            ResetController(d);

        // DQ change March 06
        // The sectors are approaching the limited of the 16 bit system which
        // is theoretically possible however may not be well supported.
        // At this point lets just say the last sector is the reported sector
        // This is an issue with a Compaq Deskpro EN where the drive does not fail when attempting
        // to read beyond the max sectors. Basically this process infinitely loops because there is
        // never an error. This code would/could also correct an issue if sector mapping wrapped at the
        // end boundary back to zero + x sectors. Ideally a test of wrapping should be done however in the case of
        // the compaq it does not appear to be an issue caused by sector wrapping since viewing sectors
        // end + 1 or more does not show the first sectors.

        // JB December 06
        // The largest drive today is 750gb. We need to compare the sector count with a value
        // that's high enough to accommodate some future growth, but not so high that we're not
        // jumping 1000000 sectors over and over again for ages. So how about 2 terabytes?
        currentMBPosition = COMBINE_LBA(*RealLastSectorHi,*RealLastSectorLo)*(double)STD_BYTES_PER_SECTOR/1024.0/1024.0;
        //printf("%.2lf\n", currentMBPosition);
        if (currentMBPosition > 2000000L && Jump == 1000000l)
        {
           farfree(buffer);
           return(3); // returns an error which will force the system to use reported.
        }

        ReadSignalError = 0;
        ReadSignalError = setjmp(DiskReadJump);

        if (ReadSignalError == 0)
        {
            error = LBARead(d, *RealLastSectorHi, *RealLastSectorLo, 1, buffer, 1);
        }
        else
        {
            error = 4; // Sector not found - generated from SIGNAL SIGFPE
            signal(SIGFPE, SigDivZeroHandler);
        }

        if (error != 0) // Sector not found
        {
            // Added 16/5/2002 - ST - Check the Return Code. If its 0x80, perform a controller reset
            // Seen on 36GB SCSI Disk - 0x80 is a timeout
            if (error == 0x80)
            {
                ResetController(d);
            }
            // Jan 2007 - JB - Using extended mode to access a Fujitsu MPC on a very old HP Vectra
            // seemed to produce near-random occurrences of 0x04 (indicating an out of range sector
            // when the sector in fact is valid or 0xAA ('Drive not ready'). So if we run into error
            // 0xAA, we need to stop using extended mode and do CHS mode instead.
            // The same HP machine performed very differently when two drives were attached to the
            // IDE port -- there would be no 0xAA, but the same 0x04 error would occur causing the
            // physically-detected size of the drive to be grossly inaccurate (the 3gb Fujitsu MPC was
            // detected as 588mb, while a 2gb Seagate ST32122A was detected as 8gb). So if we get 0x04,
            // try doing a couple more jumps after and see if we get the same error. If we keep getting
            // 0x04, we know there really are no more sectors. If one of the reads has no error, then
            // we have a stuffed controller. Need to abort the physical and just use the bios queried
            // values.
            if (error == 0xAA) {
               farfree(buffer);
               return 0xAA;
            } else if (error == 0x04) {
               // Try a few more jumps
               dodgyControllerSectorHi = *RealLastSectorHi;
               dodgyControllerSectorLo = *RealLastSectorLo;
               for (i = 0; i < 6; i++) {
                  dodgyControllerError = LBARead(d, dodgyControllerSectorHi, dodgyControllerSectorLo, 1, buffer, 1);
                  //printf("dodgy HP controller error reading %.0lf: %d\n", COMBINE_LBA(dodgyControllerSectorHi,dodgyControllerSectorLo), dodgyControllerError);
                  if (dodgyControllerError == 0) {
                     // The read returned OK for a sector number that's higher
                     // that one we've already got an 0x04 error for. So we can
                     // conclude, the controller is really stuffed and we need
                     // to use the queried values.
                     farfree(buffer);
                     return 0x04;
                  }
                  add_lba(&dodgyControllerSectorHi, &dodgyControllerSectorLo, Jump);
               }
            }

            if (Jump > 1)
            {
                // December 06 - JB - changed to support 48b LBA
                //*RealLastSector -= Jump;
                add_lba(RealLastSectorHi, RealLastSectorLo, -Jump);

                Jump /= 10;
                // Reset error flag - this ensures that we try again
                error = 0;
            }
        }
    }

    // Reset to default action
    signal(SIGFPE, SIG_DFL);

    farfree(buffer);
    if (!ClearController) {
        return(VerifyLastSector(d, RealLastSectorHi, RealLastSectorLo));
    } else {
       return(0);
    }
}

WORD VerifyLastCHS(Drive *d,
                   DWORD *RealCyl,
                   DWORD *RealHead,
                   DWORD *RealSec)
{
    // This is a method introduced after some testing with Hard Disks.
    // It was found on one particular disk, that doing a check though
    // INT13/AH=48h gave TotalSectors of X. Doing a physical check gave
    // Y. But the real last sector, Z, was defined as : X < Z < Y. This
    // indicated some sort of disk error. After running a tool, spinrite,
    // it appeared to fix the erroring Hard Disk. But we can not insist
    // that customers run spinrite. So, given the RealLastSector, do a Read.
    // if it Fails, then so a slow, but accurate check of the disk to get the
    // end of the disk.
    WORD error;
    DWORD SlowRealLastCyl = 0;
    DWORD SlowRealLastHead = 0;
    DWORD SlowRealLastSect = 0;
    char *buffer = farmalloc(STD_BYTES_PER_SECTOR);

    ResetController(d);
    error = StandardRead(d->diskId,
                         (WORD)(*RealCyl - 1),
                         (WORD)(*RealHead - 1),
                         (WORD)*RealSec,
                         1,
                         buffer);
    if (!error)
    {
        // This is good. Last Sector at CHS was re-read successfully after a controller
        // reset.
        //txtline(1, 24, 1, "Verify OK");
        farfree(buffer);
        return(0);
    }
#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,22,1,"Verify Failure - Further testing now");
#endif
    //txtline(1, 23, 1, "Verify Failed");
    // OK - this is not so good. It's is from experience an indication that the
    // disk controller may be a little bit flakey. So re-query using HDD
    // controller resets before each read.
    GetRealLastCHS(d, &SlowRealLastCyl, &SlowRealLastHead, &SlowRealLastSect, 1);

    // try the verification of this sector
    ResetController(d);
    error = StandardRead(d->diskId,
                         (WORD)(SlowRealLastCyl - 1),
                         (WORD)(SlowRealLastHead - 1),
                         (WORD)SlowRealLastSect,
                         1,
                         buffer);
    farfree(buffer);

    if (!error)
    {
        // This is good. Last Sector was re-read successfully after a controller
        // reset.
        *RealCyl = SlowRealLastCyl;
        *RealHead = SlowRealLastHead;
        *RealSec = SlowRealLastSect;
        return(0);
    }

    // ok - well it just looks like the disk is completely screwed. So error
    return(error);
}

// NOTE: Need to Fix Like LBA.
WORD GetRealLastCHS(Drive *d,
                    DWORD *RealCyl,
                    DWORD *RealHead,
                    DWORD *RealSec,
                    int ClearController)
{
    // CHS Mode - We can potentially have hidden Cylinders on the disk. Seek to
    // the end of the disk, and find out what the last cynlinder is.
    // this is essentially required because some BIOS disk configurations dont
    // report hidden cylinders through calls to INT13/AH=08h and INT13/AH=48h
    WORD RealLastCyl = 0;
    WORD RealLastHead = 0;
    WORD RealLastSector = 1;
    WORD CylJump = 100;
    WORD HeadJump = 10;
    WORD SectJump = 10;
    WORD error;
    char *buffer = farmalloc(STD_BYTES_PER_SECTOR);

    *RealCyl = 0;
    *RealHead = 0;
    *RealSec = 0;

    if (ClearController)
        ResetController(d);

#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,21,1,"Drive Detection begin for %d...", d->diskId);
#endif
    error = StandardRead(d->diskId,
                         RealLastCyl,
                         RealLastHead,
                         RealLastSector,
                         1,
                         buffer);

    while (error == 0)
    {
#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,22,1,E_DEBUG,"Scanning H:%d C:%d S:%d Jump Cyl:%d ...", RealLastHead, RealLastCyl, RealLastSector, CylJump);
   //getch();
#endif
        RealLastCyl += CylJump;

        if (ClearController)
            ResetController(d);

        error = StandardRead(d->diskId,
                             RealLastCyl,
                             RealLastHead,
                             RealLastSector,
                             1,
                             buffer);

        if (error != 0) // CHS Seek not found
        {
            if (CylJump > 1)
            {
                RealLastCyl -= CylJump;
                CylJump /= 10;
                // Reset error flag - this ensures that we try again
                error = 0;
            }
            else
            {
                *RealCyl = RealLastCyl--;
            }
        }
        else
        {
            if (RealLastCyl > MAX_STD_CYLINDERS)
            {
                // This has been found in some testing of scsi drives, where
                // the controller has been configured for normal when disk > 1GB
                // and has more than the max number of cylinders possible using
                // INT13/0x calls
                //*RealCyl = RealLastCyl = MAX_STD_CYLINDERS + 1;
                //break;

                // Jan 2007 - JB - On an HP vectra, jumping 100 cylinders at a time and
                // going past 1023 never results in an error -- in other words, it'll
                // just keep jumping forever. So we should return an error which ensures
                // that the bios queried cylinder value will be used. We can detect if
                // the cylinder jump is going on 'forever' by checking for an overflow.
                // The code above I commented out as it shouldn't be necessary -- apparently,
                // the BIOS queried values can be trusted for SCSI drives and so physical
                // detection shouldn't be necessary.
                return 1;
            }
        }
    }

#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,21,1,"Scanning pass 2");
#endif
    // Reset error flag - this ensures that we try again
    error = 0;
    while (error == 0)
    {
        RealLastHead += HeadJump;
#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,22,1,"Scanning H:%d C:%d S:%d Jump Head:%d ...", RealLastHead, RealLastCyl, RealLastSector, HeadJump);
#endif

        if (ClearController)
            ResetController(d);

        error = StandardRead(d->diskId,
                             RealLastCyl,
                             RealLastHead,
                             RealLastSector,
                             1,
                             buffer);

        if (error != 0) // CHS Seek not found
        {
            if (HeadJump > 1)
            {
                RealLastHead -= HeadJump;
                HeadJump /= 10;
                // Reset error flag - this ensures that we try again
                error = 0;
            }
            else
            {
                *RealHead = RealLastHead--;
            }
        }
        else
        {
            if (RealLastHead >= d->queriedConfig.Heads)
            {
                // This has been found in some testing of scsi drives, where
                // the controller has been configured for normal when disk > 1GB
                // and has more than the max number of cylinders possible using
                // INT13/0x call. Never Go Above Queried Max Heads
                *RealHead = RealLastHead = (WORD)d->queriedConfig.Heads;
                break;
            }
        }
    }
#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,21,1,"Scanning pass 3");
#endif

    // Reset error flag - this ensures that we try again
    error = 0;
    while (error == 0)
    {
        RealLastSector += SectJump;
#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,22,1,"Scanning H:%d C:%d S:%d Jump Sect:%d ...", RealLastHead, RealLastCyl, RealLastSector, SectJump);
#endif

        if (ClearController)
            ResetController(d);

        error = StandardRead(d->diskId,
                             RealLastCyl,
                             RealLastHead,
                             RealLastSector,
                             1,
                             buffer);

        if (error != 0) // CHS Seek not found
        {
            if (SectJump > 1)
            {
                RealLastSector -= SectJump;
                SectJump /= 10;
                // Reset error flag - this ensures that we try again
                error = 0;
            }
            else
            {
                *RealSec = --RealLastSector;
            }
        }
        else
        {
            if (RealLastSector >= d->queriedConfig.Sectors)
            {
                // This has been found in some testing of scsi drives, where
                // the controller has been configured for normal when disk > 1GB
                // and has more than the max number of cylinders possible using
                // INT13/0x calls
                *RealSec = RealLastSector = (WORD)d->queriedConfig.Sectors;
                break;
            }
        }
    }

#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,21,1,"Now Verify");
#endif
    if (!ClearController)
        return(VerifyLastCHS(d, RealCyl, RealHead, RealSec));
#if defined(DISKWIPE_ULTRAWIPE_DEBUG)
   txtline(1,21,1,"Verify finished");
#endif
    farfree(buffer);
    return(0);
}

WORD CalculateDiskParams(Drive* d)
{
    WORD error = 0;

    if (d->accessMode == ACCESS_MODE_ATADRVR || d->accessMode == ACCESS_MODE_EXTENDED) {
        if (d->type == DISK_TYPE_IDE) {
            // If it's an IDE disk, parameter calculation has already been
            // done in the BIOS detection routine (by using an ATA IDENTIFY DEVICE command
            // rather than the config info the BIOS gives us.
        } else {
            d->queriedConfig.totalSectorsHi = d->extParams.totalsectors.upperword;
            d->queriedConfig.totalSectorsLo = d->extParams.totalsectors.lowerword;
            if (d->extParams.info & 0x02) { // If CHS information in EDP block is valid
               d->queriedConfig.Heads = d->realConfig.Heads = d->extParams.heads;
               d->queriedConfig.Sectors = d->realConfig.Sectors = d->extParams.sectorstrack;
            } else {
               d->queriedConfig.Heads = d->realConfig.Heads = d->stdParams.heads + 1;
               d->queriedConfig.Sectors = d->realConfig.Sectors = d->stdParams.spt;
            }
        }
        d->queriedConfig.Cylinders = COMBINE_LBA(d->queriedConfig.totalSectorsHi,d->queriedConfig.totalSectorsLo) / (double)d->queriedConfig.Heads / (double)d->queriedConfig.Sectors;
        d->queriedConfig.DiskSize = COMBINE_LBA(d->queriedConfig.totalSectorsHi,d->queriedConfig.totalSectorsLo) * (double)d->bps / 1024.0 / 1024.0;

        // Added 22/5/2002 - ST - If Total Disk Size is > 8GB, dont do physical check
        // Changed 13/6/2002 - ST - If Marked SCSI, dont do physical check
        // Changed 22/12/2006 - JB - reversed comparison for clarity
        if (d->type == DISK_TYPE_SCSI || forceSCSI == 2) {
            d->realConfig.totalSectorsHi = d->queriedConfig.totalSectorsHi;
            d->realConfig.totalSectorsLo = d->queriedConfig.totalSectorsLo;
        } else {
            error = GetRealLastSector(d, &d->realConfig.totalSectorsHi, &d->realConfig.totalSectorsLo, 0);
            if (error == 0xAA) {
               // Added January 2007 - JB - An HP Vectra XU 5/133c was near-randomly returning with errors AX=04h
               // (sector out of range) and AX=AAh (drive not ready) from int 13 calls using extended mode.
               // If this happens, just drop back to CHS mode.
               // !!! we may want to impose a size constraint on this too. Using CHS mode on drives >8gb means
               // that not everything will be wiped.
               return error;
            } else if (error == 0x04) {
               // The same HP machine performed very differently when two drives were attached to the
               // IDE port -- there would be no 0xAA, but the same 0x04 error would occur causing the
               // physically-detected size of the drive to be grossly inaccurate (the 3gb Fujitsu MPC was
               // detected as 588mb, while a 2gb Seagate ST32122A was detected as 8gb). This was detected
               // by reading a few more jumps after getting an 0x04 error. If any subsequent read *didn't*
               // return 0x04, we know the controller is stuffed.
               // !!! Again, reverting to CHS mode may require a size constraint. On the HP machine I was
               // testing with, LBA reads would not complete (wiping progressed extremely slowly, I assume
               // they were erroring and timing out). I think it's safe to say that if a computer has
               // a controller as broken as this HP machine's, then it's old enough to have drives for which
               // CHS won't be a limitation.
               d->queriedConfig.PhysicalDetectError = 1;
               memcpy(&d->realConfig, &d->queriedConfig, sizeof(DiskConfig));
               return error; // Return an error so we can revert to CHS.
            } else if (error) {
               // Added 13/5/2002 - ST - Because of a problem at Defense - if we get an error during the Physical
               // Detection, assume there is some issue with it, and just use the parameters out of the BIOS.
               d->queriedConfig.PhysicalDetectError = 1;
               //QueriedConfig.Cylinders = stddiskparam.totalcyl + 1;
               memcpy(&d->realConfig, &d->queriedConfig, sizeof(DiskConfig));
               error = 0;
            }
        }
        d->realConfig.Cylinders = COMBINE_LBA(d->realConfig.totalSectorsHi,d->realConfig.totalSectorsLo) / (double)d->realConfig.Heads / (double)d->realConfig.Sectors;
        d->realConfig.DiskSize = COMBINE_LBA(d->realConfig.totalSectorsHi,d->realConfig.totalSectorsLo) * (double)d->bps / 1024.0 / 1024.0;

    } else {
        // No Int13 Extensions installed-> Use the standard disk information
        if (d->type != DISK_TYPE_FLOPPY) {
            d->queriedConfig.Cylinders = d->stdParams.cylinders + 2; // add 2 - see int13/ah=0x08h
        } else {
            // Amended 12/11/2001 - ST - Only add 1 for all Standard disks. Any hidden
            // cylinders will be picked up in the Detection mechanism
            d->queriedConfig.Cylinders = d->stdParams.cylinders + 1;
        }
        d->queriedConfig.Heads = d->stdParams.heads + 1;
        d->queriedConfig.Sectors = d->stdParams.spt;

        // Sectors play no real part in CHS mode - so just blank them
        d->queriedConfig.totalSectorsHi = 0;
        d->queriedConfig.totalSectorsLo = d->queriedConfig.Cylinders * d->queriedConfig.Heads * d->queriedConfig.Sectors;
        d->queriedConfig.DiskSize = (double)d->queriedConfig.totalSectorsLo * (double)d->bps / 1024.0 / 1024.0;

        if (d->type != DISK_TYPE_FLOPPY) {
            // Added 22/5/2002 - ST - If Total Disk Size is > 8GB, dont do physical check
            if (d->queriedConfig.DiskSize < 8192) {
                error = GetRealLastCHS(d,
                                       &d->realConfig.Cylinders,
                                       &d->realConfig.Heads,
                                       &d->realConfig.Sectors,
                                       1);
               if (error) {
                  // Added 13/5/2002 - ST - Because of a problem at Defense - if we get an error during the Physical
                  // Detection, assume there is some issue with it, and just use the parameters out of the BIOS.
                  d->queriedConfig.PhysicalDetectError = 1;
                  memcpy(&d->realConfig, &d->queriedConfig, sizeof(DiskConfig));
                  error = 0;
               }
            } else {
               memcpy(&d->realConfig, &d->queriedConfig, sizeof(DiskConfig));
            }
        } else {
            memcpy(&d->realConfig, &d->queriedConfig, sizeof(DiskConfig));
        }

        // This gets a little hairy with CHS, because if we dont have complete Heads and Cylinders
        // in Final CHS figures, then you have to do part calculations.
        if (d->realConfig.Cylinders == d->queriedConfig.Cylinders &&
            d->realConfig.Heads == d->queriedConfig.Heads &&
            d->realConfig.Sectors == d->queriedConfig.Sectors)
        {
            d->realConfig.totalSectorsHi = 0;
            d->realConfig.totalSectorsLo = d->realConfig.Cylinders * d->realConfig.Heads * d->realConfig.Sectors;
        } else {
            // Calculate the total disk size
            d->realConfig.totalSectorsHi = 0;
            d->realConfig.totalSectorsLo = d->realConfig.Sectors;

            if (d->realConfig.Sectors != d->queriedConfig.Sectors)
            {
                // incomplete last head/sector
                d->realConfig.totalSectorsLo += (d->queriedConfig.Sectors * (d->realConfig.Heads - 2));
            }
            else
            {
                d->realConfig.totalSectorsLo += (d->queriedConfig.Sectors * (d->realConfig.Heads - 1));
            }

            if (d->realConfig.Heads != d->queriedConfig.Heads)
            {
                // incomplete Last Cylinder/Track
                d->realConfig.totalSectorsLo += ((d->realConfig.Cylinders - 2) * d->queriedConfig.Heads * d->queriedConfig.Sectors);
            }
            else
            {
                d->realConfig.totalSectorsLo += ((d->realConfig.Cylinders - 1) * d->queriedConfig.Heads * d->queriedConfig.Sectors);
            }

        }

        // Partial last cylinder
        if (d->queriedConfig.Cylinders == d->realConfig.Cylinders &&
            d->queriedConfig.Heads > d->realConfig.Heads &&
            d->queriedConfig.Sectors > d->realConfig.Sectors)
            d->queriedConfig.Cylinders--;

        // Sectors play no real part in CHS mode - so just blank them
        d->queriedConfig.totalSectorsHi = 0;
        d->queriedConfig.totalSectorsLo = d->queriedConfig.Cylinders * d->queriedConfig.Heads * d->queriedConfig.Sectors;

        d->queriedConfig.DiskSize = (double)d->queriedConfig.totalSectorsLo * (double)d->bps / 1024.0 / 1024.0;
        d->realConfig.DiskSize = (double)d->realConfig.totalSectorsLo * (double)d->bps / 1024.0 / 1024.0;
    }
    return(0);
}
