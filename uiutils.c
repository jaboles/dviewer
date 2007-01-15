#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <malloc.h>
#include <mem.h>
#include <bios.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include "uiutils.h"

extern char far *helpText[];
extern int helpTextLineCount;


inputs *iptr;
static inputs *ifirst, *ilast = NULL;
static chstruct *cptr;
char (*itf)(void);
static char showzero = FALSE;
static iformat iform;

sbuf far *scpntr   = NULL,
     far *scfrst = NULL,
     far *sclast  = NULL;     /* used by the save/return screen rtns  */
sbuf far *sptr;
msglist far *mptr   = NULL,
        far *mfirst = NULL,
        far *mlast  = NULL;  /* used by message save process         */
struct text_info save_info;     /* used by set/return window      */




struct screen_defin
{
   colortable savecol;
   word       x;
   word       y;
   word       curstype;
} screen_def;





// global variables
colortable        far coltable    = {15,14,15,15,31,7};
colortable        far ocoltable;
//colortable        start, mainc, menuc, menu2c, loc, dentry, err, boxs, listc;
word              EXPWAIT     = 30;
word              DESTWAIT    = 35;
sbuf far *scptr, far *scfirst;     /* used by the save/return screen rtns  */
word              oldcursor,
                  normcursor,
                  nocursor = 0x2000;


char ch         = 255;   /* Global normal character variable                */
char extend     = 0;     /* Global extended character variable              */
char ctrl_brk   = TRUE;  /* This halts the program if ctrl break is pressed */
static word insx = 75;
static word insy = 25;
static word insc = 1;
static char show_ins   = FALSE;
static char insmess[15] = " Ins ";
static char ovrmess[15] = " Ovr ";
char ins_mode   = TRUE;  /* Set insert mode ON;  keyin()                     */
char und_score  = FALSE; /* Show space as "_";   keyin()                     */
char listvar[MAXSTR];
char found;

//filestruc fptr[MAXAREA];
//filestruc genfptr;
int cwork;

char ap[3];              /* am/pm var */
//int dtx = 3,dty = 23;    /* position for the date and time */

char dir;

char listcheck[255];
char buffer[MAXSTR];





static word mid(word x1, word x2)
{
   return(x1+((x2-x1)/2));
}



/****************************************************************************/
/* Choice is the function used to setup an option list in data entry        */
/*                                                                          */
/* x1,y1,x2,y2 : Box size/List size                                         */
/* style       : Type of box eg. single, double etc.                        */
/* mode        : Type of box drawn                                          */
/* shadow      : Show a shadow around the box                               */
/* x           : Position of the option selected                            */
/* y           : ""                                                         */
/* Numitems    : Number of items to display                                 */
/* option      : An array contain the options eg. "First","Second", etc.    */
/* desc        : An array containing a description of each option           */
/* retval      : The position in the array of the option chosen             */
/*                                                                          */
/* Warning: Each item in the arrays must be the same length otherwise a     */
/*          memory crash may occur. The system checks the first item.       */
/*                                                                          */
/* Return : Nothing                                                         */
/****************************************************************************/

static void border(word x1, word y1, word x2, word y2,
            word style, word mode, word shadow)

{

   char topl[] = {201, 218, 223, 32};    /* double, single, thick, none */
   char topr[] = {187, 191, 220, 32};
   char botl[] = {200, 192, 220, 32};
   char botr[] = {188, 217, 220, 32};
   char horl[] = {205, 196, 223, 32};
   char verl[] = {186, 179, 221, 32};

   char line[MAXSTR];
   char base[2];
   word i, width = x2 - x1;
   struct text_info txt;

   if (mode != NOCLEAR)                    /* If a inside box clear allowed */
   {
      gettextinfo(&txt);                   /* Store current screen detail */
      window(x1,y1,x2,y2);                 /* Set window                  */
      textattr(coltable[BOXI]);            /* Set inside box colour       */
      clrscr();                            /* Clear the window            */
      window(txt.winleft,txt.wintop,txt.winright,txt.winbottom); /* reset */
   }
   //line[width] = '\0';
   textattr(coltable[BOXL]);             /* Set line colour         */
   //strcpy(line,space(1));                /* Force null in so strcat works */
   //strcat(line,rep(horl[style],width));  /* Top horizontal line     */
   memset(line, horl[style], width);

   line[0] = topl[style];                /* Top left corner         */
   line[width] = topr[style];            /* Top right corner        */
   line[width+1] = NULL;                 /* Put end of line char in */
   //printf("%d\n", strlen(line));
   gotoxy(x1,y1);                        /* Position &              */
   cprintf("%s",line);                   /* Write the top line      */
   for (i = y1+1; i < y2; i++)           /* Loop top to bot         */
   {
      gotoxy(x1,i);                      /* Pos & Print left line   */
      cprintf("%c",verl[style]);
      gotoxy(x2,i);                      /* Pos & Print right line   */
      cprintf("%c",verl[style]);
   }
   line[0]     = botl[style];            /* Set base corners        */
   line[width] = NULL;
   gotoxy(x1,y2);                        /* Position &              */
   cprintf("%s",line);                   /* Write the top line      */
   base[0] = botr[style];                /* Move corner to prevent  */
   base[1] = coltable[BOXL];             /* scroll if at 80,25      */
   puttext(x2,y2,x2,y2,base);
   if (shadow)
   {
      for (i = y1+1; i <= y2+1; i++)
         cla(x2+1,i,2,8);
//      cla(x1+2,y2+1,x2-x1+1,8);
      for(i = x1 + 2; i < x2 + 2; i++)
         cla(i, y2 + 1, 2, 8);
   }
   if (mode == CARD)
   {
      textline(x1,y1+1,NORM,BOXL,"%s",rep('Í',x2-x1+1));
      textline(x1,y2-1,NORM,BOXL,"%s",rep('Ä',x2-x1+1));
   }
}

void boxdestruct(word handle,
                 word x1, word y1, word x2, word y2,
                 word style, word shadow)
{
   word vexp = mid(x1,x2) / mid(y1,y2) / 3;         /* Vertical wait              */
   word exp  = vexp;

   sptr = findptr(handle);
   do
   {
      puttext(x1,y1,x2+1,y1,sptr->sbuffer);
      puttext(x1,y1,x1+3,y2+1,sptr->sbuffer);
      puttext(x2-1,y1,x2+3,y2+1,sptr->sbuffer);
      puttext(x1,y2,x2,y2+3,sptr->sbuffer);
      x1 += 3;
      x2 -= 3;
      if (exp == vexp)
      {
         y1++;
         y2--;
      }
      dbox(x1,y1,x2,y2,style,NORM,shadow);
      delay(DESTWAIT);
   } while ((x2-x1 > 3) && (y2-y1 > 1));
   returnscreen(handle,TRUE);
}

static void boxexplode(word x1, word y1, word x2, word y2,
                word style, word mode, word shadow)
{
   word xcent = (x2-x1) / 2;                /* define X centre            */
   word ycent = (y2-y1) / 2;                /* define Y centre            */
   word ex1 = x2 - xcent - 1;               /* define start box positions */
   word ex2 = x2 - xcent + 1;
   word ey1 = y2 - ycent - 1;
   word ey2 = y2 - ycent;
   word hexp = 3;                           /* Horiz expansion amount     */
   word vexp = (xcent / ycent) / 3;         /* Vertical wait              */
   word exp  = 0;
   do
   {
      border(ex1,ey1,ex2,ey2,style,mode,shadow); /* draw the box */
      ex1 -= hexp;                               /* change hor box size */
      ex2 += hexp;
      if (exp++ == vexp)                         /* change ver if ok */
      {
         --ey1;
         ++ey2;
         exp = 0;
      }
      delay(EXPWAIT);      /* delay between new draw, def in global head */
   }
   while ((ex1 >= x1) && (ex2 <= x2) && (ey1 >= y1) && (ey2 <= y2));
   border(x1,y1,x2,y2,style,mode,shadow);
}

static word centre(word pos, char output[])
{
   word centpos;

   centpos = (pos == 1) ? 40 : pos;

   return (centpos - (strlen(output) / 2));
}

