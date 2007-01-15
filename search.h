#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#define SEARCH_TERMS_BOX_HANDLE 5
#define SEARCH_PROGRESS_HANDLE 6
#define NO_RESULTS_HANDLE 7


#define MAX_SEARCH_TERMS 5
#define MAX_SEARCH_TERM_LENGTH 56
#define TOTAL_SEARCH_CONTEXT_LENGTH 66

#define VIEW_RESULTS_MODE_COUNT 2

typedef struct _SearchResult {
   unsigned long id;
   unsigned long lbahi;
   unsigned long lbalo;
   unsigned long offset;
   int searchTermIndex;
   char contextBefore[TOTAL_SEARCH_CONTEXT_LENGTH+1];
   int contextBeforeLength;
   char contextAfter[TOTAL_SEARCH_CONTEXT_LENGTH+1];
   int contextAfterLength;
   struct _SearchResult *next;
   struct _SearchResult *prev;
} SearchResult;


int searchTermsPrompt(char far *searchTerms[], int far *searchTermsCount);
void printResults(SearchResult *results, int highlightedIndex);
void freeAllSearchResults(SearchResult far *r);
SearchResult far *findLastSearchResult(SearchResult far *r);


#endif
