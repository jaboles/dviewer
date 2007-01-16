#ifndef GLOBALS_H_INCLUDED
#define GLOBALS_H_INCLUDED

extern IDEChannel far *ideChannels;
extern Drive far *ideDriveList;
extern Drive far *biosDriveList;
extern Drive far *floppyDriveList;
extern Drive far *driveList;
extern int NumberOfIDEDrives;
extern int NumberOfBIOSDrives;
extern int NumberOfFloppyDrives;

extern int forceSCSI;

#endif
