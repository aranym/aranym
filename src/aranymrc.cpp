/*
 * $Header$
 *
 * MJ 2001
 */

#include "sysdeps.h"

#ifndef HAVE_TVISION
#error You need TurboVision library for aranymrc
#endif

#define Uses_TEventQueue
#define Uses_TEvent
#define Uses_TProgram
#define Uses_TApplication
#define Uses_TKeys
#define Uses_TRect
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusItem
#define Uses_TStatusDef
#define Uses_TDeskTop
#define Uses_TView
#define Uses_TWindow
#define Uses_TFrame
#define Uses_TScroller
#define Uses_TScrollBar
#define Uses_ScrollDialog
#define Uses_ScrollGroup
#define Uses_TDialog
#define Uses_TButton
#define Uses_TSItem
#define Uses_TCheckBoxes
#define Uses_TRadioButtons
#define Uses_TOutline
#define cpOutlineViewer "\x6\x7\x3\x8"
#define Uses_TOutlineViewer
#define Uses_TOutline
#define Uses_TEvent
#define Uses_TScroller
#define Uses_ScrollDialog
#define Uses_ScrollGroup
#define Uses_TFileDialog
#define Uses_TLabel
#define Uses_TInputLine

#include <tv.h>

#include <stdio.h>
#include <stdlib.h>
#include "parameters.h"

const int cmOpenRc	= 200;
const int cmSaveRc	= 202;

uint32 FastRAMSize;	// Size of FastRAM
char FRAMS[20];
char IDE0cyl[20];
char IDE0hea[20];
char IDE0spt[20];
char IDE1cyl[20];
char IDE1hea[20];
char IDE1spt[20];


class TSetWindow : public TWindow
{
	TGroup *sGlobal;
	TCheckBoxes32 *sDebugger;
	TInputLine *sFastRAMSize;
	TInputLine *sTOS;
	TGroup *sIDE0;
	TCheckBoxes32 *sIDE0i;
	TInputLine *sIDE0cyl;
	TInputLine *sIDE0hea;
	TInputLine *sIDE0spt;
	TInputLine *sIDE0pat;
	TGroup *sIDE1;
	TCheckBoxes32 *sIDE1i;
	TInputLine *sIDE1cyl;
	TInputLine *sIDE1hea;
	TInputLine *sIDE1spt;
	TInputLine *sIDE1pat;
public:
	TSetWindow(const TRect&r, const char *aTitle, short aNumber);
	virtual void saveSet();
};

class TMyApp : public TApplication {
	TSetWindow *SetWindow;
public:
	FILE *f;
	TMyApp(FILE *f);
	static TMenuBar *initMenuBar(TRect r);
	static TStatusLine *initStatusLine(TRect r);
	virtual void openInfo();
	virtual void openSet();
	virtual void saveSet();
	virtual void handleEvent(TEvent& event);
};

