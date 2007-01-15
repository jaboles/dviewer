#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <alloc.h>
#include <mem.h>
#include <time.h>
#include "util.h"
#include "ataio.h"

//**********************************************

// a little function to display all the error
// and trace information from the driver

void ShowAll( void )

{
   unsigned char * cp;
   int rwcount = 1;

   printf( "ERROR !\n" );

   // display the command error information
   trc_err_dump1();           // start
   while ( 1 )
   {
      cp = trc_err_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
      if (++rwcount == 15)
      {
         printf("Press any key\n");
         getch();
         rwcount = 1;
      }
   }
   // display the command history
   trc_cht_dump1();           // start
   while ( 1 )
   {
      cp = trc_cht_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
      if (++rwcount == 15)
      {
         printf("Press any key\n");
         getch();
         rwcount = 1;
      }
   }
   // display the low level trace
   trc_llt_dump1();           // start
   while ( 1 )
   {
      cp = trc_llt_dump2();   // get and display a line
      if ( cp == NULL )
         break;
      printf( "* %s\n", cp );
      if (++rwcount == 15)
      {
         printf("Press any key\n");
         getch();
         rwcount = 1;
      }
   }

   // now clear the command history and low level traces
   trc_cht_dump0();     // zero the command history
   trc_llt_dump0();     // zero the low level trace
}

int checkerror(int rc)
{
   if ( rc )
      ShowAll();
   else
   {
      trc_cht_dump0();     // zero the command history
      trc_llt_dump0();     // zero the low level trace
   }
   return rc;
}

void setcolor(int fore, int back)
{
   textattr(fore + back * 16);
}


void bigmemset(char far *fptr, int c, long sz)
{
   const unsigned int blocksize = 65534U;
   unsigned int i = 0;
   char huge *ptr;

   ptr = fptr;
   while (sz > 0)
   {
      i = (sz > blocksize) ? blocksize : (unsigned int)sz;
      _fmemset((char huge *)ptr, c, i);
      ptr = ((char huge *)ptr + i);
      sz -= i;
   }
   //getch();
}
char far *condense(const char far *searchTerm, char far *output, int length) {
   int str1length;
   int str2length;

   memset(output, 0, length+1);
   if (strlen(searchTerm) > length) {
      str1length = (length-3) / 2;
      str2length = length - 3 - str1length;
      strncpy(output, searchTerm, str1length);
      strncpy(output+str1length, "...", 3);
      strncpy(output+str1length+3, searchTerm+(strlen(searchTerm)-str2length), str2length);
   } else {
      strncpy(output, searchTerm, MIN(strlen(searchTerm), length));
   }
   return output;
}


