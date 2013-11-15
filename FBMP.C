/* FBMP.C -- Freeware BMP viewer for PM */

/* Version 0.97 */

/* This program is Copyright (c) 1992 by Raja Thiagarajan. However, it may
   be used for any NON-commercial purpose.
   If you have any comments or questions, you can contact me at
   sthiagar@bronze.ucs.indiana.edu (Internet) or 72175,12 (CompuServe).
   Note that I read my Internet mail almost daily, but only check my
   CompuServe mail about once a week. I would appreciate hearing
   about bugfixes or improvements!
   Thanks to Peter Nielsen (pnielsen@aton.abo.fi) for some clever ideas
   and good improvements!
*/

#include <stdio.h> /* include fclose(), fopen(), fread(), fseek(), ftell(),
                      sprintf()*/
#include <stdlib.h> /* include EXIT_FAILURE, exit(), free(), malloc() */
#include <string.h> /* include strcat(), strrchr() */

#define INCL_DOS /* include DosBeep() */
#define INCL_GPI
#define INCL_WIN
#include <os2.h>

/* Prototypes */
VOID ReadFile (CHAR * fName);
VOID PlaceTheWindow (HWND hwnd);
MRESULT EXPENTRY ClientWinProc (HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2);

static VOID  * bPtr;
static HAB     hab;  /* the handle to the program's anchor block */
static ULONG   bWid, bHi; /* Bitmap width and height */
static USHORT  bBits; /* Bits per plane */

static LONG    hasPalMan;

static CHAR    title [384]; /* Window title */

INT main (INT argc, CHAR * argv[])
{
   HMQ   hmq;        /* handle for message queue */
   QMSG  qmsg;       /* message queue element */
   HWND  hwnd,       /* handle to frame window */
         hwndClient; /* handle for client window */

   ULONG createFlags =  FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER
                              | FCF_MINMAX | FCF_SHELLPOSITION
                              | FCF_TASKLIST;

   static   CHAR  clientClass [] = "Client Window";

   if (argc != 2) {
      DosBeep (440, 100);
      exit (EXIT_FAILURE);
   } else {
      ReadFile (argv[1]);
   }

   {  /* Display the filename (no path!) and bitmap dimensions */
      CHAR * cp = strrchr (argv[1], '\\');
      if (!cp) {
         cp = strrchr (argv[1], ':');
      }
      if (!cp) {
         cp = argv[1];
      } else {
         cp++; /* skip past the backslash (or colon) */
      }
      sprintf (title, "fBMP -- %s (%ux%u)", cp, bWid, bHi);
   }

   hab = WinInitialize (0);   /* initialize PM usage */
   hmq = WinCreateMsgQueue (hab, 0);   /* create message queue */

   WinRegisterClass (hab, clientClass, (PFNWP) ClientWinProc, CS_SIZEREDRAW, 0);

      /* create standard window and client */
   hwnd = WinCreateStdWindow (HWND_DESKTOP, 0UL, &createFlags,
                     clientClass, title, 0L, 0UL, 0, &hwndClient);

   PlaceTheWindow (hwnd);

   while (WinGetMsg (hab, &qmsg, NULLHANDLE, 0, 0)) { /* msg dispatch loop */
      WinDispatchMsg (hab, &qmsg);
   }

   WinDestroyWindow (hwnd);   /* destroy frame window */
   WinDestroyMsgQueue (hmq);  /* destroy message queue */
   WinTerminate (hab);        /* terminate PM usage */

   return 0;
}

VOID ReadFile (CHAR * fname)
{
   FILE * bmpFile;
   LONG   fSize;

   bmpFile = fopen (fname, "rb");

   /* If file not found, append ".BMP" and try again */
   if ((bmpFile == NULL) && (!strrchr (fname, '.'))) {
      CHAR fbName [300];
      strcpy (fbName, fname);
      strcat (fbName, ".BMP");
      bmpFile = fopen (fbName, "rb");
   }

   if (bmpFile == NULL) {
      DosBeep (440, 100);
      exit (EXIT_FAILURE);
   }

   /* Check to see if it has the right format */
   {
      CHAR ut [2];
      fread (ut, sizeof (CHAR), 2, bmpFile);
      if ((ut[0] != 'B') || (ut[1] != 'M')) {
         DosBeep (440, 100); DosBeep (220, 100);
         exit (EXIT_FAILURE);
      }
   }

   /* Determine the size of the file */
   fseek (bmpFile, 0L, SEEK_END);
   fSize = ftell (bmpFile);

   /* Allocate that much free memory. [Boy, do I love virtual, flat-model
      memory!] */
   bPtr = malloc (fSize);
   if (bPtr == NULL) {
      fclose (bmpFile);
      DosBeep (440, 100); DosBeep (220, 100); DosBeep (110, 100);
      exit (EXIT_FAILURE);
   }

   /* Move back to the start of the file ...*/
   fseek (bmpFile, 0L, SEEK_SET);
   /* ... and load the whole thing in one gulp! */
   fread (bPtr, sizeof (BYTE), fSize, bmpFile);

   /* Now close the file [since we can read from RAM instead!] */
   fclose (bmpFile);

   /* As long as we're here, set the width, height, and bits per plane */
   {
      PBITMAPFILEHEADER2 pbfh2 = bPtr;
      bWid = pbfh2->bmp2.cx;
      bHi = pbfh2->bmp2.cy;
      bBits = pbfh2->bmp2.cBitCount;
   }

}

