#include "types.h"
#include <stdarg.h>

#ifndef UIUTILS_H_INCLUDED
#define UIUTILS_H_INCLUDED

#define NORM   0              /* Used all over the place                */
#define TRUE   1
#define FALSE  0

#define ADD    0
#define DEC    1
#define EDIT   2
#define DEL    3

#define MAXSTR 255            /* Global string buffer size              */

/* box() - style */
#define DOUBLE 0              /* Box styles                             */
#define SINGLE 1
#define THICK  2
#define NONE   3

/* box() - mode */
#define NOCLEAR 1             /* When drawing a box don't clear inside  */
#define EXPLODE 2             /* Draw an exploding box                  */
#define CARD    3             /* Print a single line across top         */

/* textline() - mode */
#define CTR     1             /* Centre the text based on the  x value  */
#define CLE     2             /* Clear to the end of line (textline)    */
#define CTE     3             /* Centre and clear eol                   */
#define ECTR    4             /* Centre and clear eol                   */

typedef int colortable[7];

extern colortable  coltable;
extern colortable  ocoltable;
extern word        EXPWAIT;   /* Num of millsecs to delay in box draw   */
extern word        DESTWAIT;  /* Num of millsecs to delay in box destroy*/
extern colortable  start, mainc, menuc, loc, dentry, err,  boxs, listc;

#define HIGH  1               /* 1 - Highlighted text                   */
#define BOXL  2               /* 2 - Box line                           */
#define BOXI  3               /* 3 - Box inside                         */
#define BAR   4               /* 4 - Menu Bar                           */
#define NA    5               /* 5 - Not in use text                    */
#define OTHER 6               /* 6 - Any other use that may be needed   */


/* Screen defines */

typedef struct scrbuf         /* savescreen()                           */
{
   word x1,y1,x2,y2;          /* window location                        */
   word handle;               /* handle assigned to this struc          */
   char *sbuffer;             /* pointer to memory buffer               */
   struct scrbuf *next;       /* pointer to the next struc              */
} sbuf;

typedef struct mlst           /* msg()                                  */
{
   word handle;               /* handle assign to this structure        */
   word x1 , y1, x2, y2;      /* box coords                             */
   word style, mode, shadow;  /* box styles                             */
   struct mlst *next;         /* pointer to next message                */
} msglist;

typedef struct lstptr         /* prompt(), list()                       */
{
   char *option;              /* string option                          */
   char *hlp;                 /* status line help text                  */
   char mode;                 /* display/action mode                    */
   word itnum;                /* internal current item no.              */
   struct lstptr *prev;       /* pointer to previous item in list       */
   struct lstptr *next;       /* next item in list                      */
} lists;

typedef struct inpptr         /* get(), entry()                         */
{
   word x;                    /* position of get                        */
   word y;
   word len;                  /* length of text to be input             */
   word nitem;                /* number of items (used in choice)       */
   char *format;              /* format of variable (type, width etc.)  */
   char *chvar;               /* Choice buffer, used only in choice     */
   void *arg1;                /* variable to display and return         */
   char *value;               /* text format of variable                */
   void *valfnt;              /* valid funtion pointer                  */
   struct inpptr *prev;       /* previous get                           */
   struct inpptr *next;       /* next get in list                       */
} inputs;




extern sbuf *scptr;           /* general screen save ptr                */
extern sbuf *scfirst;         /* used by the save/return screen rtns    */

/* Key defines */

#define  NOKEY    0          /* ASCII keyboard values returned rkbd()  */

#define  F1KEY    59
#define  F2KEY    60
#define  F3KEY    61
#define  F4KEY    62
#define  F5KEY    63
#define  F6KEY    64
#define  F7KEY    65
#define  F8KEY    66
#define  F9KEY    67
#define  F10KEY   68