void chinsert(char *substr1, char *str1, word ps)
{
   char buffer[MAXSTR];
   word i,j,k;
   word bound = strlen(substr1) + ps - 1;
   word tot   = strlen(substr1) + strlen(str1);


   for (i = j = k = 0; i < tot; i++)
   {
      if (i < ps || i > bound)
         buffer[i] = str1[j++];
      else
         buffer[i] = substr1[k++];
   }
   buffer[tot] = NULL;                /* put a null at the end of the string */
   strcpy(str1,buffer);               /* return it in the source */
}

/*void choice(word x1, word y1, word x2, word y2,
            word style, word mode, char shadow,
            word xpos, word ypos, word numitems,
            opts *option, opts *desc,
            word *retval)
{
   if (coreleft() < 1000)
      fatal("FATAL choice call","Not enough memory available");

   if (ifirst == NULL)
   {
      ifirst = (inputs *) malloc(sizeof(inputs));
      iptr   = ifirst;
      iptr->prev = NULL;
   } else
   {
      iptr->next       = (inputs *) malloc(sizeof(inputs));
      iptr->next->prev = iptr;
      iptr             = iptr->next;
   }
   iptr->next   = NULL;
   ilast        = iptr;

   cptr = (chstruct *) malloc(sizeof(chstruct));
   cptr->x1     = x1;
   cptr->y1     = y1;
   cptr->x2     = x2;
   cptr->y2     = y2;
   cptr->mode   = mode;
   cptr->style  = style;
   cptr->shadow = shadow;
   cptr->option = option;
   cptr->desc   = desc;
   cptr->retval = retval;
   iptr->x      = xpos;
   iptr->y      = ypos;
   iptr->chvar  = (char *) cptr;
   iptr->len    = strlen(option[0]);
   iptr->nitem  = numitems;
   iptr->value = (char *) malloc(iptr->len+1);
   strcpy(iptr->value,space(iptr->len));
   iptr->format = (char *) malloc(3);
   iptr->valfnt = NULL;
   strcpy(iptr->format,"%x");
}*/

void cla(word x, word y, word width, word col)
{
   char buffer[160];
   int i;
   gettext(x,y,x+width,y,buffer);
   for (i=1;i < width * 2; i += 2)
      buffer[i] = col;
   puttext(x,y,x+width,y,buffer);
}

void clrs(word x1, word y1, word x2, word y2)
{
   setwindows(x1,y1,x2,y2);
   clrscr();
   returnwindow();
}

int colcode(char *col)
{
   int fore, back;
   int fs = pos("/",col);

   fore = atoi(substr(col,0,fs));
   back = atoi(substr(col,fs+1,strlen(col)));

   return(cval(fore,back));
}

static char *comma(char *str1)
{
   int en;
   word i, tpos = 1;
   //char buffer[MAXSTR];

   strcpy(buffer,str1);
   en = ((en = pos(".",buffer)) == -1) ? strlen(buffer) : en;
   for (i = --en; i > 0; i--)
   {
      if (strchr("0123456789",buffer[i-1]) != NULL)
      {
         if (tpos == 3)
         {
            chinsert(",",buffer,i);
            tpos = 1;
         } else
            tpos++;
      }
   }
   if (strlen(buffer) > iptr->len)
      strcpy(buffer,substr(buffer,0,iptr->len-1));
   return(buffer);
}

void dbox(word x1, word y1, word x2, word y2,
          word style, word mode, word shadow)

/*                                                                 */
/* Draw a box within the current window based on the coords passed */
/* Style options: DOUBLE, SINGLE, THICK, NONE                      */
/* Mode options : NORM, NOCLEAR (draw border only), EXPLODE        */
/* Shadow       : TRUE or FALSE                                    */
/*                                                                 */

{
   if (mode == EXPLODE)
      boxexplode(x1, y1, x2, y2, style, NORM, shadow);
   else
      border(x1, y1, x2, y2, style, mode, shadow);
}

void defcolor(colortable newtab, char *nc, char *hc, char *bl, char *bi, char *bc, char *na, char *ot)
{
   coltable[0] = colcode(nc);
   coltable[1] = colcode(hc);
   coltable[2] = colcode(bl);
   coltable[3] = colcode(bi);
   coltable[4] = colcode(bc);
   coltable[5] = colcode(na);
   coltable[6] = colcode(ot);
   textattr(coltable[0]);
   memmove(newtab,coltable,sizeof(colortable));
}

void delete(char *str1, word index, word num)
{
   word i, j;
   char buffer[MAXSTR];

   for (i = j = 0; i < strlen(str1); i++)
      if (i < index || i > index + num - 1)
         buffer[j++] = str1[i];
   buffer[j] = NULL;
   strcpy(str1,buffer);
}

char derr(char *str1)
{
   word d,m,y;
   char err = FALSE;

   d = partdate(str1,0);
   m = partdate(str1,3);
   y = partdate(str1,6);

   err = (m <1 || m > 12) ? TRUE : FALSE;
   if (!err)
      err = (d < 1 || d > 31 || d == 31 &&
            (m == 2 || m == 4 || m == 6 || m == 9 || m == 11)) ? TRUE : FALSE;
   if (!err)
      err = (m == 2 && (d == 30 || (d == 29 && y % 4 != 0))) ? TRUE : FALSE;
   return(err);
}

void doprompt(void)
{
   word i;
   char st[MAXSTR];


   for (i=0; i < iptr->nitem; i++)
   {
      strcpy(st,cptr->option[i]);
      if (cptr->desc != NULL && cptr->desc[i][0] != NULL)
         strcat(st,cptr->desc[i]);
      prompt(st,NOCOL,"");
   }
}

void help(void)
{
   int i = 0;

   save_status();
   defcolor(coltable,"15/2","14/2","15/2","15/2","14/2","15/2","15/2");
   //msg(HELPHANDLE,5,5,75,20,SINGLE,NORM,TRUE," Help ",0,helpText);
   notice(HELPHANDLE, 5, 2, 75, 23, SINGLE, NORM, TRUE, " Help ", 0, "");
   for (i = 0; i < helpTextLineCount; i++) {
      textline(7, 3+i, NORM, NORM, helpText[i]);
   }
   while (!kbhit() || getch()==0);
   killmsg(HELPHANDLE);
   return_status();
}

int entry(char mode)
{
   char ok  = TRUE;
   char ext;
   int rv = 0;

   init_entry();
   while (ok)
   {
      show_entry();
      getdata();
      show_entry();
      ext = TRUE;
      if (ch == NOKEY)
      {
         switch(extend)
         {
            case DOWNARR : if (iptr->next != NULL)
                              iptr = iptr->next;
                           ext = FALSE;
                           break;
            case UPARR   : if (iptr->prev != NULL)
                              iptr = iptr->prev;
                           ext = FALSE;
                           break;
            case F10KEY : ok = FALSE;
                          break;
            case BACKTAB  : if (iptr->prev != NULL) iptr = iptr->prev;
                            ext = FALSE;
                            break;
         }
      } else
      {
         switch(ch)
         {
            case ENTERKEY : /*if (iptr->next != NULL)
                            {
                               iptr = iptr->next;
                               ext = FALSE;
                            } else*/
                               ok = FALSE;
                            break;
            case ESCKEY   : ok = FALSE;
                            rv = -1; // User aborted.
                            break;
            case TAB       : if (iptr->next != NULL) iptr = iptr->next;
                            ext = FALSE;
                            break;
         }
      }
      ok = (ext && mode == KEYEXIT) ? FALSE : ok;

   }
   zapinput();
   return rv;
}

void errbeep(void)
{
   play(900,60);
   delay(40);
   play(900,60);
   delay(40);
   play(900,60);
}

