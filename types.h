#ifndef DISKWIPE_TYPES
#define DISKWIPE_TYPES

#include <dos.h>

// Define Interrupt Data Types
typedef unsigned char   BYTE;             // 8 Bit Unsigned Value
typedef unsigned int    WORD;             // 16 Bit Unsigned Value
typedef unsigned long   DWORD;            // 32 Bit Unsigned Value
typedef unsigned char byte;     /* 8-bit  */
typedef unsigned short word;    /* 16-bit */
typedef unsigned long dword;    /* 32-bit */

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
   int diskId;
   
   
   int cylinders;
   int heads;
   int spt;
   unsigned long lbahi;
   unsigned long lbalo;

   int bps;

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