TSetWindow::TSetWindow(const TRect& r, const char *aTitle, short aNumber):
	TWindow(r,aTitle,aNumber),
	TWindowInit(&TSetWindow::initFrame)
{
	TRect bounds = r;

	TRect bounds2 =TRect(r.b.x-1,r.a.y+1,r.b.x,r.b.y-1);

	TScrollBar *vScrollBar = new TScrollBar(bounds2);
	assert(vScrollBar!=NULL);
	vScrollBar->options |= ofPostProcess;
	vScrollBar->growMode = gfGrowHiY;
	insert ( vScrollBar );

	bounds2 = TRect(r.a.x+1,r.b.y-1,r.b.x-1,r.b.y);
	TScrollBar *hScrollBar = new TScrollBar(bounds2);
	assert(hScrollBar!=NULL);
	hScrollBar->options |= ofPostProcess;
	hScrollBar->growMode = gfGrowHiX;
	insert ( hScrollBar );

	flags &= ~wfClose; /* No close button */
	flags &= ~wfGrow; /* No resize */
	flags &= ~wfMove; /* No D&D */
	flags &= ~wfZoom; /* No zoom button */
	state &= ~sfShadow; /* No shadow */

	bounds.grow(-1,-1);
	
	sGlobal = new TGroup(TRect(2, 2, r.b.x - 2, 8));

	sDebugger = new TCheckBoxes32( TRect(1, 2, 15, 3),
            new TSItem( "~D~ebugger", NULL )
            );
	if (start_debug) sDebugger->press(0);
        sGlobal->insert( sDebugger );

	sFastRAMSize = new TInputLine(TRect(43, 2, 54, 3), 10);
	sprintf(FRAMS, "%d", FastRAMSize / 1024 / 1024);
	sFastRAMSize->setData(FRAMS);
	sGlobal->insert(sFastRAMSize);
	sGlobal->insert(new TLabel(TRect(20, 2, 42, 3), "FastRAM size (in MB):", sFastRAMSize));
	
	sTOS = new TInputLine(TRect(8, 4, 75, 5), 511);
	sTOS->setData(rom_path);
	sGlobal->insert( sTOS);
	sGlobal->insert( new TLabel(TRect(2, 4, 8, 5), "TOS:", sTOS ));

	insert( sGlobal);
	insert( new TLabel(TRect( 2, 2, 10, 3), "Global", sGlobal ));

	sIDE0 = new TGroup(TRect(2, 9, r.b.x - 2, 14));

	sIDE0i = new TCheckBoxes32( TRect(1, 2, 15, 4),
            new TSItem( "~P~resent",
	    new TSItem( "~B~yteswap", NULL))
            );
	if (bx_options.diskc.present) sIDE0i->press(0);
	if (bx_options.diskc.byteswap) sIDE0i->press(1);
	sIDE0->insert( sIDE0i );

	sIDE0cyl = new TInputLine(TRect(23, 1, 34, 2), 10);
	sprintf(IDE0cyl, "%d", bx_options.diskc.cylinders);
	sIDE0cyl->setData(IDE0cyl);
	sIDE0->insert( sIDE0cyl);
	sIDE0->insert( new TLabel(TRect(12, 1, 23, 2), "Cylinders:", sIDE0cyl ));

	sIDE0hea = new TInputLine(TRect(42, 1, 53, 2), 10);
	sprintf(IDE0hea, "%d", bx_options.diskc.heads);
	sIDE0hea->setData(IDE0hea);
	sIDE0->insert( sIDE0hea);
	sIDE0->insert( new TLabel(TRect(35, 1, 42, 2), "Heads:", sIDE0hea ));

	sIDE0spt = new TInputLine(TRect(64, 1, 75, 2), 10);
	sprintf(IDE0spt, "%d", bx_options.diskc.spt);
	sIDE0spt->setData(IDE0spt);
	sIDE0->insert( sIDE0spt);
	sIDE0->insert( new TLabel(TRect(54, 1, 63, 2), "Sectors:", sIDE0spt ));

	sIDE0pat = new TInputLine(TRect(23, 3, 75, 4), 511);
	sIDE0pat->setData(bx_options.diskc.path);
	sIDE0->insert( sIDE0pat);
	sIDE0->insert( new TLabel(TRect(17, 3, 23, 4), "Path:", sIDE0pat ));

	insert( sIDE0);
	insert( new TLabel(TRect( 2, 9, 10, 10), "IDE0", sIDE0 ));

	sIDE1 = new TGroup(TRect(2, 15, r.b.x - 2, 20));

	sIDE1i = new TCheckBoxes32( TRect(1, 2, 15, 4),
            new TSItem( "~P~resent",
	    new TSItem( "~B~yteswap", NULL))
            );
	if (bx_options.diskd.present) sIDE1i->press(0);
	if (bx_options.diskd.byteswap) sIDE1i->press(1);
	sIDE1->insert( sIDE1i );

	sIDE1cyl = new TInputLine(TRect(23, 1, 34, 2), 10);
	sprintf(IDE1cyl, "%d", bx_options.diskd.cylinders);
	sIDE1cyl->setData(IDE1cyl);
	sIDE1->insert( sIDE1cyl);
	sIDE1->insert( new TLabel(TRect(12, 1, 23, 2), "Cylinders:", sIDE1cyl ));

	sIDE1hea = new TInputLine(TRect(42, 1, 53, 2), 10);
	sprintf(IDE1hea, "%d", bx_options.diskd.heads);
	sIDE1hea->setData(IDE1hea);
	sIDE1->insert( sIDE1hea);
	sIDE1->insert( new TLabel(TRect(35, 1, 42, 2), "Heads:", sIDE1hea ));

	sIDE1spt = new TInputLine(TRect(64, 1, 75, 2), 10);
	sprintf(IDE1spt, "%d", bx_options.diskd.spt);
	sIDE1spt->setData(IDE1spt);
	sIDE1->insert( sIDE1spt);
	sIDE1->insert( new TLabel(TRect(54, 1, 63, 2), "Sectors:", sIDE1spt ));

	sIDE1pat = new TInputLine(TRect(23, 3, 75, 4), 511);
	sIDE1pat->setData(bx_options.diskd.path);
	sIDE1->insert( sIDE1pat);
	sIDE1->insert( new TLabel(TRect(17, 3, 23, 4), "Path:", sIDE1pat ));

	insert( sIDE1);
	insert( new TLabel(TRect( 2, 15, 10, 16), "IDE1", sIDE1 ));

}

TMyApp::TMyApp(FILE *f) :
	TProgInit(&TMyApp::initStatusLine, &TMyApp::initMenuBar,
		  &TMyApp::initDeskTop),  f(f) {
	SetWindow = NULL;
	openInfo();
	fclose(f);
	openSet();
}

TMenuBar *TMyApp::initMenuBar( TRect r )
{
    r.b.y = r.a.y + 1;    // set bottom line 1 line below top line
    return new TMenuBar( r,
        *new TSubMenu( "~F~ile", kbAltF )+
            *new TMenuItem( "~O~pen profile", cmOpenRc, kbF3, hcNoContext, "F3" )+
            newLine()+
            *new TMenuItem( "~S~ave config", cmSaveRc, kbF5, hcNoContext, "F5" )+
            newLine()+

            *new TMenuItem( "E~x~it", cmQuit, cmQuit, hcNoContext, "Alt-X" )


            // new dialog menu added here
        );
}

