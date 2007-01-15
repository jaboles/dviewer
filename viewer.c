#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <alloc.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "types.h"
#include "constants.h"
#include "ataio.h"
#include "ide.h"
#include "util.h"
#include "uiutils.h"
#include "search.h"
#include "viewer.h"
#include "lowlvl.h"
#include "pci_lib.h"

char far *helpText[] = {
   "While Examining Drive:",
   "   ESC             Quit",
   "   F1              Display this screen",
   "   F2              Select drive to examine",
   "   \x18/\x19             Scroll up/down one line",
   "   PgUp/PgDn       Jump forward/backward one sector",
   "   Ctrl-PgUp/PgDn  Jump to next/prev sector boundary",
   "   F3              Change display mode between ASCII and Hex",
   "   F4              Go to sector number",
   "   F5              Search for text",
   "   F6              Continue a halted search",
   "   F7              Display search results",
   "   F8              Save current sector to disk",
   "",
   "While Viewing Search Results:",
   "   ESC             Hide search results",
   "   +/-             Display more/fewer search results",
   "   ENTER           Go to highlighted occurrence of search text",
   "   F6              Continue a halted search"
};
int helpTextLineCount = 19;

unsigned char far *bufferPtr;
unsigned char far *diskViewBufferPtr;

displaymode displaymodes[2];
int currentDisplayMode = 0;
char far *searchTerms[MAX_SEARCH_TERMS];

extern int searchResultsModeStartY[];
extern int searchResultsModeLineCount[];
extern int currentSearchResultsMode;
extern SearchResult far *resultCurrentlySelected;

IDEChannel far *ide_channels;
Drive far *drives;
int sd = 0;
SearchResult far *searchResults = NULL;

// per-drive
unsigned long lastLBAhi;
unsigned long lastLBAlo;
unsigned long currentLBAhi;
unsigned long currentLBAlo;
unsigned long currentSearchLBAhi;
unsigned long currentSearchLBAlo;
int currentOffset;