static iformat evalform(char *format)
{
   char buffer[MAXSTR];
   char *tokptr;
   iformat efm;
   char chk;

   setmem(&efm,sizeof(efm),0);
   efm.dec = 2;
   strcpy(buffer,strip(format));
   tokptr = strtok(buffer,"%");
   while (tokptr != NULL)
   {
      chk = tokptr[0];
      switch(chk)
      {
         case 'd' : /* Date          Max size = 8 99/99/99    */
         case 't' : /* Time          Max size = 8 99:99:99    */
                    efm.mode[3] = TRUE;
                    efm.type = chk;
                    break;
         case 's' : /* String        Max size = 255           */
                    efm.type = chk;
                    break;
         case 'x' : efm.type = chk;
                    cptr = (chstruct *) iptr->chvar;
                    break;
         case 'l' : /* Long (int)    Max size = +- 2147483647 */
         case 'w' : /* Word          Max size = 0..65536      */
         case 'i' : /* Integer       Max size = -32767..32767 */
         case 'c' : /* Character     Max size = 1             */
         case 'f' : /* Float point   Max size = Compiler Max. */
                    efm.mode[2] = TRUE;
                    efm.type = chk;
                    break;
         case 'W' : strcpy(tokptr,numonly(tokptr));
                    efm.width = atoi(tokptr);
                    break;
         case 'D' : strcpy(tokptr,numonly(tokptr));
                    efm.dec = atoi(tokptr);
                    break;
         case 'F' : delete(tokptr,0,1);
                    efm.form = *tokptr;
                    break;
         case 'N' : efm.neg = TRUE; break;
         case '0' : efm.zero = TRUE; break;
         case 'U' : efm.mode[0] = TRUE; break;
         case 'C' : efm.mode[1] = TRUE; break;
         case 'J' : efm.just = tokptr[1]; break;
         case '[' : delete(tokptr,0,1);
                    strcpy(efm.valid,tokptr);
                    efm.valid[strlen(tokptr)-1] = NULL;
                    break;
         case '<' : break;
      }
      tokptr = strtok(NULL,"%");
   }
   efm.width = (efm.width == 0) ? iptr->len : efm.width;
   return(efm);
}

void fatal(char *head, char *msg)
{
   defcolor(coltable,"15/4","14/4","0/4","15/4","15/4","7/4","15/4");
   dbox(35,18,75,20,SINGLE,NORM,TRUE);
   textline(55,18,CTR,1," %s ",head);
   textline(55,19,CTR,0,"%s",msg);
   errbeep();
   getch();
   gotoxy(1,25);
   textattr(7);
   clreol();
   exit(1);
}

sbuf *findptr(word handle)                        /* Internal, used to find  */
{                                                 /* a handle and return ptr */
   if (scfrst == NULL)                           /* The handle must exist   */
      fatal(" FATAL call ","Return screen called before save");

   scpntr = scfrst;                              /* Go top of list          */
   while (scpntr != NULL && scpntr->handle != handle) /* while not end of list */
      scpntr = scpntr->next;                          /* skip                  */
   if (scpntr == NULL)
      fatal(" FATAL Return error ","Could not find screen handle");
   return(scpntr);
}

static void frmstr(char *str)
{
   int ps;

   switch(iform.form)
   {
      case 'C' : strcpy(str,comma(str));
                 break;
      case '$' : strcpy(str,comma(str));
                 chinsert("$",str,0);
                 break;
   }
   if (iform.neg && (ps = pos("-",str)) != -1)
   {
      delete(str,ps,1);
      chinsert("(",str,0);
      chinsert(")",str,strlen(str));
   }
}

static char *ftos(double num, int dec)
{
   //char buffer[50];
   char tmp[32];
   int decpt, negpt;

   strcpy(buffer,fcvt(num,dec,&decpt,&negpt));
   if (decpt < 1)
   {
      decpt = abs(decpt) + 1;
      strcpy(tmp,rep('0',decpt));
      chinsert(tmp,buffer,0);
      decpt = 1;
   }
   chinsert(".",buffer,decpt);
   if (strlen(buffer) >= decpt+dec)
   {
      decpt = (dec == 0) ? decpt : decpt+1;
      delete(buffer,decpt+dec,strlen(buffer)-decpt+dec);
   }

   if (negpt)
      chinsert("-",buffer,0);
   if (strlen(buffer) > iptr->len)  /* String to big to fit, cut to fit */
      strcpy(buffer,substr(buffer,0,iptr->len-1));
   return(buffer);
}

void get(word x, word y, word len, char far *format, void far *arg1, void far *valfnt)
{
   if (coreleft() < 1000)
      fatal("FATAL get call","Not enough memory to input detail");

   if (ifirst == NULL)
   {
      ifirst = (inputs *) malloc(sizeof(inputs));
      iptr   = ifirst;
      iptr->prev = NULL;
   } else
   {
      iptr->next       = (inputs *) malloc(sizeof(inputs));
      iptr->next->prev = iptr;
      iptr             = iptr->next;
   }
   iptr->next   = NULL;                          /* Last item on list        */
   ilast        = iptr;                          /* This one's the last      */
   iptr->x      = x;                             /* X and Y coords           */
   iptr->y      = y;
   iptr->len    = len;                           /* The passed vars length   */
   iptr->format = (char *) malloc(strlen(format)+1); /* Alloc space for Format*/
   strcpy(iptr->format,format);                  /* Fill with format details */
   iptr->arg1   = arg1;                          /* Pointer to the actual var */
   iptr->chvar  = NULL;                          /* Used by choice function  */
   iptr->valfnt = valfnt;                        /* Pointer to the valid func */
   iptr->value  = (char *) malloc(iptr->len+1);  /* Display value space      */
}

/*void getchoice(void)
{
   word chnum;

   textline(iptr->x,iptr->y,NORM,BAR,"%s",iptr->value);
   gotoxy(iptr->x,iptr->y);
   rkbd();
   if (ch != NOKEY && ch != ESCKEY)
   {
      msg(CHOICEHAN,cptr->x1,cptr->y1,cptr->x2,cptr->y2,
          cptr->style,cptr->mode,cptr->shadow,"",0,"");
      doprompt();
      chnum = list(cptr->x1+1,cptr->y1+1,cptr->x2-1,cptr->y2-1,NOROLL,1,0,0);
      killmsg(CHOICEHAN);
      if (chnum)
      {
         strcpy(iptr->value,ltrim(cptr->option[chnum-1]));
         *(word *)cptr->retval = chnum - 1;
         if (iptr->next == NULL)
         {
            extend = UPARR;
            ch = NOKEY;
         }
      }
   }
}*/

static void getdata(void)
{
      char serr = TRUE;
      char cont;

   do
   {
      textline(iptr->x,iptr->y,NORM,BAR,"%s",space(iform.width+1));
      gotoxy(iptr->x,iptr->y);
      switch(iform.type)
      {
         case 't' :
         case 'd' : do
                    {
                       keyin(iptr->value,iptr->len,iform.width,BAR,iform.mode);
                       strcpy(iptr->arg1,iptr->value);
                       serr = derr(iptr->value);
                       gotoxy(iptr->x,iptr->y);
                    } while (serr && ch != 27);
                    break;

         case 's' : keyin(iptr->value,iptr->len,iform.width,BAR,iform.mode);
                    strcpy(iptr->arg1,iptr->value);
                    break;

         case 'w' : keyin(iptr->value,iptr->len,iform.width,BAR,iform.mode);
                    strcpy(iptr->value,wordonly(iptr->value));
                    *(word *) iptr->arg1 = (word) atoi(iptr->value);
                    if (*(word *) iptr->arg1 == 0)
                       showzero = TRUE;
                    break;

         case 'i' : keyin(iptr->value,iptr->len,iform.width,BAR,iform.mode);
                    strcpy(iptr->value,intonly(iptr->value));
                    *(int *) iptr->arg1 = atoi(iptr->value);
                    if (*(int *) iptr->arg1 == 0)
                       showzero = TRUE;
                    break;

         case 'l' : keyin(iptr->value,iptr->len,iform.width,BAR,iform.mode);
                    strcpy(iptr->value,numonly(iptr->value));
                    *(long *) iptr->arg1 = (word) atol(iptr->value);
                    if (*(long *) iptr->arg1 == 0)
                       showzero = TRUE;
                    break;

         case 'f' : keyin(iptr->value,iptr->len,iform.width,BAR,iform.mode);
                    strcpy(iptr->value,numonly(iptr->value));
                    *(double *) iptr->arg1 = atof(iptr->value);
                    if (*(double *) iptr->arg1 == 0)
                       showzero = TRUE;
                    break;

         /*case 'x' : getchoice();
                    ch = (ch == ESCKEY) ? NULL : ch;
                    break;*/
      }
      if (iptr->valfnt != NULL)
      {
         itf  = iptr->valfnt;
         cont = (*itf)();
      } else
         cont = TRUE;
   } while (!cont);
}

