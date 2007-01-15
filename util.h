#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#define PRINT_SECTOR(lbahi,lbalo) cprintf("%.0lf",(((double)lbahi)*pow(2,32))+(double)lbalo)
#define MAKE_DISPLAY_FRIENDLY(c) ((c==0 || c==7 || c==8 || c==13 || c==10 || c==9)? '.' : c)
#define MAX(a,b) (a>b? a : b)
#define MIN(a,b) (a<b? a : b)

void ShowAll( void );
int checkerror(int rc);
void setcolor(int fore, int back);
void bigmemset(char far *fptr, int c, long sz);
char far *condense(const char far *searchTerm, char far *output, int length);

#endif