int main(int argc, char* argv[]) {
   int ch, i, searchTermsCount = 0, rc = 0;
   int ideChannelsFound = 0;
   int drivesFound = 0;
   int programJustStarted = 1;

   displaymodes[0].linesToDisplay = 19;
   displaymodes[0].bytesPerLine = 64;
   displaymodes[0].displayStartX = 1;
   displaymodes[0].displayStartY = 3;
   displaymodes[0].type[0] = DM_ASCII;
   displaymodes[0].addspaceafter[0] = 0;
   displaymodes[0].type[1] = DM_NONE;
   displaymodes[0].addspaceafter[1] = 0;
   displaymodes[1].linesToDisplay = 19;
   displaymodes[1].bytesPerLine = 16;
   displaymodes[1].displayStartX = 1;
   displaymodes[1].displayStartY = 3;
   displaymodes[1].type[0] = DM_HEX;
   displaymodes[1].addspaceafter[0] = 1;
   displaymodes[1].type[1] = DM_ASCII;
   displaymodes[1].addspaceafter[1] = 0;

   bufferPtr = (unsigned char far *)farmalloc(BUFFER_SIZE);
   diskViewBufferPtr = (unsigned char far *)farmalloc(SMALL_BUFFER_SIZE);
   ide_channels = (IDEChannel far *)farmalloc(sizeof(IDEChannel) * MAX_IDE_CHANNELS);
   drives = (Drive far *)farmalloc(sizeof(Drive) * MAX_IDE_DRIVES);
   for (i = 0; i < MAX_SEARCH_TERMS; i++) {
      searchTerms[i] = (char far *)farmalloc(MAX_SEARCH_TERM_LENGTH);
      memset(searchTerms[i],0,MAX_SEARCH_TERM_LENGTH);
   }

   setcolor(15,1);
   clrscr();

   coltable[NORM] = cval(15,1);
   coltable[HIGH] = cval(1,7);
   coltable[BOXL] = cval(0,7);
   coltable[BOXI] = cval(0,7);
   coltable[OTHER] = cval(15,4);

   header();
   
   if (!pcibios_present()) {
      printf("No PCI BIOS found!\n");
      return -1;
   } else if (scan_devices(ide_channels, &ideChannelsFound) < 0) {
      printf("Error scanning for IDE.\n");
      return -1;
   }

   // Initial parameters for the ATA driver.
   reg_buffer_size = BUFFER_SIZE;
   pio_xfer_width = 32;

   drivesFound = bios_detect(drives, 0x00, 2, 0);
   //exit(0);

   drivesFound += detect_ide_drives(ide_channels, ideChannelsFound, drives, drivesFound);

   while (1==1) {
      // Get a character from the user. If the program was just launched,
      // don't wait for a character input -- go straight to the drive selection
      // list.
      if (programJustStarted == 0) ch = getch();

      if (ch == 27) { // escape
         break;
      } else if (ch == 72 && // Up arrow
            (cmp_lba(currentLBAhi,currentLBAlo,0,0) > 0 || currentOffset > 0)) {
         // If we're at the 2nd or later sector, or if offset is nonzero, we can go back.
         decrementView(&currentLBAhi, &currentLBAlo, &currentOffset);
      } else if (ch == 73 && // Page Up
            (cmp_lba(currentLBAhi, currentLBAlo, 0, 0) > 0)) {
         // If we're at the 2nd or later sector, we can go back one.
         add_lba(&currentLBAhi, &currentLBAlo, -1);
      } else if (ch == 132 && // Control Page Up
            (cmp_lba(currentLBAhi, currentLBAlo, 0, 0) > 0 || currentOffset > 0)) {
         if (currentOffset > 0) {
            currentOffset = 0;
         } else {
            add_lba(&currentLBAhi, &currentLBAlo, -1);
         }
      } else if (ch == 71) { // Home
         currentLBAlo = 0;
         currentLBAhi = 0;
         currentOffset = 0;
      } else if (ch == 80 &&  // Down arrow
            (cmp_lba(lastLBAhi, lastLBAlo, currentLBAhi, currentLBAlo) > 0 ||
            currentOffset < (&drives[sd].bps - displaymodes[currentDisplayMode].bytesPerLine))) {
         // If we're before the last sector, or if offset is not at the maximum, we can still go forward.
         incrementView(&currentLBAhi, &currentLBAlo, &currentOffset);
      } else if (ch == 81 &&  // Page Down
            (cmp_lba(lastLBAhi, lastLBAlo, currentLBAhi, currentLBAlo) > 0)) {
         // If we're before the last sector, we can go forward one.
         add_lba(&currentLBAhi, &currentLBAlo, 1);
         
      } else if (ch == 118 && (cmp_lba(lastLBAhi, lastLBAlo, currentLBAhi, currentLBAlo) > 0)) {
         // CtrlPgDn: Jump to next sector boundary.
         add_lba(&currentLBAhi, &currentLBAlo, 1);
         currentOffset = 0;

      } else if (ch == 79) {
         // End: Jump to last sector.
         currentLBAlo = lastLBAlo;
         currentLBAhi = lastLBAhi;
         currentOffset = 0;

      } else if (ch == 59) {
         // F1: Help
         help();
         header();

      } else if (ch == 60 || programJustStarted > 0) {
         // F2: Select Drive
         rc = driveSelectionMenu(drives, drivesFound);

         if (rc < 0 && programJustStarted > 0) {
            // If this is the initial drive selection box (i.e. just after starting
            // the viewer) and the user cancels it, quit the program.
            break;
         } else if (rc >= 0) {
            // Otherwise, initialise the drive and reset variables if the user
            // actually selected a drive.
            sd = rc;
            activate_drive(&drives[sd]);

            // Reset the current and max LBA variables.
            currentLBAhi = 0;
            currentLBAlo = 0;
            currentOffset = 0;
            currentSearchLBAhi = 0;
            currentSearchLBAlo = 0;
            lastLBAhi = drives[sd].lbahi;
            lastLBAlo = drives[sd].lbalo;
            // The last LBA is the # of sectors on the drive minus one.
            add_lba(&lastLBAhi, &lastLBAlo, -1);
         }
         // If the program was just started, the key handler below won't refresh.
         // Manually do one.
         if (programJustStarted > 0) {
            programJustStarted = 0;
            refresh();
            footer();
         }

      } else if (ch == 61) {
         // F3: Change the display mode.
         //setcolor(15,1);
         clrs(displaymodes[currentDisplayMode].displayStartX, displaymodes[currentDisplayMode].displayStartY,
            80, 1+displaymodes[currentDisplayMode].displayStartY+displaymodes[currentDisplayMode].linesToDisplay);
         currentDisplayMode = (currentDisplayMode + 1) % DISPLAY_MODES_COUNT;

      } else if (ch == 62) { // F4
         // F4: go to sector. Prompt the user to enter a sector. If they
         //     type in a number (i.e. sectorPrompt()==0) refresh the display.
         if (sectorPrompt(&currentLBAhi, &currentLBAlo, lastLBAhi, lastLBAlo) >= 0)
            displayView(&drives[sd], currentLBAhi, currentLBAlo, currentOffset);

      } else if (ch == 63) {
         // F5: Search
         rc = searchTermsPrompt(searchTerms, &searchTermsCount);
         if (rc >= 0 && searchTermsCount == 0) {
            domsg("No search terms were entered.");
         } else if (searchTermsCount > 0) {
            if (searchResults != NULL) freeAllSearchResults(searchResults);
            if (rc == 0) {
               searchResults = startSearch(&drives[sd], searchTerms, 0, 0, &currentSearchLBAhi, &currentSearchLBAlo, MAX_SEARCH_RESULTS);
            } else if (rc == 1) {
               searchResults = startSearch(&drives[sd], searchTerms, 0, 0, &currentSearchLBAhi, &currentSearchLBAlo, -1);
            } else {
               fatal("Error","Return code from searchTermsPrompt is an invalid value!");
            }
            if (searchResults == NULL) {
               domsg("No results were found");
            } else {
               viewResults(searchResults, searchResults);
               refresh();
               footer();
            }
         }

      } else if (ch == 64) {
         // F6: Continue Search
         if (searchTermsCount == 0) {
            domsg("Nothing to search for yet.");
         } else {
            searchAgain(&drives[sd], 1, searchTerms);
         }

      } else if (ch == 65) {
         // F7: Display results
         if (searchResults == NULL) {
            domsg("No results to display.");
         } else {
            viewResults(searchResults, NULL);
            refresh();
            footer();
         }

      } else if (ch == 66) {
         // F8: Save sector to disk
         saveSector(&drives[sd], currentLBAhi, currentLBAlo);

      } else if (ch == 0) {
         // do nothing
      } else {
         //printf("%d\n", ch);
      }

      // If any of the keys were pressed which should change the view, refresh.
      if (ch==59 || ch==71 || ch==72 || ch==73 || ch==79 || ch==80 || ch==81 || ch==60 || ch==61 || ch==132 || ch==118) {
         refresh();
         footer();
      }
   }
   farfree(bufferPtr);
   farfree(diskViewBufferPtr);
   farfree(drives);
   farfree(ide_channels);
   // Set screen colour back to white-on-black and clear screen.
   setcolor(7,0);
   clrscr();
   return 0;
}