static void init_entry(void)
{
   iptr = ifirst; /* Point to first input record */
   while (iptr != NULL)
   {
      show_entry();
      iptr = iptr->next;
   }
   iptr = ifirst; /* Point to first input record */
}

void inschr(char substr1, char *str1, word ps)
{
   char buffer[MAXSTR];
   word i,j;
   word tot   = strlen(str1) + 1;


   for (i = j = 0; i < tot; i++)
   {
      if (i < ps || i > ps)
         buffer[i] = str1[j++];
      else
         buffer[i] = substr1;
   }
   buffer[tot] = NULL;                /* put a null at the end of the string */
   strcpy(str1,buffer);               /* return it in the source */
}

char *intonly(char *str1)
{
   int i, j;
   long l;

   if ((i = pos("(",str1)) != -1)           /* Fix for ( mode */
      str1[i] = '-';
   for (i = j = 0; i <= strlen(str1); i++)
      if (strchr("1234567890+-.",str1[i]))
         buffer[j++] = str1[i];
   buffer[j] = NULL;
   l = atol(buffer);
   if (l > 32737L || l < -32737L)
      strcpy(buffer,"0");
   return(buffer);
}

char *justfy(void)
{
   //char hold[MAXSTR];
   char fm[MAXSTR];
   char xxx[MAXSTR];

   sprintf(xxx,"-%ds",iform.width+1);
   strcpy(fm,"%");
   strcat(fm,xxx);
   //sprintf(hold,fm,substr(iptr->value,0,iform.width+1));
   //return(hold);
   sprintf(buffer,fm,substr(iptr->value,0,iform.width+1));
   return(buffer);
}

void killitem(sbuf *scpntr)                     /* Internal, used to destroy */
{                                              /* screen list item and buf  */
   char located = FALSE;
   sbuf *tmpptr,*srcptr;

   if (scpntr == scfrst)
   {
      tmpptr = scfrst;
      scfrst = scpntr->next;
      free(tmpptr->sbuffer);
      free(tmpptr);
   }
   else
   {
      srcptr = scfrst;
      while (srcptr->next != NULL && !located)
      {
         if (srcptr->next == scpntr)
            located = TRUE;
         else
            srcptr = srcptr->next;
      }
      if (located)
      {
         scpntr = srcptr;
         tmpptr = scpntr->next;
         scpntr->next = scpntr->next->next;
         free(tmpptr->sbuffer);
         free(tmpptr);
         if (scpntr->next == NULL)
            sclast = scpntr;
      }
   }
}

void killmsg(word handle)
{
   char located = FALSE;
   msglist *tmpptr, *locptr;

   if (mfirst == NULL)
      fatal(" FATAL call ","Cannot kill the message");

   mptr = mfirst;                              /* Find handle             */
   while (mptr != NULL && mptr->handle != handle)
      mptr = mptr->next;

   if (mptr == NULL)
      fatal(" FATAL Return error ","Could not find message handle");

   if (mptr->mode == EXPLODE)
      boxdestruct(mptr->handle,mptr->x1,mptr->y1,mptr->x2,mptr->y2,
                  mptr->style,mptr->shadow);

    else
       returnscreen(mptr->handle,TRUE);

   if (mptr == mfirst)
   {
      tmpptr = mfirst;
      mfirst = mptr->next;
      mptr   = mfirst;
      free(tmpptr);
   }
   else
   {
      locptr = mptr;
      mptr = mfirst;
      while (mptr->next != NULL && !located)
         if (mptr->next == locptr)
            located = TRUE;
         else
            mptr = mptr->next;
      if (located)
      {
         tmpptr = mptr->next;
         mptr->next = mptr->next->next;
         free(tmpptr);
         if (mptr->next == NULL)
            mlast = mptr;
      }
   }
}

char *ltrim(char *str1)
{
   word i = 0;

   strcpy(buffer,str1);
   while ((buffer[i] == ' ') && (i <= strlen(buffer)))
      i++;
   delete(buffer,0,i);
   return(buffer);
}

void msg(word handle,                         /* Handle used to access msg  */
         word x1, word y1, word x2, word y2,  /* Box co-ords                */
         word style, word mode, word shadow,  /* Box style                  */
         char *head, word hpos,               /* Header & position from top */
         char *ms)                            /* The message                */
{
   savemsg(handle,x1,y1,x2,y2,style,mode,shadow);
   dbox(x1,y1,x2,y2,style,mode,shadow);
   textline(mid(x1,x2),y1+hpos,CTR,HIGH,head);
   textline(mid(x1,x2),y2 - 1,CTR,NORM,ms);
}

void notice(word handle,                         /* Handle used to access msg  */
            word x1, word y1, word x2, word y2,  /* Box co-ords                */
            word style, word mode, word shadow,  /* Box style                  */
            const char far *head, word hpos,               /* Header & position from top */
            const char far *ms) //,...)                        /* The message format & info  */
{
   //va_list mess;
   //char *form;
   //char output[MAXSTR];

   savemsg(handle,x1,y1,x2,y2,style,mode,shadow);
   dbox(x1,y1,x2,y2,style,mode,shadow);
   textline(mid(x1,x2),y1+hpos,CTR,HIGH,head);
   //va_start(mess, ms);
   //form = ms;
   //vsprintf(output, form, mess);
   setwindows(x1+2,y1+1,x2-2,y2-1);
   //textline(1,1+hpos+1,NORM,NORM,output);
   textline(1,1+hpos+1,NORM,NORM,ms);
   returnwindow();
}

char *numonly(char *str1)
{
   int i, j;

   if ((i = pos("(",str1)) != -1)           /* Fix for ( mode */
      str1[i] = '-';
   for (i = j = 0; i <= strlen(str1); i++)
      if (strchr("1234567890+-.",str1[i]))
         buffer[j++] = str1[i];
   buffer[j] = NULL;
   return(buffer);
}

static word partdate(char *dstr, word st)
{
   char buf[3];

   buf[0] = dstr[st];
   buf[1] = dstr[st+1];
   buf[2] = NULL;
   return(atoi(buf));
}

static void play(word freq, word dur)
{
   sound(freq);
   delay(dur);
   nosound();
}

word pos(char *substr1, char *str1)
{
   char *rslt;

   if ((rslt = strstr(str1,substr1)) != NULL)
      return(strlen(str1) - strlen(rslt));
   else
      return(-1);
}

char *rep(char ch, word num)
{

   setmem(&buffer,num,ch);
   buffer[num] = NULL;
   return(buffer);
}

void return_status(void)
{
   memmove(coltable,screen_def.savecol,sizeof(coltable));
   gotoxy(screen_def.x,screen_def.y);
   setcol(coltable);
}

void returnscreen(word hand, char destroy)   /* Return a saved screen   */
{
   scpntr = findptr(hand);                    /* Find the handle, it must be */
                                             /* valid and exist otherwise   */
                                             /* the system will halt        */
   puttext(scpntr->x1,scpntr->y1,scpntr->x2,scpntr->y2,scpntr->sbuffer); /* return */
   if (destroy)                              /* If kill then kill it        */
      killitem(scpntr);
}

void returnwindow(void)                            /* Return the last saved */
{                                                  /* window                */
   window(save_info.winleft,save_info.wintop,
          save_info.winright,save_info.winbottom);
}

void rkbd(void)       /* Reads the keyboard (via bios) and store values  */
{                     /* in global vars ch and extend.                   */
   int chkey;
   //char init = TRUE;

   do
   {
      while (!bioskey(1))
      {
         //drawdt(init);
         //init = FALSE;
      }
      chkey  = bioskey(0);                /* Get the new key                */
      ch     = chkey & 0X00FF;            /* Store the standard key value   */
      extend = (chkey & 0XFF00) >> 8;     /* Store the extended key value   */
      if (extend == F1KEY)
         help();
      if (ctrl_brk && ch == 0 && extend == 0)  /* Check ctrl break          */
         exit(1);
   } while (extend == F1KEY);
}

