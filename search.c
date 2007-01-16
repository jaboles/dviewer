#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <alloc.h>
#include "constant.h"
#include "util.h"
#include "uiutils.h"
#include "search.h"
#include "ide.h"


// Starting y-coord and number of lines to display, for each mode.
int searchResultsModeStartY[] = {20, 14};
int searchResultsModeLineCount[] = {5, 11};
int currentSearchResultsMode = -1;
SearchResult far *resultCurrentlySelected = NULL;

extern char far *searchTerms[MAX_SEARCH_TERMS];
extern unsigned char far *bufferPtr;

/** Prompts the user to enter phrases to search for.
   Returns -1 if the user hit ESC to cancel, 0 if the user hit ENTER (search for a limited
      number of occurrences), or 1 if the user hit F10 (search for everything). */
int searchTermsPrompt(char far *searchTerms[], int far *searchTermsCount) {
   int i = 0;
   int entryReturnValue;
   *searchTermsCount = 0;

   notice(SEARCH_TERMS_BOX_HANDLE,10,5,70,20,NONE,EXPLODE,TRUE,"Search Terms",0,"");
   textline(12,7,NORM,BOXI,"Type words/phrases to search for. Use up/down arrows to");
   textline(12,8,NORM,BOXI,"change fields. Press ENTER to commence search.");

   for (i = 0; i < MAX_SEARCH_TERMS; i++) {
      get(12, 10+i*2, MAX_SEARCH_TERM_LENGTH-1, "%s", searchTerms[i], NULL);
   }
   entryReturnValue = entry(KEYEXIT);
   killmsg(SEARCH_TERMS_BOX_HANDLE);

   if (entryReturnValue < 0) {
      return -1;
   } else {
      for(*searchTermsCount = 0; strlen(searchTerms[*searchTermsCount])>0; (*searchTermsCount)++);
      if (ch == 13) { // User pressed ENTER
         return 0;
      } else if (extend == 68) { // User pressed F10
         return 1;
      }
      // Else search for a limited number of occurrences.
      return 0;
   }
}


void printResults(SearchResult *results, int highlightedIndex) {
   int i, j;
   SearchResult *r = results;
   char condensedSearchTerm[16];
   char srNumStr[10];
   char srTotalStr[10];
   int startY = searchResultsModeStartY[currentSearchResultsMode];
   int lineCount = searchResultsModeLineCount[currentSearchResultsMode];
   memset(srNumStr, 0, 10);
   memset(srTotalStr, 0, 10);

   setcolor(0,7);
   gotoxy(1, startY);
   sprintf(srNumStr, "%lu", results->id + highlightedIndex + 1); // The index of the currently-highlighted result
   sprintf(srTotalStr, "%lu", findLastSearchResult(results)->id + 1); // The total number of results
   cprintf(" RESULTS (%s of %s)", srNumStr, srTotalStr);
   for (i = 0; i < 9-strlen(srNumStr)-strlen(srTotalStr); i++) cprintf("Ä");
   cprintf("ESC:Hide F6:Continue Search \x18/\x19:Scroll ENTER:GoTo ");
   cprintf(currentSearchResultsMode==VIEW_RESULTS_MODE_COUNT-1? "-:Less" : "+:More");
   clreol();
   for (i = 0; i < lineCount && r != NULL; i++, r = r->next) {
      gotoxy(1, startY+i+1);
      setcolor(10, i==highlightedIndex? 4 : 1); // Green
      for (j = 0; j < r->contextBeforeLength; j++) cprintf("%c", MAKE_DISPLAY_FRIENDLY(r->contextBefore[j]));
      setcolor(14, i==highlightedIndex? 4 : 1); // Yellow
      condense(searchTerms[r->searchTermIndex], condensedSearchTerm, 10);
      cprintf("%s", condensedSearchTerm);
      setcolor(10, i==highlightedIndex? 4 : 1); // Green
      for (j = 0; j < r->contextAfterLength; j++) cprintf("%c", MAKE_DISPLAY_FRIENDLY(r->contextAfter[j]));
      setcolor(15,1); // White
      clreol();
   }
   for (; i < lineCount; i++) {
      // Blank the rest of the lines.
      printf("\n");
      clreol();
   }
}



void freeAllSearchResults(SearchResult far *r) {
   while (r->next != NULL) {
      r = r->next;
      free(r->prev);
   }
   free(r);
}

SearchResult far *findLastSearchResult(SearchResult far *r) {
   while (r->next != NULL) r = r->next;
   return r;
}