void refresh() {
   displayView(&drives[sd], currentLBAhi, currentLBAlo, currentOffset);
}

void incrementView(unsigned long far *lbahi, unsigned long far *lbalo, int far *offset) {
   int incrementAmount = displaymodes[currentDisplayMode].bytesPerLine;
   if ((*offset + incrementAmount) % drives[sd].bps < incrementAmount) {
      *offset = 0;
      add_lba(lbahi, lbalo, 1);
   } else {
      *offset += incrementAmount;
   }
}

void decrementView(unsigned long far *lbahi, unsigned long far *lbalo, int far *offset) {
   int decrementAmount = displaymodes[currentDisplayMode].bytesPerLine;
   if ((*offset - decrementAmount) < 0) {
      *offset = drives[sd].bps - decrementAmount;
      add_lba(lbahi, lbalo, -1);
   } else {
      *offset -= decrementAmount;
   }
}

int ListSelectionBox(const char far *header, const char far *description1, const char far *description2, opts far optionList[], int optionCount) {
   int optionLength, i;
   word x1, y1, x2, y2;
   int ch;
   int si = 0;

   optionLength = strlen(optionList[0]);

   x1 = (80 - (optionLength + 4)) / 2;
   y1 = (25 - (optionCount + 4)) / 2;
   x2 = x1 + (strlen(header)>optionLength+4?  strlen(header) : optionLength+4);
   y2 = y1 + optionCount + 4;

   notice(LIST_SELECTION_BOX_HANDLE,x1,y1,x2,y2,NONE,EXPLODE,TRUE,header,0,"");

   textline(x1+2, y1+2, NORM, BOXI, description1);
   textline(x1+2, y1+3, NORM, BOXI, description2);
   for (i = 0; i < optionCount; i++) {
      textline(x1+2, y1+i+4, NORM, i==si? OTHER : BOXI, optionList[i]);
   }

   while (1==1) {
      while (!kbhit());
      ch = getch();
      switch (ch) {
         case 27:  // Escape
            killmsg(LIST_SELECTION_BOX_HANDLE); return -1;
         case 72: // up arrow
            if (si > 0) si--; break;
         case 80: // dn arrow
            if (si < optionCount-1) si++; break;
         case 13: // enter
            killmsg(LIST_SELECTION_BOX_HANDLE); return si;
         case 0: break;
         //default: printf("%d\n", ch);
      }
      for (i = 0; i < optionCount; i++) {
         textline(x1+2, y1+i+4, NORM, i==si? OTHER : BOXI, optionList[i]);
      }
   }
}