void savescreen(word handle, word x1, word y1, word x2, word y2)
{
   word bufsize = (x2-x1+1) * (y2-y1+1) * 2;       /* size of the screen area */

   if (coreleft() <= 5000L)                        /* prevent memory crash    */
      fatal(" Out of memory ","Cannot perform screen save");

   if (scfrst == NULL)                            /* if first save           */
   {
      scfrst = (sbuf *) malloc(sizeof(sbuf));     /* Alloc list space        */
      scpntr = scfrst;                             /* Last = first ptr        */
   } else
   {
      scpntr       = sclast;                        /* Go to end of list       */
      scpntr->next = (sbuf *) malloc(sizeof(sbuf)); /* Add the new ptr         */
      scpntr = scpntr->next;                         /* Store to current        */
   }
   scpntr->next    = NULL;                        /* This is the last on the list */
   sclast         = scpntr;                       /* Point to the last entry  */
   scpntr->handle  = handle;                      /* Store detail to struct   */
   scpntr->x1      = x1;
   scpntr->y1      = y1;
   scpntr->x2      = x2;
   scpntr->y2      = y2;
   scpntr->sbuffer = (char *) malloc(bufsize);    /* Alloc screen buffer      */
   gettext(x1,y1,x2,y2,scpntr->sbuffer);          /* Save it                  */
}

void save_status(void)
{
   memmove(screen_def.savecol,coltable,sizeof(coltable));
   screen_def.x = wherex();
   screen_def.y = wherey();
}

void savemsg(word handle,
             word x1, word y1, word x2, word y2,
             word style, word mode, word shadow)
{
   if (coreleft() < 5000)
      fatal("FATAL message","Not enough memory to save message");

   if (mfirst == NULL)
   {
      mfirst = (msglist *) malloc(sizeof(msglist));
      mptr   = mfirst;
   } else
   {
      mptr->next = (msglist *) malloc(sizeof(msglist));
      mptr       = mptr->next;
   }
   mptr->next   = NULL;                         /* save message details */
   mlast        = mptr;
   mptr->handle = handle;
   mptr->x1     = x1;
   mptr->x2     = x2;
   mptr->y1     = y1;
   mptr->y2     = y2;
   mptr->style  = style;
   mptr->mode   = mode;
   mptr->shadow = shadow;
                                          /* All messages save the screen  */
   savescreen(handle,x1,y1,x2+3,y2+3);    /* below therefore they must be  */
                                          /* killed later                  */
}

void setcol(colortable newone)
{
   memmove(coltable,newone,sizeof(colortable));
   textattr(coltable[0]);
}

void setcursor(word shape)
/* Sets the shape of the cursor */
{
 union REGS reg;

 reg.h.ah = 1;
 reg.x.cx = shape;
 int86(0X10, &reg, &reg);
} /* setcursor */

void setwindows(word x1, word y1, word x2, word y2) /* Save the old window & */
{                                                  /* set a new one         */
   gettextinfo(&save_info);
   window(x1,y1,x2,y2);
}

static void show_entry(void)
{

   iform = evalform(iptr->format);
   switch(iform.type)
   {
      case 'd' : strcpy(iptr->value,iptr->arg1);
                 if (strlen(iptr->value) != 8)
                    strcpy(iptr->value,"  /  /  ");
                 iptr->value[2] = '/';
                 iptr->value[5] = '/';
                 break;

      case 't' : strcpy(iptr->value,iptr->arg1);
                 if (strlen(iptr->value) != 8)
                    strcpy(iptr->value,"  :  :  ");
                 iptr->value[2] = ':';
                 iptr->value[5] = ':';
                 break;

      case 's' : strcpy(iptr->value,iptr->arg1);
                 break;

      case 'w' : itoa(*(word *)iptr->arg1,iptr->value,10);
                 iform.just = (iform.just == NULL) ? 'R' : iform.just;
                 break;

      case 'i' : itoa(*(int *)iptr->arg1,iptr->value,10);
                 iform.just = (iform.just == NULL) ? 'R' : iform.just;
                 break;

      case 'l' : ltoa(*(long *)iptr->arg1,iptr->value,10);
                 iform.just = (iform.just == NULL) ? 'R' : iform.just;
                 break;

      case 'f' : strcpy(iptr->value,ftos(*(double *)iptr->arg1,iform.dec));
                 iform.just = (iform.just == NULL) ? 'R' : iform.just;
                 break;

      case 'x' : cptr = (chstruct *) iptr->chvar;
                 strcpy(iptr->value,ltrim(cptr->option[*(word *)cptr->retval]));
                 break;


   }
   showinput();
}

static void showinput(void)
{
   char buffer[MAXSTR];
   char tform[MAXSTR];

   strcpy(buffer,iptr->value);
   if (showzero && iform.zero)
   {
      strcpy(buffer,space(iptr->len));
      showzero = FALSE;
   } else
      frmstr(buffer);
   strcpy(tform,justfy());
   textline(iptr->x,iptr->y,NORM,NORM,tform,buffer);
}
char *space(word num)
{
   return(rep(' ',num));
}

char *strip(char *str1)
{
   word i, j;

   for (i = j = 0; i <= strlen(str1); i++)
      if (str1[i] != ' ')
         buffer[j++] = str1[i];
   buffer[j] = NULL;
   return(buffer);
}

char *substr(char *str1, word st, word num)
{
   word i, p = 0;
   static char retstr[5][MAXSTR];
   static int sp;

   setmem(retstr[sp],MAXSTR,0);
   for (i = 0; i < num; i++)
      retstr[sp][p++] = (st > strlen(str1)) ? ' ' : str1[st++];

   retstr[sp][p] = NULL;
   i = sp;
   sp = (sp == 4) ? 0 : sp + 1;
   return(retstr[i]);
}

void textline(word x, word y, word mode, word colr, va_list items,...)
{
 va_list it_ptr;
 char *format;
 char output[MAXSTR];

 va_start(it_ptr, items);
 format = items;
 vsprintf(output, format, it_ptr);
 textattr(coltable[colr]);
 switch (mode)
 {
    case NORM : gotoxy(x,y);
                break;
    case CTR  : gotoxy(centre(x,output),y);
                break;
    case CLE  : gotoxy(x,y);
                clreol();
                break;
    case ECTR : gotoxy(1,y);clreol();
                gotoxy(centre(x,output),y);
                break;
 }
 cprintf("%s",output);
 va_end(it_ptr);
}

char *trim(char *str1)
{
   int i;
   char buf2[MAXSTR];

   i = strlen(str1)-1;
   strcpy(buf2,str1);
   while (buf2[i] == ' ' && i >= 0)
      i--;
   strncpy(buffer,buf2,i+1);
   buffer[i+1] = NULL;
   return(buffer);
}

void warbeep(void)
{
   play(1200,50);
   delay(50);
   play(1200,50);
}

char *wordonly(char *str1)
{
   int i, j;
   long l;

   if ((i = pos("(",str1)) != -1)           /* Fix for ( mode */
      str1[i] = '-';
   for (i = j = 0; i <= strlen(str1); i++)
      if (strchr("1234567890+-.",str1[i]))
         buffer[j++] = str1[i];
   buffer[j] = NULL;
   l = atol(buffer);
   if (l > 65535L)
      strcpy(buffer,"0");
   return(buffer);
}

static void zapinput(void)
{
   iptr = ifirst;
   while (iptr->next != NULL)
   {
      iptr = iptr->next;
      free(iptr->prev->chvar);
      free(iptr->prev->value);
      free(iptr->prev->format);
      free(iptr->prev);
   }
   free(iptr->chvar);
   free(iptr->value);
   free(iptr->format);
   free(iptr);
   ifirst    = NULL;
   iptr      = NULL;
}




























////// LIST STUFF /////////////////


static word numitems = 0;
static word width;
static word offs;
static word numcols;
static word maxline;
static word xp, yp;
static word xstart;
static word ystart;
static word top = 0;
static word base;
static word ix1,iy1,ix2,iy2;
static word lstyle;
int lretv;                      /* global return value */

lists *lptr, *lfirst, *llast = NULL;
static void movepoint(char dir);

void prompt(char *opt, char mode, char *hlp)
{

   if (coreleft() < 1000)
      fatal("System Problem","Not enough memory (Listing)");

   if (lfirst == NULL)
   {
      lfirst = (lists *) malloc(sizeof(lists));
      lptr   = lfirst;
      lptr->prev = NULL;
      lptr->itnum = 0;
   } else
   {
      lptr->next       = (lists *) malloc(sizeof(lists));
      lptr->next->prev = lptr;
      lptr             = lptr->next;
      lptr->itnum      = lptr->prev->itnum + 1;
   }
   lptr->next   = NULL;
   llast        = lptr;
   lptr->option = (char *) calloc(1,strlen(opt)+1);
   lptr->hlp    = (char *) calloc(1,strlen(hlp)+1);
   strcpy(lptr->option,opt);
   strcpy(lptr->hlp,hlp);
   lptr->mode   = mode;
   numitems++;
}