#define  SFF1     84
#define  SFF2     85
#define  SFF3     86
#define  SFF4     87
#define  SFF5     88
#define  SFF6     89
#define  SFF7     90
#define  SFF8     91
#define  SFF9     92
#define  SFF10    93

#define  CTRLF1   94
#define  CTRLF2   95
#define  CTRLF3   96
#define  CTRLF4   97
#define  CTRLF5   98
#define  CTRLF6   99
#define  CTRLF7   100
#define  CTRLF8   101
#define  CTRLF9   102
#define  CTRLF10  103

#define  ALTF1    94
#define  ALTF2    95
#define  ALTF3    96
#define  ALTF4    97
#define  ALTF5    98
#define  ALTF6    99
#define  ALTF7    100
#define  ALTF8    101
#define  ALTF9    102
#define  ALTF10   103

#define  HOMEKEY   71
#define  ENDKEY    79
#define  UPARR     72
#define  DOWNARR   80
#define  LEFTARR   75
#define  RIGHTARR  77
#define  PGDNKEY   81
#define  PGUPKEY   73
#define  CTRLEFT   115
#define  CTRLRT    116
#define  CTRLPGUP  132
#define  CTRLPGDN  118
#define  CTRLHOME  119
#define  CTRLEND   117
#define  INSKEY    82
#define  DELKEY    83
#define  ESCKEY    27
#define  BACKSP    8
#define  TAB       9
#define  BACKTAB   15
#define  ENTERKEY  13

#define  ALT1      120
#define  ALT2      121
#define  ALT3      122
#define  ALT4      123
#define  ALT5      124
#define  ALT6      125
#define  ALT7      126
#define  ALT8      127
#define  ALT9      128


extern char ch;           /* Standard key value,     Routine: rkbd()      */
extern char extend;       /* Extended key value,            : rkbd()      */
extern char ctrl_brk;     /* Allow ctrl_brk to halt         : rkbd()      */
extern word insx;         /* Pos of ins/ovr mess            : setins()    */
extern word insy;         /* as above                                     */
extern word insc;         /* Colour array position (0-5)    : setins()    */
extern char ins_mode;     /* Set current ins mode (T/F)     : setins()    */
extern char show_ins;     /* Show the ins.ovr message       : setins()    */
extern char insmess[15];  /* Ins/Ovr messages               : setins()    */
extern char ovrmess[15];
extern char und_score;    /* Show space as "_"              : keyin()     */
extern word oldcursor,    /* Cursor mode                    : setcursor() */
            normcursor,
            nocursor;

/* defines for list */

#define FWD   0           /* used to indicate direction to move in list   */
#define BWD   1

#define UP    0           /* used to indicate type of move in list        */
#define DOWN  1
#define LEFT  2
#define RIGHT 3
#define PGUP  4
#define PGDN  5

#define DISPLAY 1         /* type of list to process                      */
#define POPUP   2
#define KEYEXIT 3
#define NOSET 32000
#define ALTMODE 4
#define ALTRET 32000  /* Indicates alt x pressed Should be beyond normal range. */
#define NOCOL  8
#define NOROLL 9

extern char listvar[MAXSTR];    /* Points to the selected var on ret from list  */
typedef char opts[70];


char usrvalid(word valopt);     /* This is used to access a valid function */
                                /* which must exist if a valid is required */

extern char found;

/* Reserved handles for processing */

/* Note: handles 200 thru 255 should not be used by applications */

#define HELPHANDLE 250
#define CHOICEHAN  251
#define SYSERRHAN  252
#define DBERRHAN   253
#define ASKHANDLE  254


extern int dtx,dty;

extern char dir;

extern char listcheck[255];
extern int  lretv;
extern char buffer[MAXSTR];


typedef struct chstruct
{
   word x1, x2, y1, y2;
   word style,mode;
   char shadow;
   opts *option;
   opts *desc;
   word *retval;
} chstruct;