int driveSelectionMenu(const Drive far *d, const int drivesFound) {
   int i, selectedDrive;
   opts far *driveList;
   const char far *str1 = "Select Drive";
   const char far *str2 = "   Sectors    Size MB  Model                    Serial Number";
   const char far *str3 = "---------- ----------  --------------------  ----------------";

   driveList = (opts far *)farmalloc(sizeof(opts) * drivesFound);

   for (i = 0; i < drivesFound; i++) {
      memset(driveList[i], 0, 60);
      sprintf(driveList[i], "%10.0lf %10.2f  %-17.17s %-20.20s",
         (((double)d[i].lbahi)*pow((double)2,(double)32))+(double)d[i].lbalo,
         (((double)d[i].lbahi)*pow((double)2,(double)32))+(double)d[i].lbalo / 2048.0,
         (d[i].type==DISK_TYPE_IDE? d[i].ide.model : (d[i].type==DISK_TYPE_FLOPPY? "Floppy" : "<unknown>")),
         (d[i].type==DISK_TYPE_IDE? d[i].ide.serial_no : ""));
      driveList[i][61] = '\0';
   }

   selectedDrive = ListSelectionBox(str1,str2,str3, driveList,drivesFound);
   farfree(driveList);
   return selectedDrive;
}

// Prompts the user to enter a sector number. Returns 0 if a sector number
// was entered, and -1 if the user hit escape. maxLBA is the highest LBA
// that the user is allowed to enter. lba is where the entered sector number
// will be stored.
int sectorPrompt(unsigned long *lbahi, unsigned long *lbalo, unsigned long maxLBAhi, unsigned long maxLBAlo) {
   char inputLBAstr[20];
   double inputLBA;
   int i,j;
   int finished = 0;
   unsigned long inputLBAhi, inputLBAlo;

   // indicates what the user did. 0 = completed the entry. 1 = they hit ESC to abort
   int entryReturnValue = 0;

   // Show the popup message requesting user entry
   notice(SECTOR_PROMPT_HANDLE,25,10,56,15,NONE,EXPLODE,TRUE,"Go To",0,"");
   textline(27,12,NORM,BOXI,"Enter a sector number between");
   textline(27,13,NORM,BOXI,"0 and ");
   PRINT_SECTOR(maxLBAhi,maxLBAlo);
   while (finished == 0) {
      memset(inputLBAstr, 0, 15);
      // Get sector number from user.
      get(27, 14, 19, "%s", inputLBAstr, NULL);
      entryReturnValue = entry(0);

      if (entryReturnValue < 0) break;

      // Go through each digit. If it's out of range, abort, otherwise multiply
      // it by the appropriate power of ten and sum them.
      inputLBA = 0;
      j = 0;
      for (i = strlen(inputLBAstr); i > 0; i--, j++) {
         if (inputLBAstr[i-1] >= '0' && inputLBAstr[i-1]<='9') {
            // Digit is in range (0-9), multiply by the appropriate power of 10.
            inputLBA += (inputLBAstr[i-1]-'0') * pow10(j);
            finished = 1;
         } else {
            // Digit out of range, beep and stop.
            finished = 0;
            errbeep();
            break;
         }
      }

      inputLBAhi = (unsigned long)(inputLBA / pow(2,32));
      inputLBAlo = (unsigned long)(inputLBA - (inputLBAhi * pow(2,32)));
      if (cmp_lba(inputLBAhi,inputLBAlo,maxLBAhi,maxLBAlo) > 0) {
         errbeep();
         finished = 0;
      }
   }
   // Get rid of the popup box.
   killmsg(SECTOR_PROMPT_HANDLE);

   if (entryReturnValue < 0) {
      // User aborted, do nothing
   } else {
      // User entered a sector number.
      *lbahi = inputLBAhi;
      *lbalo = inputLBAlo;
   }
   return entryReturnValue;
}