static void skip(int times, word dir)
{
   word i;

   for (i = 0; i < times && lptr != NULL; i++)
   {
      if (dir == FWD)
      {
         lptr = lptr->next;                   /* skip one in the list.       */
         if (lptr != NULL)                    /* If not end of list          */
            if (lptr->itnum % numcols == 0)   /* If at right edge of table   */
            {                                 /* go down a line and end left */
               yp++;
               xp = xstart;
            }
            else                              /* else go right one column    */
               if (width == 0)                /* if noset mode go right the  */
                  xp += strlen(lptr->prev->option); /* width of the option   */
               else
                  xp += width;                /* else right the column width */
      } else
      {
         lptr = lptr->prev;                   /* go back one on the list     */
         if (lptr != NULL)                    /* if not null & at left edge  */
            if (lptr->itnum % numcols == numcols - 1)
            {
               yp--;                          /* up one line and right edge  */
               xp = xstart + (width * (numcols - 1));
            }
            else                              /* left one column, check NOSET */
               if (width == 0)
                  xp -= strlen(lptr->option);
               else
                  xp -= width;
      }
   }
}

static void show_fwd_page(word start)
{
   word orig_pos = lptr->itnum;
   int back     =  lptr->itnum % numcols;
   word i,j,x;

   skip(back,BWD);                              /* go to the top corner      */
   for (i = 0;i < maxline && lptr != NULL; i++) /* for the num lines in page */
   {
      for (j = 0; j < numcols && lptr != NULL; j++)  /* for num of cols */
      {
         x = (lptr->mode == NA) ? 5:0;               /* set color mode NORM/NA */
         textline(xp+offs,yp,NORM,x,"%s",lptr->option); /* draw option */
         if (lptr->mode != NA && lptr->mode != NOCOL)
         {
            x = strspn(lptr->option," ");               /* write highlight letter */
            textline(xp+offs+x,yp,NORM,1,"%c",lptr->option[x]);
         }
         skip(1,FWD);                                    /* go to next one */
      }
      if (lptr == NULL)
         lptr = llast;
   }
//   if (start)                                  /* if first time then goto the */
//      skip(lptr->itnum - start,BWD);           /* first item else goto the    */
//   else                                        /* original position           */
      skip(lptr->itnum - orig_pos,BWD);
      if (start)
         while (lptr->next != NULL && lptr->itnum != start)
            movepoint(DOWN);

}


static void show_line(void)
{
   word orig_pos = lptr->itnum;
   int back     =  lptr->itnum % numcols;
   word i,x;

   skip(back,BWD);
   if (width == 0)
      clrs(xp,yp,xp+(80*numcols),yp);
   else
      clrs(xp,yp,xp+(width*numcols)-1,yp);
   for (i = 0; i < numcols && lptr != NULL; i++)
   {
      x = (lptr->mode == NA) ? 5:0;
      textline(xp+offs,yp,NORM,x,"%s",lptr->option);
      if (lptr->mode != NA && lptr->mode != NOCOL)
      {
         x = strspn(lptr->option," ");
         textline(xp+offs+x,yp,NORM,1,"%c",lptr->option[x]);
      }
      skip(1,FWD);
   }
   if (lptr == NULL)
      lptr = llast;
   skip(lptr->itnum - orig_pos,BWD);
}

static void show_bar(void)
{
   if (strlen(lptr->hlp) > 0)
      textline(1,24,CTR,6,"%s",lptr->hlp);
   cla(xp+offs,yp,strlen(lptr->option),coltable[4]);
}

static void clear_bar(void)
{
   word x;

   if (strlen(lptr->hlp) > 0)
      textline(1,24,ECTR,6," ");
   cla(xp+offs,yp,strlen(lptr->option),coltable[0]);
   if (lptr->mode != NA && lptr->mode != NOCOL)
   {
      x = strspn(lptr->option," ");
      textline(xp+offs+x,yp,NORM,1,"%c",lptr->option[x]);
   }
}


static void chkscreen(void)
{
   if (lptr->itnum < top)
   {
      top -= numcols;
      base -= numcols;
      movetext(ix1,iy1,ix2,iy2-1,ix1,iy1+1);
      yp++;
      show_line();
   } else
   if (lptr->itnum > base)
   {
      top += numcols;
      base += numcols;
      movetext(ix1,iy1+1,ix2,iy2,ix1,iy1);
      yp--;
      show_line();
   }
}


static void gohome(void)
{
   char showpg = TRUE;
   if (top == 0)
      showpg = FALSE;
   lptr = lfirst;
   xp   = xstart;
   yp   = ystart;
   top  = 0;
   base = maxline * numcols - 1;
   if (showpg)
      show_fwd_page(0);
}

static void goend(void)
{
   word i;
   word back;

   back = ((maxline - 1) * numcols) + (llast->itnum % numcols);
   if (top < llast->itnum - back)
   {
      lptr = llast;
      for (i = 0; i < back && lptr->prev != NULL; i++)
         lptr = lptr->prev;

      xp  = xstart;
      yp  = ystart;
      top = lptr->itnum;
      base = maxline * numcols - 1 + top;
      if (top > 0)
         clrs(ix1,iy1,ix2,iy2);
      show_fwd_page(0);
   }
   skip(llast->itnum - lptr->itnum,FWD);
   while (lptr->mode == NA)
      skip(1,BWD);
}

static void gopgup(void)
{
   word back = lptr->itnum % (numcols * maxline);
   lists *tptr = lptr;

   if (top > 0)
   {
      skip(back,BWD);
      skip(numcols*maxline,BWD);
      xp = xstart;
      yp = ystart;
      if (lptr == NULL)
      {
         gohome();
         skip(tptr->itnum % numcols,FWD);
      } else
      {
         top  = lptr->itnum;
         base = maxline * numcols - 1 + top;
         clrs(ix1,iy1,ix2,iy2);
         show_fwd_page(0);
         skip(back,FWD);
      }
   } else
   {
      gohome();
      skip(tptr->itnum % numcols,FWD);
   }
   while (lptr->mode == NA)
      skip(1,FWD);
}


static void gopgdn(void)
{
   word back = lptr->itnum % (numcols * maxline);
   int adj;
   lists *sptr = lptr;

      if (lptr->itnum + numcols * maxline < numitems)
      {
         skip(numcols * maxline - back,FWD);
         xp = xstart;
         yp = ystart;
         if (back + lptr->itnum > numitems - 1)
            back = numitems - lptr->itnum - 1;
         top  = lptr->itnum;
         base = maxline * numcols - 1 + top;
         clrs(ix1,iy1,ix2,iy2);
         show_fwd_page(0);
         skip(back,FWD);
      } else
      {
         goend();
         adj = lptr->itnum %numcols - (sptr->itnum % numcols);
         if (adj < 0)
         {
            skip(numcols,BWD);
            skip(adj*-1,FWD);
         } else
            skip(adj,BWD);
      }
      while (lptr->mode == NA)
         skip(1,FWD);
}


static void movepoint(char dir)
{
   lists *stptr = lptr;
   word sxp     = xp;
   word syp     = yp;
   int adj;

   switch(dir)
   {
      case UP    : do
                   {
                     skip(numcols,BWD);
                   }
                   while (lptr != NULL && lptr->mode == NA);

                   if (lptr == NULL)
                   {
                      if (numcols == 1 && lstyle != NOROLL)
                      {
                         lptr = lfirst;
                         goend();
                      } else
                      {
                         lptr = stptr;
                         xp   = sxp;
                         yp   = syp;
                      }
                   }
                   break;
      case DOWN  : do
                   {
                     skip(numcols,FWD);
                   }
                   while (lptr != NULL && lptr->mode == NA);
                   if (lptr == NULL)
                   {
                      if (numcols == 1 && lstyle != NOROLL)
                        gohome();
                      else
                      {
                         lptr = stptr;
                         xp   = sxp;
                         yp   = syp;
                         goend();
                         adj = lptr->itnum %numcols - (stptr->itnum % numcols);
                         if (adj < 0)
                         {
                            skip(numcols,BWD);
                            skip(adj*-1,FWD);
                         } else
                            skip(adj,BWD);

                      }
                   }
                   break;
      case LEFT  : do
                   {
                     skip(1,BWD);
                   }
                   while (lptr != NULL && lptr->mode == NA);
                   if (lptr == NULL)
                   {
                      lptr = lfirst;
                      (lstyle == NOROLL) ? gohome() : goend();
                   }
                   break;
      case RIGHT : do
                   {
                     skip(1,FWD);
                   }
                   while (lptr != NULL && lptr->mode == NA);
                   if (lptr == NULL)
                   {
                      lptr = llast;
                      (lstyle != NOROLL) ? gohome() : goend();
                   }
                   break;
      case PGUP  : gopgup();
                   break;
      case PGDN  : gopgdn();
                   break;

   }
   chkscreen();
}