typedef struct
{
   char type;
   word width;
   word dec;
   char form;
   char mode[10];
   char neg;
   char zero;
   char just;
   char valid[26];
   word ranlo;
   word ranhi;
} iformat;






static void border(word x1, word y1, word x2, word y2,
            word style, word mode, word shadow);
void boxdestruct(word handle,
                 word x1, word y1, word x2, word y2,
                 word style, word shadow);
static void boxexplode(word x1, word y1, word x2, word y2,
                word style, word mode, word shadow);
static word centre(word pos, char output[]);
void chinsert(char *substr1, char *str1, word ps);
void choice(word x1, word y1, word x2, word y2,
            word style, word mode, char shadow,
            word xpos, word ypos, word numitems,
            opts *option, opts *desc,
            word *retval);
void cla(word x, word y, word width, word col);
void clrs(word x1, word y1, word x2, word y2);
int colcode(char *col);
static char *comma(char *str1);
void dbox(word x1, word y1, word x2, word y2,
          word style, word mode, word shadow);
void defcolor(colortable newtab, char *nc, char *hc, char *bl, char *bi, char *bc, char *na, char *ot);
void delete(char *str1, word index, word num);
char derr(char *str1);
char *dirlist(word x1, word y1, word x2, word y2,
              word style, word mode, word shadow, word dirtype);
void doprompt(void);
int entry(char mode);
void errbeep(void);
void help(void);
static iformat evalform(char *format);
static void init_entry(void);
void inschr(char substr1, char *str1, word ps);
void fatal(char *head, char *msg);
sbuf *findptr(word handle);
static void frmstr(char *str);
static char *ftos(double num, int dec);
void get(word x, word y, word len, char far *format, void far *arg1, void far *valfnt);
void getchoice(void);
static void getdata(void);
char *intonly(char *str1);
char *justfy(void);
char *keyin(char *line, word len, word max, word colr, char form[11]);
void killitem(sbuf *scpntr);
void killmsg(word handle);
word list(word x1, word y1, word x2, word y2,
          word style, word nocols, word offset, word first);
char *ltrim(char *str1);
void msg(word handle,
         word x1, word y1, word x2, word y2,
         word style, word mode, word shadow,
         char *head, word hpos,char *ms);
void notice(word handle,                         /* Handle used to access msg  */
            word x1, word y1, word x2, word y2,  /* Box co-ords                */
            word style, word mode, word shadow,  /* Box style                  */
            const char far *head, word hpos,               /* Header & position from top */
            const char far *ms); //,...);                        /* The message format & info  */
char *numonly(char *str1);
static word partdate(char *dstr, word st);
static void play(word freq, word dur);
word pos(char *substr1, char *str1);
void prompt(char *opt, char mode, char *hlp);
char *rep(char ch, word num);
void return_status(void);
void returnscreen(word hand, char destroy);
void returnwindow(void);
void rkbd(void);
void savescreen(word handle, word x1, word y1, word x2, word y2);
void savemsg(word handle,
             word x1, word y1, word x2, word y2,
             word style, word mode, word shadow);
void save_status(void);
void setcol(colortable newone);
void setcursor(word shape);
void setins(word x, word y, word col, char show, char ins, char *im, char *om);
void setwindows(word x1, word y1, word x2, word y2);
static void show_entry(void);
static void showinput(void);
char *space(word num);
char *strip(char *str1);
char *substr(char *str1, word st, word num);
void textline(word x, word y, word mode, word colr, va_list items,...);
char *trim(char *str1);
void warbeep(void);
char *wordonly(char *str1);
static void zapinput(void);

#define cval(fore,back) (fore+back*16)
#define odd(val) (val % 2)
//#define rkbd() (ch = getch())
//#define mid(x1,x2) (x1+((x2-x1)/2))
#define upchar(c) (c>96&&c<123? c-32 : c)
#define beep() play(830,50)
#define altpress() (extend>119 && extend<129)

#endif