void displayView(const Drive far *drive, const unsigned long lbahi, const unsigned long lbalo, const int startOffset) {
   static unsigned long cachedLBAhi = 0;
   static unsigned long cachedLBAlo = 0;
   unsigned long lbacaphi = drive->lbahi;// The drive's capacity (NOT the last LBA)
   unsigned long lbacaplo = drive->lbalo;
   int rc, i, j, k, ch;
   long offset = startOffset;
   displaymode *d = &displaymodes[currentDisplayMode];
   int fullDataLinesToDisplay; // number of lines to display (dependent on quantity of data avail)
   int linesToDisplay; // number of actual lines to display (as above, but possibly reduced by the search box at the bottom of screen)
   int viewportLines;
   int sectorsToRead;
   long diff;
   long diffLines;


      if (diff_lba(lbacaphi,lbacaplo,lbahi,lbalo,&diff)==0 && diff < 10) {
         sectorsToRead = diff;
         diffLines = (diff*512-startOffset)/d->bytesPerLine; // The number of lines required to display <diff> sectors from offset <offset>
         fullDataLinesToDisplay = (diffLines < d->linesToDisplay)?  diffLines : d->linesToDisplay;
      } else {
         sectorsToRead = 10;
         fullDataLinesToDisplay = d->linesToDisplay;
      }

   // If the data display is going to overrun the search results being displayed, reduce it.
   if (currentSearchResultsMode >= 0 &&
         (d->displayStartY + fullDataLinesToDisplay + 1) > searchResultsModeStartY[currentSearchResultsMode]) {
      viewportLines = searchResultsModeStartY[currentSearchResultsMode] - d->displayStartY - 1;
      if (viewportLines>d->linesToDisplay) viewportLines = d->linesToDisplay;
   } else {
      viewportLines = d->linesToDisplay;
   }
   linesToDisplay = MIN(viewportLines, fullDataLinesToDisplay);

   gotoxy(d->displayStartX,d->displayStartY);
   setcolor(15,1);
   cprintf("Displaying 0x%04X bytes, starting at sector ", d->bytesPerLine * linesToDisplay);
   PRINT_SECTOR(lbahi,lbalo);
   cprintf(" offset 0x%03X.", offset);

   clreol();
   printf("\n");

   if (diff_lba(lbahi,lbalo,cachedLBAhi,cachedLBAlo,&diff) == 0 && diff >= 0 && diff < (sectorsToRead-3) && (lbalo != 0 || lbahi != 0)) {
      // We retrieved that sector recently. No need to fetch it again.
   } else {
      // It isn't cached, need to fetch it.
      memset(diskViewBufferPtr, 0, SMALL_BUFFER_SIZE);
      rc = do_read(drive, lbahi, lbalo, sectorsToRead, diskViewBufferPtr);
      if (rc != 0) {
         printf("Read failed with return code: %d\n", rc);
         exit(1);
      }
      cachedLBAhi = lbahi;
      cachedLBAlo = lbalo;
      diff = 0;
   }
   for (i = 0; i < linesToDisplay; i++) {
      if (offset % 512 == 0) setcolor(14,1); else setcolor(15,1);
      cprintf("0x%03X  ", offset % 512);
      setcolor(15, 1);
      for (j = 0; j < 2; j++) {
         for (k = 0; k < d->bytesPerLine; k++) {
            ch = diskViewBufferPtr[offset + diff*512 + k];
            if (d->type[j] == DM_ASCII) {
               cprintf("%c", MAKE_DISPLAY_FRIENDLY(ch));
            } else if (d->type[j] == DM_HEX) {
               cprintf("%02X", ch);
            }
            if (d->addspaceafter[j] > 0) cprintf(" ");
         }
         cprintf(" ");
      }
      // Blank up to the end of the line.
      clreol();
      printf("\n");
      offset += d->bytesPerLine;
   }
   // If not displaying a full number of lines, print a message indicating
   // the end of the drive, then blank the rest.
   if (linesToDisplay < viewportLines) {
      printf("       ---=== END OF DRIVE ===---");
      clreol();
      printf("\n");
   }
   for (i = linesToDisplay+1; i < viewportLines; i++) {
      clreol();
      printf("\n");
   }
}