static void search_item(char key)
{
   char skey   = upchar(key);
   char testopt[MAXSTR];
   lists *tptr = lptr;
   char found  = FALSE;

   do
   {
      tptr = tptr->next;
      if (tptr == NULL)
         tptr = lfirst;
      strcpy(testopt,ltrim(tptr->option));
      if (upchar(testopt[0]) == skey && lptr->mode != NA)
      {
         if (tptr->itnum < lptr->itnum)
            skip(lptr->itnum - tptr->itnum,BWD);
         else
            skip(tptr->itnum - lptr->itnum,FWD);
         found = TRUE;
      }
   } while (tptr != lptr && !found);
   chkscreen();
}

void zaplist(void)
{
   lptr = lfirst;
   while (lptr->next != NULL)
   {
      lptr = lptr->next;
      free(lptr->prev->option);
      free(lptr->prev->hlp);
      free(lptr->prev);
   }
   free(lptr->option);
   free(lptr->hlp);
   free(lptr);
   lfirst    = NULL;
   numitems  = 0;
   top       = 0;
}

/*word list(word x1, word y1, word x2, word y2,
          word style, word nocols, word offset, word first)
{
   word retnval;
   char killch = FALSE;

   if (lfirst == NULL)
   {
      beep();
      notice(15,20,8,60,10,SINGLE,NORM,TRUE," Notice ",0,"       No information to list");
      rkbd();
      killmsg(15);
      listvar[0] = NULL;
      ch = NOKEY;
      return(NOSET);
   }

   setcursor(nocursor);
   maxline = (numitems - 1 > (y2 - y1) * nocols) ? y2 - y1 + 1 : (numitems - 1) / nocols + 1;
   first = (first == NOSET) ? 0 : first;
   first = (numitems - 1 < first) ? numitems - 1 : first;
   numcols = nocols;
   lstyle  = style;
   if (style != NOSET)
      width   = (x2 - x1 + 1) / nocols;
   else
   {
      width = 0;
      style = NORM;
   }
   offs    = offset;
   xp      = x1;
   yp      = y1;
   xstart  = x1;
   ystart  = y1;
   base    = maxline * numcols - 1;
   ix1     = x1;
   ix2     = x2;
   iy1     = y1;
   iy2     = y2;


   lptr = lfirst;

   show_fwd_page(first);
   if (style != DISPLAY)
   do
   {
      show_bar();
      rkbd();
      if (ch != ENTERKEY && ch != ESCKEY)
         clear_bar();
      if (altpress && style == ALTMODE)        // Check to see if alt x pressed
         ch = ESCKEY;
      lretv = lptr->itnum;
      strcpy(listvar,lptr->option);
      if (strchr(listcheck,extend) != NULL)
         return(NOSET);
      if (ch == NOKEY)
      {
         switch (extend)
         {
            case LEFTARR  : movepoint(LEFT);
                            break;
            case RIGHTARR : movepoint(RIGHT);
                            break;
            case UPARR    : movepoint(UP);
                            break;
            case DOWNARR  : movepoint(DOWN);
                            break;
            case HOMEKEY  : gohome();
                            break;
            case ENDKEY   : goend();
                            break;
            case PGUPKEY  : movepoint(PGUP);
                            break;
            case PGDNKEY  : movepoint(PGDN);
                            break;
            default       : ch = (style == KEYEXIT) ? ESCKEY : NOKEY;
                            killch = TRUE;
         }
      } else
      {
         if (ch == 32)
            movepoint(RIGHT);
         if (ch != 27 && ch != 13 && lstyle != NOROLL)
            search_item(ch);

      }
   } while (ch != 27 && ch != 13 && ch != 66);

   if (ch == 13 || style == KEYEXIT)
   {
      if (ch == 13)
         retnval = lptr->itnum + 1;
      else
         retnval = 0;
      strcpy(listvar,lptr->option);
   }
   else
   {
      if (altpress && style == ALTMODE)        // Check to see if alt 1..9 pressed
         retnval = ALTRET;
      else
         retnval  = 0;
      strcpy(listvar,"");
   }
   if (strlen(lptr->hlp) > 0)
      textline(1,24,ECTR,6," ");
   zaplist();
   if (style == KEYEXIT && killch) // force the keyexit to return null ch
      ch = NOKEY;
   setcursor(normcursor);
   return(retnval);
}*/



















///////// KEYIN STUFF //////////////////////


void setins(word x, word y, word col, char show, char ins, char *im, char *om)
{
   insx = x;
   insy = y;
   insc = col;
   ins_mode = ins;
   show_ins = show;
   strcpy(insmess,im);
   strcpy(ovrmess,om);
}

static void ins_pressed(void)
{
   if (show_ins)
      if (ins_mode)
         textline(insx,insy,NORM,insc,"%s",insmess);
      else
         textline(insx,insy,NORM,insc,"%s",ovrmess);
}

static void fixkeyin(char *line, word len, word colr)
{
   word hr      = wherex();                        /* Current screen horiz.  */
   word start   = wherex();                        /* Start screen horiz.    */
   word vr      = wherey();                        /* Screen vertical        */
   word max     = len + (start-1);
   word vislen  = len;
   char oline[MAXSTR];                             /* Used for restore line  */
   char nomore  = FALSE;                           /* Finished ok!           */
   ins_pressed();                                  /* Show insert mode       */
   strcpy(oline,line);                             /* Save line to old       */
   textline(start,vr,NORM,colr,"%s",               /* Draw line              */
            substr(line,0,vislen));
   do                                              /* Loop until nomore      */
   {
      gotoxy(hr,vr);                               /* set cursor             */
      rkbd();                                      /* Get a key from bios    */
      if (ch == NOKEY)                             /* If extended            */
      {
         nomore = TRUE;                            /* if key not is list then*/
         switch (extend)                           /* exit routine.          */
         {

            case F8KEY    : strcpy(line,oline);    /* Restore line as origin */
                            textline(start,vr,NORM,colr,"%s",space(vislen));
                            hr   = start;
                            nomore = FALSE;
                            break;


            case LEFTARR  : if (hr > start)        /* Left arrow             */
                               hr--;
                               if (hr - start == 2 || hr - start == 5)
                                  hr--;
                            nomore = FALSE;
                            break;

            case RIGHTARR : if ((hr < max) && (hr < start + strlen(line)))
                               hr++;
                               if (hr - start == 2 || hr - start == 5)
                                  hr++;
                            nomore = FALSE;
                            break;

            case HOMEKEY  : hr   = start;
                            nomore = FALSE;
                            break;

            case ENDKEY   : hr = start + strlen(line);
                            nomore = FALSE;
                            break;

            case F4KEY    :
            case CTRLEFT  :
            case CTRLRT   :
            case DELKEY   : nomore = FALSE;
                            break;

            case INSKEY   : ins_mode = (ins_mode) ? FALSE : TRUE;
                            ins_pressed();
                            nomore = FALSE;
                            break;
         }
      } else
      {
         if (ch == BACKSP || ch == ENTERKEY || ch == ESCKEY)
         {
            switch (ch)
            {

               case BACKSP  : if (hr > start)        /* Left arrow             */
                                  hr--;
                                  if (hr - start == 2 || hr - start == 5)
                                     hr--;
                              nomore = FALSE;
                              break;

               case ENTERKEY : nomore = TRUE;
                               break;

               case ESCKEY   : nomore = TRUE;
                               break;

            }
         }
         else
         {
            if (strchr("0123456789",ch) != NULL)
            if (hr - start < len)
            {
               if (line[hr-start] != NULL)
                  delete(line,hr-start,1);
               inschr(ch,line,hr-start);
               if (hr < max)
                  ++hr;
                  if (hr - start == 2 || hr - start == 5)
                     hr++;
            }
         }
      }
      textline(start,vr,NORM,colr,"%s",substr(line,0,vislen));
   } while (!nomore);
}



