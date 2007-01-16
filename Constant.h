#ifndef DISKWIPE_CONSTANTS
#define DISKWIPE_CONSTANTS

#define MAXSTR 255

#include "types.h"

//Some constants for various numbers
extern const BYTE MAX_BYTE;
extern const WORD MAX_WORD;
extern const DWORD MAX_DWORD;

#define STD_BYTES_PER_SECTOR 512
#define MAX_STD_SECTORS 63
#define MAX_STD_HEADS 255
#define MAX_STD_CYLINDERS 1023

#define DISK_TYPE_FLOPPY 1
#define DISK_TYPE_IDE 2
#define DISK_TYPE_SCSI 3
#define DISK_TYPE_UNKNOWN_HDD 4
#define ACCESS_MODE_STANDARD 1
#define ACCESS_MODE_EXTENDED 2
#define ACCESS_MODE_ATADRVR 3

#define BUFFER_SIZE 262144L
#define MAX_HDD_LIST_SIZE 20
#define MAX_FDD_LIST_SIZE 2
#define MAX_HDD_DISPLAY_LIST_SIZE 4
#define MAX_WIPE_RESULTS_DISPLAY_LIST_SIZE 4

extern const WORD FLOPPY_DISK_A;
extern const WORD FLOPPY_DISK_B;
extern const WORD INT13_41H_BX;
extern const WORD INT13_41H_BX_SWAPPED;

extern WORD MAX_EXT_TRANSFER_BLOCKS;
extern const WORD BYTES_PER_K;

// Interrupt Numbers
extern const int INT13;

// Int13 Services
extern const BYTE INT13_CONTROLLER_RESET;
extern const BYTE INT13_STD_DISK_READ;
extern const BYTE INT13_STD_DISK_WRITE;
extern const BYTE INT13_STD_GET_DISK_PARAMS;
extern const BYTE INT13_STD_DISK_SEEK;
extern const BYTE INT13_EXT_INSTALL_CHECK;
extern const BYTE INT13_EXT_DISK_READ;
extern const BYTE INT13_EXT_DISK_WRITE;
extern const BYTE INT13_EXT_DISK_SEEK;
extern const BYTE INT13_EXT_GET_DISK_PARAMS;
extern const BYTE INT13_IDENTIFY_DRIVE;

// Some Ascii Code for some "Pretty" Borders
extern const int TOP_LEFT;
extern const int TOP_RIGHT;
extern const int BOTTOM_LEFT;
extern const int BOTTOM_RIGHT;
extern const int HORIZONTAL_BAR;
extern const int VERTICAL_BAR;
extern const int CROSSING;
extern const int LEFT_T;
extern const int RIGHT_T;
extern const int TOP_T;
extern const int BOTTOM_T;

extern const int DOUBLE_VERTICAL;
extern const int DOUBLE_HORIZONTAL;
extern const int DOUBLE_BOTTOM_T;
extern const int DOUBLE_TOP_T;

extern const int F1KEY;
extern const int F2KEY;
extern const int F3KEY;
extern const int F4KEY;
extern const int F5KEY;
extern const int F6KEY;
extern const int F7KEY;
extern const int F8KEY;
extern const int F9KEY;
extern const int F10KEY;

extern const int UPARR;
extern const int PGUPKEY;
extern const int DOWNARR;
extern const int PGDNKEY;
extern const int CTRLPGDN;
extern const int CTRLPGUP;
extern const int CONTROL_F;

extern const int ENTER_KEY;
extern const int ESCAPE_KEY;

extern const DWORD UPDATE_THRESHHOLD;

#define DISKREAD_DETECTFAIL 666


#define MAX_SEARCH_RESULTS 1000
#define SMALL_BUFFER_SIZE 20480L

#define INT13_41H_BX 0x55AA
#define INT13_41H_BX_SWAPPED 0xAA55


// Interrupt Numbers
#define INT13                          0x13

// Int13 Services
#define INT13_CONTROLLER_RESET 		0x00
#define INT13_STD_DISK_READ      	0x02
#define INT13_STD_DISK_WRITE			0x03
#define INT13_STD_GET_DISK_PARAMS   0x08
#define INT13_STD_DISK_SEEK			0x0C
#define INT13_EXT_INSTALL_CHECK		0x41
#define INT13_EXT_DISK_READ			0x42
#define INT13_EXT_DISK_WRITE			0x43
#define INT13_EXT_DISK_SEEK			0x47
#define INT13_EXT_GET_DISK_PARAMS	0x48



#endif