SearchResult * startSearch(Drive far *drive, char far *searchTerms[], unsigned long startLBAhi, unsigned long startLBAlo, unsigned long *gotToLBAhi, unsigned long *gotToLBAlo, int maxResults) {
   char far *substrptr;
   SearchResult far *firstResult = NULL;
   int rc;
   int sectorsToRead;
   int bufOffset;
   int sectorOffsetInBuffer;
   short byteOffsetInSector;
   short contextBeforeLength;
   short contextAfterLength;
   long diff;
   long i;
   unsigned long instancesFound = 0;
   SearchResult *r;
   unsigned long tmpLBAhi;
   unsigned long tmpLBAlo;
   unsigned long cLBAhi = startLBAhi;
   unsigned long cLBAlo = startLBAlo;
   unsigned long endLBAhi = drive->lbahi;
   unsigned long endLBAlo = drive->lbalo;
   //char* blah[] = {"-", "\\", "|", "/"};
   char *blah[] = {"-  *\xF9\xF9", "/  \xF9*\xF9", "|  \xF9\xF9*", "\\  \xF9*\xF9"};
   unsigned char far *bufferPtrStr;
   time_t lastUpdate = 0;
   short progressIndicatorIndex = 0;

   notice(SEARCH_PROGRESS_HANDLE, 30, 8, 51, 16, NONE, EXPLODE, TRUE, "Searching...", 0, "");
   bufferPtrStr = (unsigned char far *)farmalloc(65536L);

   add_lba(&endLBAhi, &endLBAlo, -1);

   while ((rc = diff_lba(endLBAhi, endLBAlo, cLBAhi, cLBAlo, &diff))==1 || diff > 0) {
      if (lastUpdate < time(NULL)) {
         textline(32, 10, NORM, BOXI, "Searching    %s", blah[progressIndicatorIndex++ % 4]);
         textline(32, 11, NORM, BOXI, "(@ %.0lf)", (((double)cLBAhi)*pow(2,32))+(double)cLBAlo);
         textline(32, 13, NORM, BOXI, "%.0lf instances found", (double)instancesFound);
         textline(32, 15, NORM, BOXI, "Hit ESC to abort.");
         lastUpdate = time(NULL);
      }

      // Check if user hit Escape to abort search
      if (kbhit() && getch() == 27) {
         free(bufferPtrStr);
         *gotToLBAhi = cLBAhi;
         *gotToLBAlo = cLBAlo;
         killmsg(SEARCH_PROGRESS_HANDLE);
         return firstResult;
      }

      if (cmp_lba(cLBAhi, cLBAlo, startLBAhi, startLBAlo) == 0) {
         bufOffset = 0;
      } else {
         bufOffset = 512;
      }
      sectorsToRead = (rc==1 || diff>126)? 126 : diff;

      // Read into the buffer
      memset(bufferPtr+bufOffset, 0, sectorsToRead*512);
      rc = do_read(drive, cLBAhi, cLBAlo, sectorsToRead, bufferPtr+bufOffset);

      // Copy into the 2nd buffer.
      memcpy(bufferPtrStr+bufOffset, bufferPtr+bufOffset, sectorsToRead*512);

      // Set all the nulls to spaces in the 2nd buffer
      for (i = 0; i < sectorsToRead*512L; i++) {
         if (bufferPtrStr[i+bufOffset] == '\0') bufferPtrStr[i+bufOffset] = ' ';
      }

      // Set the last char to NULL.
      bufferPtrStr[sectorsToRead*512 + bufOffset] = '\0';

      for (i = 0; i < MAX_SEARCH_TERMS; i++)
      if (strlen(searchTerms[i]) > 0) {
         // For each search term, try searching.
         for (substrptr = strstr(bufferPtrStr, searchTerms[i]); substrptr != NULL; substrptr = strstr(substrptr + 1, searchTerms[i])) {
            // If bufOffset is 512, and substrptr < bufferPtr, then the search term was found in the last
            // sector of the previous read (and could straddle the sector boundary).
            sectorOffsetInBuffer = (substrptr-bufferPtrStr)<bufOffset?  -1 : (substrptr-bufferPtrStr-bufOffset) / 512;
            byteOffsetInSector = (substrptr-bufferPtrStr) % 512;
            // Get the sector number that the substring occurs in. It's the current start LBA of the
            // disk read, plus the sector offset within the buffer.
            tmpLBAhi = cLBAhi;
            tmpLBAlo = cLBAlo;
            add_lba(&tmpLBAhi, &tmpLBAlo, sectorOffsetInBuffer);
            if (firstResult == NULL) {
               firstResult = (SearchResult *)malloc(sizeof(SearchResult));
               r = firstResult;
               r->prev = NULL;
            } else {
               r->next = (SearchResult *)malloc(sizeof(SearchResult));
               r->next->prev = r;
               r = r->next;
            }
            r->id = instancesFound;
            r->lbahi = tmpLBAhi;
            r->lbalo = tmpLBAlo;
            r->offset = byteOffsetInSector;
            r->searchTermIndex = i;

            // Extract search 'context' strings (i.e. stuff before and after the occurrence of the search text)
            memset(r->contextBefore, 0, TOTAL_SEARCH_CONTEXT_LENGTH+1);
            memset(r->contextAfter, 0, TOTAL_SEARCH_CONTEXT_LENGTH+1);
            contextBeforeLength = MIN(TOTAL_SEARCH_CONTEXT_LENGTH/2, byteOffsetInSector);
            contextAfterLength = MIN(TOTAL_SEARCH_CONTEXT_LENGTH/2, 512-byteOffsetInSector-strlen(searchTerms[i]));
            if (contextBeforeLength < TOTAL_SEARCH_CONTEXT_LENGTH/2) {
               contextAfterLength = TOTAL_SEARCH_CONTEXT_LENGTH - contextBeforeLength;
            } else if (contextAfterLength < TOTAL_SEARCH_CONTEXT_LENGTH/2) {
               contextBeforeLength = TOTAL_SEARCH_CONTEXT_LENGTH - contextAfterLength;
            }

            memcpy(r->contextBefore, bufferPtr+(substrptr-bufferPtrStr)-contextBeforeLength, contextBeforeLength);
            memcpy(r->contextAfter, bufferPtr+(substrptr-bufferPtrStr)+strlen(searchTerms[i]), contextAfterLength);
            r->contextBeforeLength = contextBeforeLength;
            r->contextAfterLength = contextAfterLength;
            r->next = NULL;

            instancesFound++;

            // If we have already found a sufficient number of results, exit.
            if (maxResults > 0 && instancesFound >= maxResults) {
               free(bufferPtrStr);
               *gotToLBAhi = tmpLBAhi;
               *gotToLBAlo = tmpLBAlo;
               killmsg(SEARCH_PROGRESS_HANDLE);
               return firstResult;
            }
         }
      }
      add_lba(&cLBAhi, &cLBAlo, sectorsToRead);
      memcpy(bufferPtr, bufferPtr + ((sectorsToRead-1)*512) + bufOffset, 512);
      memcpy(bufferPtrStr, bufferPtrStr + ((sectorsToRead-1)*512) + bufOffset, 512);
   }

   free(bufferPtrStr);

   // We got to the end.
   *gotToLBAhi = endLBAhi;
   *gotToLBAlo = endLBAlo;

   killmsg(SEARCH_PROGRESS_HANDLE);
   return firstResult;
}