TStatusLine *TMyApp::initStatusLine(TRect r)
{
    r.a.y = r.b.y - 1;     // move top to 1 line above bottom
    return new TStatusLine( r,
        *new TStatusDef( 0, 0xFFFF ) +
        // set range of help contexts
            *new TStatusItem( 0, kbF10, cmMenu ) +
            *new TStatusItem( "~Alt-X~ Exit", kbAltX, cmQuit )
        );
}

void TMyApp::handleEvent(TEvent& event)
{
	TApplication::handleEvent(event);
	if (event.what == evCommand) 
		switch (event.message.command)  {
			case cmSaveRc:
				clearEvent(event);
				saveSet();
				break;

			case cmOpenRc:
				clearEvent(event);
				if ((f = tmpfile()) == NULL) {
					fprintf(stderr, "Couldn't open temporary file!\n");
					exit(-1);
				}
				decode_switches(f, 0, NULL);
				openInfo();
				openSet();
				fclose(f);
				break;

		}

	
}

void TMyApp::openInfo()
{
	char *buf[10];
	int i;
	fseek(f, 0L, SEEK_SET);
	for (i = 0; i < 10; i++) {
		buf[i] = (char *)malloc(53 * sizeof(char));
		do {
			if (fgets(buf[i], 52, f) == NULL) break;
		} while (buf[i][0] != '[' && buf[i][0] != 'U');
		if (buf[i][strlen(buf[i]) - 1] == '\n') buf[i][strlen(buf[i]) - 1] = '\0';

		if (feof(f)) break;
	}

	TDialog *pd = new TDialog( TRect( 10, 4, 70, 18), "Result Dialog" );
	if( pd ) {
		TView *b = new TView( TRect( 3, 3, 57, 6));
		pd->insert( b );

		for (int j = 0; j < i; j++) 
			pd->insert( new TLabel( TRect( 2, 2 + j, 55, 3 + j), buf[j], b ));

		deskTop->execView( pd );
	}
	destroy( pd );
}

void TSetWindow::saveSet() {
	if (sDebugger->mark(0)) start_debug = 1; else start_debug = 0;
	sTOS->getData(rom_path);

	sFastRAMSize->getData(FRAMS);
	FastRAMSize = atoi(FRAMS) * 1024 * 1024;

	if (sIDE0i->mark(0)) bx_options.diskc.present = 1;
		else bx_options.diskc.present = 0;
	if (sIDE0i->mark(1)) bx_options.diskc.byteswap = 1;
		else bx_options.diskc.byteswap = 0;
	sIDE0cyl->getData(IDE0cyl);
	bx_options.diskc.cylinders = atoi(IDE0cyl);
	sIDE0hea->getData(IDE0hea);
	bx_options.diskc.heads = atoi(IDE0hea);
	sIDE0spt->getData(IDE0spt);
	bx_options.diskc.spt = atoi(IDE0spt);
	sIDE0pat->getData(bx_options.diskc.path);

	if (sIDE1i->mark(0)) bx_options.diskd.present = 1;
		else bx_options.diskd.present = 0;
	if (sIDE1i->mark(1)) bx_options.diskd.byteswap = 1;
		else bx_options.diskd.byteswap = 0;
	sIDE1cyl->getData(IDE1cyl);
	bx_options.diskd.cylinders = atoi(IDE1cyl);
	sIDE1hea->getData(IDE1hea);
	bx_options.diskd.heads = atoi(IDE1hea);
	sIDE1spt->getData(IDE1spt);
	bx_options.diskd.spt = atoi(IDE1spt);
	sIDE1pat->getData(bx_options.diskd.path);
}

void TMyApp::saveSet() {
	if (SetWindow != NULL) {
		SetWindow->saveSet();
		saveSettings("aranymtest.rc");
	}
}

void TMyApp::openSet() {
	TRect bounds = getExtent();
	bounds.b.y-=2;
	if (SetWindow != NULL) free(SetWindow);
	SetWindow = NULL;
	SetWindow = new TSetWindow(bounds,"Settings",0);

	deskTop->insert(SetWindow);
}

int main(int argc, char **argv)
{
	FILE *f;

	if ((f = tmpfile()) == NULL) {
		fprintf(stderr, "Couldn't open temporary file!\n");
		exit(-1);
	}

	decode_switches(f, argc, argv);

	TMyApp myApp(f);
	
	myApp.run();
	return 0;
}


/*
 * $Log$
 * Revision 1.5  2001/12/27 22:24:14  joy
 * FastRAMSizeMB should not be global
 *
 * Revision 1.4  2001/10/16 19:38:44  milan
 * Integration of BasiliskII' cxmon, FastRAM in aranymrc etc.
 *
 * Revision 1.3  2001/10/09 19:25:19  milan
 * MemAlloc's rewriting
 *
 * Revision 1.2  2001/09/25 00:04:17  milan
 * cleaning of memory managment
 *
 * Revision 1.1  2001/08/29 18:36:25  milan
 * Integration of TV conf. GUI, small patches of MMU and debugger
 *
 *
 */
