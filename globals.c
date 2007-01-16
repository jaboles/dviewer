#include "types.h"

IDEChannel far *ideChannels;
Drive far *ideDriveList;
Drive far *biosDriveList;
Drive far *floppyDriveList;
Drive far *driveList;
int NumberOfIDEDrives;
int NumberOfBIOSDrives;
int NumberOfFloppyDrives;

int forceSCSI;