void searchAgain(Drive far *drive, int viewResultsOrNot, char far *searchTerms[]) {
   SearchResult far *r = NULL, *r2 = NULL;
   int i;
   unsigned long endLBAhi = drive->lbahi;
   unsigned long endLBAlo = drive->lbalo;

   add_lba(&endLBAhi, &endLBAlo, -1);

   if (cmp_lba(currentSearchLBAhi, currentSearchLBAlo, endLBAhi, endLBAlo) == 0) {
      domsg("Cannot continue - end of drive has been reached");
   } else {
      r = startSearch(drive, searchTerms, currentSearchLBAhi, currentSearchLBAlo, &currentSearchLBAhi, &currentSearchLBAlo, MAX_SEARCH_RESULTS);
      if (r != NULL && searchResults != NULL) {
         // We have found a new list of occurrences. Append it to the old list.
         // r's previous needs to be set before searchResults' last, otherwise
         // findlast(searchResults) will return the last item in r and r will be linked
         // back to itself.
         r->prev = findLastSearchResult(searchResults);
         findLastSearchResult(searchResults)->next = r;
         // Tag the new results to follow on in sequence from the previous result list.
         for (r2 = r, i = 0; r2 != NULL; r2 = r2->next, i++) {
            r2->id = r->prev->id + i + 1;
         }
      } else if (r != NULL) {
         // We have found a new list of occurrences, but none were found in a previous search.
         // Treat this new list as the entire list.
         searchResults = r;
      }
      if (r != NULL && viewResultsOrNot != 0) {
         viewResults(searchResults, r);
         refresh();
         footer();
      }
   }
}

