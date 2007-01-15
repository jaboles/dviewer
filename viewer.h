
#define DISPLAY_MODES_COUNT 2
#define DM_NONE 0
#define DM_ASCII 1
#define DM_HEX 2


#define SECTOR_PROMPT_HANDLE 3
#define LIST_SELECTION_BOX_HANDLE 4
#define GENERIC_MESSAGE_HANDLE 8
#define SECTOR_FILENAME_PROMPT_HANDLE 9


typedef struct _displaymode {
   int linesToDisplay;
   int bytesPerLine;
   int displayStartX;
   int displayStartY;
   int type[2];
   int addspaceafter[2];
} displaymode;


void refresh(void);
void incrementView(unsigned long far *lbahi, unsigned long far *lbalo, int far *offset);
void decrementView(unsigned long far *lbahi, unsigned long far *lbalo, int far *offset);
int ListSelectionBox(const char far *header, const char far *description1, const char far *description2, opts far optionList[], int optionCount);
int sectorPrompt(unsigned long *lbahi, unsigned long *lbalo, unsigned long maxLBAhi, unsigned long maxLBAlo);
int driveSelectionMenu(const Drive far *ide_drives, const int ideDrivesFound);
SearchResult * startSearch(Drive far *drive, char far *searchTerms[], unsigned long startLBAhi, unsigned long startLBAlo, unsigned long *gotToLBAhi, unsigned long *gotToLBAlo, int maxResults);
void searchAgain(Drive far *drive, int viewResultsOrNot, char far *searchTerms[]);
void displayView(const Drive far *drive, const unsigned long lbahi, const unsigned long lbalo, const int offset);
void viewResults(SearchResult *results, SearchResult *startFrom);
void header(void);
void footer(void);
void domsg(char far *msg);
void saveSector(Drive far *, unsigned long lbahi, unsigned long lbalo);