VOID PlaceTheWindow (HWND hwnd)
{
   HDC   hdcScr;
   RECTL rcl;
   LONG  scrWid, scrHi;
   hdcScr = GpiQueryDevice (WinGetPS (HWND_DESKTOP));
   DevQueryCaps (hdcScr, CAPS_WIDTH, 1L, &scrWid);   DevQueryCaps (hdcScr, CAPS_WIDTH, 1L, &scrWid);
   DevQueryCaps (hdcScr, CAPS_HEIGHT, 1L, &scrHi);
   DevQueryCaps (hdcScr, CAPS_ADDITIONAL_GRAPHICS, 1L, &hasPalMan);
   hasPalMan &= CAPS_PALETTE_MANAGER;
   rcl.xLeft = rcl.yBottom = 0;
   rcl.xRight = bWid;
   rcl.yTop = bHi;
   WinCalcFrameRect (hwnd, &rcl, FALSE);

   WinSetWindowPos (hwnd,
                     NULLHANDLE,
                     (scrWid - rcl.xRight + rcl.xLeft) / 2,
                     (scrHi - rcl.yTop + rcl.yBottom) / 2,
                     (rcl.xRight - rcl.xLeft),
                     (rcl.yTop - rcl.yBottom),
                     SWP_SHOW | SWP_MOVE | SWP_SIZE);
}

MRESULT EXPENTRY ClientWinProc (HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
   static HPS     hps;
   static HDC     hdc;
   static HPAL    hpal;
   static HBITMAP hbm;
   static BOOL    firstPaint = TRUE;

   switch (msg) {
      case WM_CREATE:
         {
            SIZEL sizl = {0L, 0L};
            hdc = WinOpenWindowDC (hwnd);
            hps = GpiCreatePS (hab, hdc, &sizl, PU_PELS | GPIF_DEFAULT
                      | GPIT_MICRO | GPIA_ASSOC);
 
            /* Create and use palette */
            {
               BYTE * foo = bPtr;
               if ((bBits >= 1) && (bBits <= 8)) {
                  hpal = GpiCreatePalette (hab, 0L, LCOLF_CONSECRGB,
                                           1 << bBits,
                                           (PULONG) (foo + 54));
                  GpiSelectPalette (hps, hpal);
               }
            }

            /* Load bitmap */
            {
               PBITMAPFILEHEADER2 pbfh2 = bPtr;
               BYTE * foo = bPtr;
               hbm = GpiCreateBitmap (hps,
                                       &(pbfh2->bmp2),
                                       CBM_INIT,
                                       (foo + pbfh2->offBits),
                                       (PBITMAPINFO2) &(pbfh2->bmp2));
            }

            /* We don't need the file image any more, so ... */
            free (bPtr);
         }
         return (MRESULT) FALSE;
      case WM_DESTROY:
         GpiSetBitmap (hps, NULLHANDLE);
         GpiDeleteBitmap (hbm);
         GpiSelectPalette (hps, NULLHANDLE);
         GpiDeletePalette (hpal);
         GpiDestroyPS (hps);
         return (MRESULT) FALSE;
      case WM_REALIZEPALETTE:
         {
            ULONG palSize = 1 << bBits;
            if (WinRealizePalette (hwnd, hps, &palSize)) {
               WinInvalidateRect (hwnd, NULL, FALSE);
            }
         }
         return (MRESULT) FALSE;
      case WM_PAINT:
         {
            POINTL ptl = {0, 0};
/*RECTL rcl;*/
            WinBeginPaint (hwnd, hps, NULL);
            if (firstPaint) {
               ULONG palSize = 256;
               WinRealizePalette (hwnd, hps, &palSize);
               firstPaint = FALSE;
            }
/*WinQueryWindowRect (hwnd, &rcl);*/
            WinDrawBitmap (hps, hbm, NULL, &ptl, 0, 0, DBM_NORMAL);
/*WinDrawBitmap (hps, hbm, NULL, (PPOINTL) &rcl, 0, 0, DBM_STRETCH);*/
            WinEndPaint (hps);
         }
         return (MRESULT) FALSE;
      case WM_ERASEBACKGROUND:
         return (MRESULT) TRUE;
      default:
         return (WinDefWindowProc (hwnd, msg, mp1, mp2));
   }  /*end switch*/
}