void viewResults(SearchResult *results, SearchResult *startFrom) {
   int highlightedIndex = 0;
   int ch = 0;
   SearchResult *printStartResult;
   int i = 0;

   if (startFrom != NULL) {
      resultCurrentlySelected = startFrom;
   } else if (resultCurrentlySelected == NULL) {
      resultCurrentlySelected = results;
   }
   printStartResult = resultCurrentlySelected;

   // If a valid search results mode isn't set, set one.
   if (currentSearchResultsMode < 0) currentSearchResultsMode = 0;

   printResults(printStartResult, highlightedIndex);
   while (1==1) {
      while (!kbhit());
      ch = getch();
      if (ch == 27) {
         break;
      } else if (ch == 71) { // Home
         printStartResult = results;
         resultCurrentlySelected = results;
         highlightedIndex = 0;
      } else if (ch == 72) { // Up arrow
         if (highlightedIndex > 0 && resultCurrentlySelected->prev != NULL) {
            highlightedIndex--;
            resultCurrentlySelected = resultCurrentlySelected->prev;
         } else if (resultCurrentlySelected->prev != NULL) {
            printStartResult = printStartResult->prev;
            resultCurrentlySelected = resultCurrentlySelected->prev;
         } else {
            // At start of list, do nothing.
         }
      } else if (ch == 73) { // Page up
         for (i = 0; i < searchResultsModeLineCount[currentSearchResultsMode]; i++) {
            if (printStartResult->prev != NULL) {
               printStartResult = printStartResult->prev;
               resultCurrentlySelected = resultCurrentlySelected->prev;
            } else {
               break;
            }
         }
      } else if (ch == 79) { // End
         printStartResult = findLastSearchResult(results);
         resultCurrentlySelected = findLastSearchResult(results);
         highlightedIndex = 0;
      } else if (ch == 80) { // Dn arrow
         if (highlightedIndex < (searchResultsModeLineCount[currentSearchResultsMode]-1) && resultCurrentlySelected->next != NULL) {
            highlightedIndex++;
            resultCurrentlySelected = resultCurrentlySelected->next;
         } else if (resultCurrentlySelected->next != NULL) {
            printStartResult = printStartResult->next;
            resultCurrentlySelected = resultCurrentlySelected->next;
         } else {
            // At end of list, do nothing.
         }
      } else if (ch == 81) { // Page Down
         for (i = 0; i < searchResultsModeLineCount[currentSearchResultsMode]; i++) {
            if (resultCurrentlySelected->next != NULL) {
               resultCurrentlySelected = resultCurrentlySelected->next;
               printStartResult = printStartResult->next;
            } else {
               break;
            }
         }
      } else if (ch == 43) { // + key
         if (currentSearchResultsMode < (VIEW_RESULTS_MODE_COUNT-1)) currentSearchResultsMode++;
      } else if (ch == 45) { // - key
         if (currentSearchResultsMode > 0) currentSearchResultsMode--;
         // If the highlighted bar drops off the edge, bring it back into range.
         while (highlightedIndex > searchResultsModeLineCount[currentSearchResultsMode] - 1) {
            resultCurrentlySelected = resultCurrentlySelected->prev;
            highlightedIndex--;
         }
         refresh();
      } else if (ch == 13) { // Enter key
         currentLBAhi = resultCurrentlySelected->lbahi;
         currentLBAlo = resultCurrentlySelected->lbalo;
         // currentOffset is the byte offset of the substring occurrence, rounded down to the nearest 'line' of bytes.
         currentOffset = resultCurrentlySelected->offset / displaymodes[currentDisplayMode].bytesPerLine * displaymodes[currentDisplayMode].bytesPerLine;
         refresh();
      } else if (ch==64) { // F6 key
         searchAgain(&drives[sd], 0, searchTerms);
      }
      printResults(printStartResult, highlightedIndex);
   }
   currentSearchResultsMode = -1;
}

void header() {
   textline(1,1,CTR,NORM,"Data Viewer Tool - Copyright (c) 1999-2007 TASC Pty Ltd. All rights reserved.");
   clreol();
   printf("\n");
   clreol();
}

void footer() {
   int i;
   setcolor(15,1);

   for (i = wherey(); i < 24; i++) {
      clreol();
      printf("\n");
   }
   setcolor(0,7);
   cprintf(" F1:Help   F2:Select Drive   F3:Display Mode   F4:Go to Sector   F5:Search");
   clreol();
   cprintf("\r\n F6:Continue Search    F7:Show Search Results    F8:Save Sector to Disk");
   clreol();
}

void domsg(char far *msg) {
   int width;
   const char *closeMsg = "Press any key to close";
   word x1;
   word x2;

   width = MAX(strlen(msg), strlen(closeMsg));
   x1 = (80-width-4)/2;
   x2 = x1 + width + 2;

   notice(GENERIC_MESSAGE_HANDLE, x1, 9, x2, 14, NONE, EXPLODE, TRUE, "Message", 0, "");
   textline(x1+2, 11, NORM, BOXI, msg);
   textline(x1+2, 13, NORM, BOXI, closeMsg);
   while (!kbhit() || getch()==0);
   killmsg(GENERIC_MESSAGE_HANDLE);
}

void saveSector(Drive far *drive, unsigned long lbahi, unsigned long lbalo) {
   char filename[30];
   int entryReturnValue;
   FILE *f;

   memset(filename, 0, 30);
   notice(SECTOR_FILENAME_PROMPT_HANDLE,25,10,56,14,NONE,EXPLODE,TRUE,"Save Sector",0,"");
   textline(27,12,NORM,BOXI,"Enter filename to save to:");
   // Get sector number from user.
   get(27, 13, 28, "%s", filename, NULL);
   entryReturnValue = entry(0);
   killmsg(SECTOR_FILENAME_PROMPT_HANDLE);
   if (entryReturnValue >= 0) {
      do_read(drive, lbahi, lbalo, 1, bufferPtr);
      f = fopen(filename, "wb");
      if (f == NULL) {
         domsg("Couldn't create file. Ensure the disk isn't write-protected.");
      } else {
         fwrite(bufferPtr, 512, 1, f);
         fclose(f);
         domsg("Sector save done!");
      }
   }
}