/* Warning... */

/* This routine does not check for overflow so ensure that the */
/* len is 1 less than the actual string array length otherwise */
/* corruption may occur. (see C manual for details)            */

/***************************************************************/
/* keyin:                                                      */
/*                                                             */
/* This routine allows input of a string of len length, if the */
/* max width is less than len then the routine will scroll the */
/* line within the max width. The cursor position is assumed   */
/* to be correct upon entry to the routine.                    */
/*                                                             */
/* line : Variable used to return the string processed.        */
/* len  : Maximum string length allowed                        */
/* max  : Width on screen for string                           */
/* colr : Colour of string displayed                           */
/* form : array of formats (True/False), they are as follows:  */
/*      : 0 - Upper, 1 - Clear line, 2 - Numbers only,         */
/*      : 3 - Date or time (XX?YY?ZZ)                          */
/*      : 4..9 - Not used (yet!)                               */
/*                                                             */
/* Keys used: Shift F3 - Delete End of line (SFF3)             */
/*            Shift F5 - Restore old line   (SFF5)             */
/*            Standard input keys (L,R,CtL,CtR,H,E,Del,Ins,BS) */
/*            Nb. These keys cannot be used (and should not be)*/
/*                by the calling func during keyin.            */
/***************************************************************/


char *keyin(char *line, word len, word max, word colr, char *form)
{
   word hr      = wherex();                        /* Current screen horiz.  */
   word start   = wherex();                        /* Start screen horiz.    */
   word vr      = wherey();                        /* Screen vertical        */
   word lpos    = 0;                               /* Logical string start   */
   word vislen  = (len > max) ? max : len;         /* Visible string width   */
   word lindex;                                    /* Used for ctrl L & R    */
   char oline[MAXSTR];                             /* Used for restore line  */
   char nomore  = FALSE;                           /* Finished ok!           */
   char drawl   = TRUE;                            /* Draw the textline      */
   char first   = TRUE;                            /* Clear or not (if clear)*/
   if (form[3])                                    /* Process date or time   */
   {
      fixkeyin(line,len,colr);
      nomore = TRUE;
   }
   ins_pressed();                                  /* Show insert mode       */
   strcpy(oline,line);                             /* Save line to old       */
   max += start;                                   /* Adjust max to actual   */
   if (form[0])                                    /* Force upper (if upper) */
      line = strupr(line);
   if (form[2])                                    /* Force number only      */
      strcpy(line,numonly(line));
   textline(start,vr,NORM,colr,"%s",               /* Draw line              */
            substr(line,0,vislen));
   while (!nomore)                                 /* Loop until nomore      */
   {
      gotoxy(hr,vr);                               /* set cursor             */
      rkbd();                                      /* Get a key from bios    */
      drawl = TRUE;                                /* Ok to draw the line    */
      if (ch == NOKEY)                             /* If extended            */
      {
         first = FALSE;                            /* Clear not allowed      */
         nomore = TRUE;                            /* if key not is list then*/
         switch (extend)                           /* exit routine.          */
         {

            case SFF3     : if (hr == start)       /* Clear end of line      */
                               line[0] = NULL;
                            else
                               delete(line,hr-start+lpos,strlen(line)-(hr-start+lpos));
                            textline(start,vr,NORM,colr,"%s",space(vislen));
                            nomore = FALSE;
                            break;

            case SFF5     : strcpy(line,oline);    /* Restore line as origin */
                            textline(start,vr,NORM,colr,"%s",space(vislen));
                            hr   = start;
                            lpos = 0;
                            nomore = FALSE;
                            break;


            case LEFTARR  : if (hr > start)        /* Left arrow             */
                            {
                               hr--;
                               drawl = FALSE;
                            } else
                              if (lpos > 0)
                                 lpos--;
                            nomore = FALSE;
                            break;

            case RIGHTARR : if ((hr < max) && (hr < start + strlen(line)))
                            {
                               hr++;
                               drawl = FALSE;
                            } else
                               if (lpos + vislen < strlen(line))
                                  lpos++;
                            nomore = FALSE;
                            break;

            case HOMEKEY  : hr   = start;
                            lpos = 0;
                            nomore = FALSE;
                            break;

            case ENDKEY   : hr = start + strlen(line);
                            if (hr >= max)
                            {
                               lpos = hr - max;
                               hr   = max;
                            }
                            nomore = FALSE;
                            break;

            case DELKEY   : delete(line,hr-start+lpos,1);
                            nomore = FALSE;
                            break;

            case INSKEY   : ins_mode = (ins_mode) ? FALSE : TRUE;
                            ins_pressed();
                            drawl = FALSE;
                            nomore = FALSE;
                            break;


            case CTRLEFT  : if ((hr > start) || (lpos > 0))
                            {
                               lindex = hr - start + lpos;
                               do
                               {
                                  --lindex;
                                  if ((hr == start) && (lpos > 0))
                                     --lpos;
                                  else
                                     --hr;
                               } while (line[lindex] == ' ' && lindex > 0);
                               while ((lindex > 0) && (line[lindex] != ' '))
                               {
                                  if ((hr == start) && (lpos > 0))
                                     --lpos;
                                  else
                                     --hr;
                                  --lindex;
                               }
                               if (lindex > 0)
                                  ++hr;
                            }
                            nomore = FALSE;
                            break;

            case CTRLRT   : if ((hr <= max) && (hr + lpos < start + strlen(line))
                               && (hr - start + lpos < len))
                            {
                               lindex = hr - start + lpos;
                               while ((line[lindex] != ' ') && (lindex < strlen(line)))
                               {
                                  if (hr >= max)
                                     ++lpos;
                                  else
                                     ++hr;
                                  ++lindex;
                               }
                               while ((line[lindex] == ' ') && (lindex < strlen(line)))
                               {
                                  ++lindex;
                                  if (hr >= max)
                                     ++lpos;
                                  else
                                     ++hr;
                               }
                            }
                            nomore = FALSE;
                            break;
         }
      } else
      {
         if (ch == BACKSP || ch == ENTERKEY || ch == ESCKEY || ch==TAB || ch==BACKTAB)
         {
            switch (ch)
            {

               case BACKSP  : if (lpos > 0)
                              {
                                 --lpos;
                                 delete(line,hr-start+lpos,1);
                                 first = FALSE;
                              } else
                                 if (hr > start)
                                 {
                                    first = FALSE;
                                    --hr;
                                    delete(line,hr-start+lpos,1);
                                 }
                              break;

               case ENTERKEY : nomore = TRUE;
                               break;

               case ESCKEY   : nomore = TRUE;
                               break;
               case TAB:
               case BACKTAB:  nomore = TRUE; break;
            }
         }
         else
         {
            if (!form[2] ||
                form[2] && strchr("0123456789-+.",ch) != NULL)
            if (((strlen(line) < len) && ins_mode)
               || (!ins_mode && (hr - start + lpos < len)))
            {
               if (first && form[1])
               {
                  line[0] = NULL;
                  first = FALSE;
                  hr    = start;
                  lpos  = 0;
                  textline(start,vr,NORM,colr,"%s",space(vislen));
               }
               if (form[0])
                  ch = upchar(ch);
               if (und_score && (ch == ' '))
                  ch = '_';
               if (ins_mode)
               {
                 inschr(ch,line,hr-start+lpos);
                 if (hr < max)
                    ++hr;
                 else
                    ++lpos;
               } else
               {
                  if (line[hr-start+lpos] != NULL)
                     delete(line,hr-start+lpos,1);
                  inschr(ch,line,hr-start+lpos);
                  if (hr < max)
                     ++hr;
                  else
                     ++lpos;
               }
            }
         }
      }
      if (drawl)
         textline(start,vr,NORM,colr,"%s ",substr(line,lpos,vislen));
   }
   return(line);
}


