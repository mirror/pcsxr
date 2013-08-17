/***************************************************************************
    drawgl.m
    an odd set of functions that seem misplaced ATM.
    presumably this is the glue to the C GPU plugin stuff
    but a much better place might be "PluginWindowController.m" as
    gluing is what a controller is made for.
    
    PeopsOpenGPU
  
    Created by Gil Pedersen on Sun April 18 2004.
    Copyright (c) 2004 Gil Pedersen.
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#import "PluginWindowController.h"
#import "PluginGLView.h"
#include "ExtendedKeys.h"
#include "externals.h"
#include "draw.h"
#include "gpu.h"
#include "menu.h"
#include "drawgl.h"
#import "ARCBridge.h"

////////////////////////////////////////////////////////////////////////////////////
// misc globals
////////////////////////////////////////////////////////////////////////////////////
#if 0 // globals for OpenGL (vs. SoftGPU) are owned by others... weird
int            iResX;
int            iResY;
long           lLowerpart;
BOOL           bIsFirstFrame = TRUE;
BOOL           bCheckMask=FALSE;
unsigned short sSetMask=0;
/* unsigned long  lSetMask=0; */
uint32_t        sSetMassk=0;
int            iDesktopCol=16;
int            iShowFPS=0;
int            iWinSize; 
int            iUseScanLines=0;
int            iUseNoStretchBlt=0;
int            iFastFwd=0;
int            iDebugMode=0;
int            iFVDisplay=0;
PSXPoint_t     ptCursorPoint[8];
unsigned short usCursorActive=0;
char *			Xpixels;
char *         pCaptionText;
#endif


extern BOOL    bCheckMask;
extern BOOL    bIsFirstFrame;
extern int     iShowFPS;
extern unsigned short sSetMask;
extern int     iUseScanLines;
extern unsigned short usCursorActive;


int            iResX;
int            iResY;
long           lLowerpart;

uint32_t        sSetMassk=0;
int            iDesktopCol=16;
int            iWinSize; 
int            iUseNoStretchBlt=0;
int            iFastFwd=0;
int            iDebugMode=0;
int            iFVDisplay=0;
PSXPoint_t     ptCursorPoint[8];
char *			Xpixels;
char *         pCaptionText;

//static PluginWindowController *windowController;
// static is BAD NEWS if user uses other plug ins
PluginGLView *glView;

////////////////////////////////////////////////////////////////////////

void DoBufferSwap(void)                                // SWAP BUFFERS
{
#if 1
	[glView swapBuffer];
#else
	static long long lastTickCount = -1;
	static int skipCount = 0;
	long long microTickCount;
	long deltaTime;
	
	Microseconds((struct UnsignedWide *)&microTickCount);
	deltaTime = (long)(microTickCount - lastTickCount);
	if (deltaTime <= (PSXDisplay.PAL ? 1000000/50 : 100000000 / 5994) ||
		 skipCount >= 3) {
		skipCount = 0;
		[glView swapBuffer];
	} else {
		skipCount++;
	}
	NSLog(@"count: %i", deltaTime); 
	lastTickCount = microTickCount;
#endif
}


////////////////////////////////////////////////////////////////////////

void DoClearScreenBuffer(void)                         // CLEAR DX BUFFER
{
	// clear the screen, and DON'T flush it
	[glView clearBuffer:NO];
}


////////////////////////////////////////////////////////////////////////

void DoClearFrontBuffer(void)                          // CLEAR DX BUFFER
{
	// clear the screen, and flush it
	[glView clearBuffer:YES];
}

////////////////////////////////////////////////////////////////////////

unsigned long ulInitDisplay(void)	// OPEN GAME WINDOW
{
	bUsingTWin = FALSE;
	
	//InitMenu(); // This function does nothing
	
	bIsFirstFrame = FALSE;
	
	if(iShowFPS)
	{
		//iShowFPS=0;
		ulKeybits |= KEY_SHOWFPS;
		szDispBuf[0] = 0;
		BuildDispMenu(0);
	}
	
	__block PluginWindowController *windowController;
	
	// this causes a runtime error if it's done on a thread other than the main thread
	RunOnMainThreadSync(^{
		windowController = [PluginWindowController openGameView];
		glView = [windowController openGLView];
		
		[[windowController window] setTitle:@(pCaptionText)];
	});
	
	return (unsigned long)[windowController window];
}


////////////////////////////////////////////////////////////////////////

void CloseDisplay(void)
{
	if (gameController) {
		[gameController close];
		RELEASEOBJ(gameController);
		gameController = nil;
		gameWindow = nil;
	}
}

void BringContextForward(void)
{
    [[glView openGLContext] makeCurrentContext];
}

void SendContextBack(void)
{
    [NSOpenGLContext clearCurrentContext];
}

void SetVSync(GLint myValue)
{
    GLint DoItMyFriend = myValue;
   [[glView openGLContext] setValues: &DoItMyFriend forParameter: NSOpenGLCPSwapInterval];

}
////////////////////////////////////////////////////////////////////////

/* taken care of in menu.c
void CreatePic(unsigned char * pMem)
{
}
*/

///////////////////////////////////////////////////////////////////////////////////////

/* taken care of in menu.c
void DestroyPic(void)
{
}
*/

///////////////////////////////////////////////////////////////////////////////////////
/* taken care of in menu.c
void DisplayPic(void)
{
}
*/

///////////////////////////////////////////////////////////////////////////////////////

void ShowGpuPic(void)
{
	// this is the default implementation...
}

///////////////////////////////////////////////////////////////////////////////////////

void ShowTextGpuPic(void)
{
	// this is the default implementation...
}

void HandleKey(int keycode)
{
	switch (keycode) {
        case GPU_FRAME_LIMIT:
            if(bUseFrameLimit) {
                bUseFrameLimit = false;
                iFrameLimit = 1;
            }
            else {
                bUseFrameLimit = true;
                iFrameLimit = 2;
            }
            SetAutoFrameCap();
            break;
        case GPU_FAST_FORWARD:
            if(bUseFrameLimit) {
                bUseFrameLimit = false;
                iFrameLimit = 1;
                bUseFrameSkip = true;
                iFastFwd = 0;
            }
            else {
                bUseFrameLimit = true;
                iFrameLimit = 2;
                bUseFrameSkip = false;
                iFastFwd = 0;
            }
            bSkipNextFrame = FALSE;
            break;
		case GPU_FULLSCREEN_KEY:
			[gameController setFullscreen:![gameController fullscreen]];
			break;
	}
}
