#ifndef DISKWIPE_TYPES
#define DISKWIPE_TYPES

#include <dos.h>

// Define Interrupt Data Types
typedef unsigned char   BYTE;             // 8 Bit Unsigned Value
typedef unsigned int    WORD;             // 16 Bit Unsigned Value
typedef unsigned long   DWORD;            // 32 Bit Unsigned Value
typedef unsigned char byte;
typedef unsigned int word;
typedef unsigned long dword;

typedef struct _QWORD               // 64 Bit Unsigned Value
{                                   // Actual Value should be calculated as
    DWORD lowerword;                // upperword + lowerword * 2^32 - 1
    DWORD upperword;                // although upperword is almost always 0
} QWORD;

// Structure Definitions
typedef struct
{
    WORD currentsize;
    WORD allocatedsize;
    char *dataptr;
} memorybuf;

typedef struct
{
    DWORD Cylinders;
    DWORD Heads;
    DWORD Sectors;
    DWORD totalSectorsHi;
    DWORD totalSectorsLo;
    double DiskSize;
    BYTE PhysicalDetectError;
    BYTE DMA_Mode;
} DiskConfig;

typedef struct
{
   struct time starttime;
   struct date startdate;
   struct time endtime;
   struct date enddate;
   int writeerror[3];
   int readerror[3];
   int verifyerror[3];
   int writeretry[3];
   int readretry[3];
   int verifyretry[3];
   int haserror;
} WipeProcessInfo;

typedef struct _ExtTransferBuffer
{
    BYTE    sz;
    BYTE    reserved;
    WORD    blocks;
    char   *buffer;
    QWORD   startblock;
} ExtTransferBuffer;

typedef struct _StdDiskParam
{
    WORD cylinders;
    WORD spt;
    WORD heads;
    WORD totaldrives;
} StdDiskParam;

typedef struct _ExtDiskParam
{
    WORD  sz;
    WORD  info;
    DWORD cyls;
    DWORD heads;
    DWORD sectorstrack;
    QWORD totalsectors;
    WORD  bytespersector;
    DWORD edd;
    WORD  isdevpath;
    BYTE  sdevpath;
    BYTE  resdev[3];
    BYTE  hostbus[4];
    BYTE  interfacetype[8];
    BYTE  intpath[8];
    BYTE  devpath[8];
    BYTE  resnull;
    BYTE  chksum;
    BYTE  reserved[256];   // ensure we don't get a buffer overflow;
    BYTE  versionsupported;
} ExtDiskParam;

typedef struct _ExtensionInfo
{
    int extensioninstalled;
    WORD BXregister;
    BYTE majorversion;
    WORD apisupportbitmap;
    BYTE extensionversion;
} ExtensionInfo;

typedef struct _MyRegPackWordRegs
{
    WORD ax;
    WORD bx;
    WORD cx;
    WORD dx;
    WORD bp;
    WORD si;
    WORD di;
    WORD ds;
    WORD es;
    WORD flags;
} MyRegPackWordRegs;

typedef struct _MyRegPackByteRegs
{
    BYTE al;
    BYTE ah;
    BYTE bl;
    BYTE bh;
    BYTE cl;
    BYTE ch;
    BYTE dl;
    BYTE dh;
    WORD bp;
    WORD si;
    WORD di;
    WORD ds;
    WORD es;
    WORD flags;
} MyRegPackByteRegs;


typedef union _MyRegPack
{
    struct REGPACK regs;
    MyRegPackWordRegs x;
    MyRegPackByteRegs h;
} MyRegPack;

















typedef struct IDEChannelInfo {
   int pci_bus;
   int pci_device;
   int pci_function;
   int mode; // compatibility or native
   WORD bmcr; // bus master control register
   WORD bmaddr; // BM i/o address: bmcr+2 for primary, bmcr+10 for secondary.
   WORD command_reg;
   WORD control_reg;
   int irq;
} IDEChannel;

typedef struct DriveInfo {
   int type;
   int accessMode;

   //unsigned long lbahi;
   //unsigned long lbalo;

   int bps;

   int diskId;
   StdDiskParam stdParams;
   ExtDiskParam extParams;
   ExtensionInfo extInfo;
   WipeProcessInfo wipeInfo;
   DiskConfig queriedConfig;
   DiskConfig realConfig;

   union {
      struct {
         char model[41];
         char serial_no[21];
         char firmware[9];
         int device; // 0=master, 1=slave
	      int max_udma_mode;
	      int selected_udma_mode;
         int supports_lba48;

         WORD command_reg;
         WORD control_reg;
         WORD bmcr;
         WORD bmaddr;
         int irq;
      } ide;

      struct {
         char pad1;
      } floppy;

      struct {
         char pad2;
      } scsi;
   };

} Drive;

typedef struct ATA_IDENTIFY_DEVICE_Infos
{
   WORD gen_config;       // 0
   WORD log_cyls;			  // 2
   WORD reserved;         // 4
   WORD log_heads;        // 6
   WORD vs_bpt;           // 8
   WORD vs_bps;           // 10
   WORD log_sectors;      // 12
   WORD vs[3];             // 14
   char serialno[20];     // 20
   WORD vs_buftype;       // 40
   WORD cont_buf_size;    // 42
   WORD vs_ECC;           // 44
   char firmware[8];      // 46
   char model[40];        // 54
   WORD multrw;           // 94
   WORD dwordtran;        // 96
   WORD capabilities;     // 98
   WORD secmode;          // 100
   WORD PIOmode;          // 102
   WORD DMAmode;		     // 104
   WORD fldvalid;         // 106
   WORD cyls;             // 108
   WORD heads;            // 110
   WORD sectors;          // 112
   DWORD capsectors;      // 114
   WORD mss;              // 118
   DWORD totalsectors;    // 120
   WORD sDMAmode;         // 124
   WORD mDMAmode;         // 126
   WORD flowPIO;          // 128
   WORD minDMATrans;      // 130
   WORD mDMAcycle;        // 132
   WORD nfPIO;            // 134
   WORD minPIO;           // 136
   WORD res_1[2];        // 138
   WORD typetime[2];      // 142
   WORD res_2[2];           // 146
   WORD qdepth;           // 150
   WORD satacap;          // 152
   WORD res_3;            // 154
   WORD satafsup;         // 156
   WORD satafenbl;        // 158
   WORD major_revnum;     // 160
   WORD minor_revnum;     // 162
   WORD fs_1;             // 164
   WORD fs_2;             // 166
   WORD fs_3;             // 168
   WORD fe_1;             // 170
   WORD fe_2;             // 172
   WORD fe_3;             // 174
   WORD udma;             // 176
   WORD erasetime;        // 178
   WORD erasetimex;       // 180
   WORD adv_pwr_mgmt;      // 182
   WORD master_pwd_rev;    // 184
   WORD hardware_reset;    // 186
   WORD acoustic_mgmt;     // 188
   WORD stream_min_sz;     // 190
   WORD stream_xfer_d;     // 192
   WORD stream_lat;        // 194
   WORD stream_perf[2];    // 196
   DWORD lba48_totalsectors[2]; // 200
   char resbuf[49];       // 208 - 256
} ATA_IDENTIFY_DEVICE_Info;


#endif
