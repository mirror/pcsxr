#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define GPU_INTERNALS_DEF
#include "gpu_i.h"
#include "primitive_drawing.h"
#include "cfg.h"
#include "PSEmu_PlugIn_Defs.h"

#include <X11/extensions/xf86vmode.h>

#define CALLBACK

// PPDK developer must change libraryName field and can change revision and build

const unsigned char version = PLUGIN_VERSION;	// do not touch - library for PSEmu 1.x

// it is up to developer but values must be in range 0-255

const unsigned char revision =	1;
const unsigned char build =		0;

// to obtain library name for your plugin, mail: plugin@psemu.com
// this must be unique, and only we can provide this
static char *libraryName =		"PCSX-df OpenGL Plugin";

float scalarDispWidth,scalarDispHeight;
float scalarDrawWidth,scalarDrawHeight;


// driver dependant variables
GpuConfS gpuConfig;

static int initGPU = 0;
static int ScreenOpened = 0;

#define RED(x) (x & 0xff)
#define BLUE(x) ((x>>16) & 0xff)
#define GREEN(x) ((x>>8) & 0xff)

#define COLOR(x) (x & 0xffffff)

// macros for easy access to packet information
#define GPUCOMMAND(x) ((x>>24) & 0xff)

// memory image of the PSX vram 
unsigned char psxVub[1024*520*2];
signed char *psxVsb;
unsigned short *psxVuw;
signed short *psxVsw;
uint32_t *psxVul;
int32_t *psxVsl;

int flip;

// internal GPU

static int32_t GPUdataRet;
int32_t GPUstatusRet;
int32_t GPUInfoVals[16];

static uint32_t gpuData[100];
static unsigned char gpuCommand = 0;
static int32_t gpuDataC = 0;
static int32_t gpuDataP = 0;

int drawingLines;

VRAMLoad_t vramWrite;
struct PSXDisplay_t psxDisp, oldpsxDisp;
struct PSXDraw_t psxDraw;

short dispWidths[8] = {256,320,512,640,368,384,512,640};

int dispLace = 0;
int dispLaceNew;
int imageTransfer;
int drawLace;

#define FRAMES 16
GLuint drawrec;
int drawreccount;

short imTYc,imTXc,imTY,imTX;
int imSize;
short imageX0,imageX1;
short imageY0,imageY1;

unsigned short textBuf[512*512];
int newTextX0,newTextX1,newTextX2,newTextX3;
int newTextY0,newTextY1,newTextY2,newTextY3;

GLuint xferTexture16 = 0;
GLuint xferTexture24 = 0;

uint32_t gpuDataX;


typedef struct
{
	Display 				*dpy;
	int						screen;
	Window					win;
	GLXContext				ctx;
	XSetWindowAttributes	attr;
	BOOL					fs;
	XF86VidModeModeInfo		deskMode;
	int						x,y;
	unsigned int			width, height;
	unsigned int			bpp;
}GLWindow;

static GLWindow GLWin;	/* Set our OpenGL Window to static, we only want one */



char * CALLBACK PSEgetLibName(void)
{
	return libraryName;
}

unsigned long CALLBACK PSEgetLibType(void)
{
	return  PSE_LT_GPU;
}

unsigned long CALLBACK PSEgetLibVersion(void)
{
	return version<<16|revision<<8|build;
}




long CALLBACK GPUinit()
{
	//if(capcom fighting game) dispWidths[4]=384;
	//else                dispWidths[4]=368;

	/* Set default configuration values */
	gpuConfig.bFullscreen=FALSE;
	gpuConfig.bBilinear=FALSE;
	gpuConfig.nMaxTextures=64;
	gpuConfig.bWireFrame=FALSE;
	gpuConfig.bAntialias=FALSE;
	gpuConfig.bClearScreen=FALSE;
	gpuConfig.FrameLimit=1;

	/* Read in values from the config file */
	readconfig();

	// mapping the VRAM
	psxVsb=(signed char *)psxVub;
	psxVsw=(signed short *)psxVub;
	psxVsl=(int32_t *)psxVub;
	psxVuw=(unsigned short *)psxVub;
	psxVul=(uint32_t *)psxVub;

	GPUstatusRet = 0x74000000;
	memset(GPUInfoVals,0x00,16*sizeof(uint32_t));

	return PSE_ERR_SUCCESS;
}


long CALLBACK GPUshutdown()
{
	if(initGPU==1)
	{
		initGPU=0;
	}
	return 0;
}


void DoGLInit(){
	int i;

	glViewport(0,0,gpuConfig.windowX,gpuConfig.windowY);
	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glScalef(1.0f,-1.0f,1.0f);
	glOrtho(0.0,1024,0.0,512,1.0,-1.0);
	*/
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(1.0f/256.0f,1.0f/256.0f,1.0f);
	//glTranslatef(0.5f, 0.5f, 0);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
	if(gpuConfig.bAntialias){
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);
	}
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	glEnable(GL_TEXTURE_2D);
	for(i=0;i<gpuConfig.nMaxTextures;i++){
		texture[i].textAddrX=0;
		texture[i].textAddrY=0;
		texture[i].textTP=0;
		texture[i].clutP=nullclutP;
		texture[i].Update=FALSE;
		glGenTextures(1,&texture[i].id);
		glBindTexture(GL_TEXTURE_2D,texture[i].id);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
		if(gpuConfig.bBilinear){
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		}else{
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		}
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glTexImage2D(GL_TEXTURE_2D,0,4,256,256,0,GL_RGBA,GL_UNSIGNED_BYTE,image);
	}
	
	glGenTextures(1,&xferTexture24);
	glBindTexture(GL_TEXTURE_2D,xferTexture24);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); 
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexImage2D(GL_TEXTURE_2D,0,3,1024,512,0,GL_RGB,GL_UNSIGNED_BYTE,0);

	glGenTextures(1,&xferTexture16);
	glBindTexture(GL_TEXTURE_2D,xferTexture16);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); 
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexImage2D(GL_TEXTURE_2D,0,4,1024,512,0,GL_RGBA,GL_UNSIGNED_BYTE,0);

	glGenTextures(1,&nullid);
	glBindTexture(GL_TEXTURE_2D,nullid);
	glTexImage2D(GL_TEXTURE_2D,0,4,0,0,0,GL_RGBA,GL_UNSIGNED_BYTE,0);

	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if(gpuConfig.bWireFrame)
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

	drawrec = glGenLists(FRAMES);
	glNewList(drawrec, GL_COMPILE);
	drawreccount=0;


	//black is reversed: transparent when STP not set, opaque when set
	torgba[0]=0x00000000;
	for(i=1;i<65536;i++){
		torgba[i] =(i&0x001f)<<3;
		torgba[i]|=(i&0x03e0)<<6;
		torgba[i]|=(i&0x7c00)<<9;
		torgba[i]|=0xfe000000;
		if (!(i&0x8000))
			torgba[i]|=0xff000000;
		//torgba[i]|=0xff000000;
	}
	torgba[0x8000] = 0xff000000;

	for(i=0; i<128; i++)
		texshade[i]=i<<1;
	for(i=128; i<256; i++)
		texshade[i]=255;

	gllog(0,(char*)glGetString(GL_VENDOR));
	gllog(0,(char*)glGetString(GL_RENDERER));
	gllog(0,(char*)glGetString(GL_VERSION));
	gllog(0,(char*)glGetString(GL_EXTENSIONS));
}

long CALLBACK GPUopen(unsigned long * disp,char * CapText,char * CfgFile)
{
	int attrListDbl[] = {GLX_RGBA, GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		None};

	XVisualInfo *vi;
	Colormap cmap;
	int dpyWidth, dpyHeight;
	int i;
	int vidModeMajorVersion, vidModeMinorVersion;
	XF86VidModeModeInfo **modes;
	int modeNum;
	int bestMode;
	Atom wmDelete;
	Window winDummy;
	unsigned int borderDummy;

	GLWin.fs = gpuConfig.bFullscreen;
	bestMode = 0;

	GLWin.dpy = XOpenDisplay(0);
	GLWin.screen = DefaultScreen(GLWin.dpy);
	XF86VidModeQueryVersion(GLWin.dpy, &vidModeMajorVersion,
							&vidModeMinorVersion);

	XF86VidModeGetAllModeLines(GLWin.dpy, GLWin.screen, &modeNum, &modes);

	GLWin.deskMode = *modes[0];

	for (i = 0; i < modeNum; i++)
	{
		if ((modes[i]->hdisplay == gpuConfig.windowX) && (modes[i]->vdisplay == gpuConfig.windowY))
		{
			bestMode = i;
		}
	}

	vi = glXChooseVisual(GLWin.dpy, GLWin.screen, attrListDbl);
	if (vi == NULL)
	{
		printf("Visual not found\n");
		exit(0);
	}

	GLWin.ctx = glXCreateContext(GLWin.dpy, vi, 0, GL_TRUE);

	cmap = XCreateColormap(GLWin.dpy, RootWindow(GLWin.dpy, vi->screen),vi->visual, AllocNone);
	GLWin.attr.colormap = cmap;
	GLWin.attr.border_pixel = 0;

	GLWin.attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
		ButtonPressMask | ButtonReleaseMask | StructureNotifyMask;


	if(GLWin.fs)
	{
		XF86VidModeSwitchToMode(GLWin.dpy, GLWin.screen, modes[bestMode]);
		XF86VidModeSetViewPort(GLWin.dpy, GLWin.screen, 0, 0);
		dpyWidth = modes[bestMode]->hdisplay;
		dpyHeight = modes[bestMode]->vdisplay;
		XFree(modes);
		GLWin.attr.override_redirect = True;
		GLWin.win = XCreateWindow(GLWin.dpy, RootWindow(GLWin.dpy, vi->screen),
								0, 0, dpyWidth, dpyHeight, 0, vi->depth, InputOutput, vi->visual,
								CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
								&GLWin.attr);
		XWarpPointer(GLWin.dpy, None, GLWin.win, 0, 0, 0, 0, 0, 0);
		XMapRaised(GLWin.dpy, GLWin.win);
		XGrabKeyboard(GLWin.dpy, GLWin.win, True, GrabModeAsync,GrabModeAsync, CurrentTime);
		XGrabPointer(GLWin.dpy, GLWin.win, True, ButtonPressMask,
		GrabModeAsync, GrabModeAsync, GLWin.win, None, CurrentTime);
	}
	else
	{
		GLWin.win = XCreateWindow(GLWin.dpy, RootWindow(GLWin.dpy, vi->screen),
								  0, 0, gpuConfig.windowX, gpuConfig.windowY, 0, vi->depth, InputOutput, vi->visual,
								  CWBorderPixel | CWColormap | CWEventMask, &GLWin.attr);
		wmDelete = XInternAtom(GLWin.dpy, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(GLWin.dpy, GLWin.win, &wmDelete, 1);
		XSetStandardProperties(GLWin.dpy, GLWin.win, CapText,
		CapText, None, NULL, 0, NULL);
		XMapRaised(GLWin.dpy, GLWin.win);
	}

	glXMakeCurrent(GLWin.dpy, GLWin.win, GLWin.ctx);
	XGetGeometry(GLWin.dpy, GLWin.win, &winDummy, &GLWin.x, &GLWin.y,
				&GLWin.width, &GLWin.height, &borderDummy, &GLWin.bpp);

	printf("Direct Rendering: %s\n",glXIsDirect(GLWin.dpy, GLWin.ctx) ? "true" : "false");
	printf("Running in %s mode\n",GLWin.fs ? "fullscreen" : "window");

	if(disp)
		*disp=(long)GLWin.dpy;                                     // wanna x pointer? ok

	ScreenOpened=1;
	
	DoGLInit();

	return 0;
}

long CALLBACK GPUclose()
{
	if(GLWin.ctx)
	{
		if(!glXMakeCurrent(GLWin.dpy, None, NULL))
		{
			printf("Error releasing drawing context : killGLWindow\n");
		}
		glXDestroyContext(GLWin.dpy, GLWin.ctx);
		GLWin.ctx = NULL;
	}

	if(GLWin.fs)
	{
		XF86VidModeSwitchToMode(GLWin.dpy, GLWin.screen, &GLWin.deskMode);
		XF86VidModeSetViewPort(GLWin.dpy, GLWin.screen, 0, 0);
	}
	XCloseDisplay(GLWin.dpy);

	return 0;
}

bool db=FALSE;


void waitforrealtime();

/*
Here's how playstation double buffering works:
1) Vsync
2) Set buffer selection (via status 0x05) (psxDisp.startX & Y)
3) (Possibly) swap draw area, and draw.
1) Vsync
...

The problem arises in step 3 when drawing is done in the back buffer while step 2 and the subsequent vsync shows the front.
Current solution is to record the draw commands (3), wait till (2), then playback the draw into the now-known buffer.
During vsync we output the most recent frame (3) to catch any offscreen drawing.  This means we draw everything twice...but at least most of it will be clipped.

*/

void updateScreenMode()
{
	int i=0;

	glEndList();

	if(psxDisp.modeX>0 && psxDisp.modeY>0)
	{
		int height = (psxDisp.rangeY2-psxDisp.rangeY1) * ((int)psxDisp.interlaced+1);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glScalef(1.0f,-1.0f,1.0f);
		glOrtho(psxDisp.startX, psxDisp.startX + psxDisp.modeX,
			psxDisp.startY, psxDisp.startY + height, 1.0,-1.0);
		//glOrtho(0,(double)1024,0,(double)768,1.0,-1.0);
		//glOrtho(0,psxDisp.modeX,0,psxDisp.modeY,1.0,-1.0);

		//glMatrixMode(GL_MODELVIEW);
		//glLoadIdentity();
		//glPushMatrix();
		//glTranslatef(-oldpsxDisp.startX, -oldpsxDisp.startY, 0);
		for(i=0;i<=drawreccount;i++)
		{
			glCallList(drawrec+i);
			//glXSwapBuffers(GLWin.dpy, GLWin.win);
		}
		drawreccount=0;
		//glPopMatrix();
	}

	glNewList(drawrec,GL_COMPILE);
}

void updateDisplay(void)
{
	glEndList();

	glCallList(drawrec+drawreccount);
	glFlush();
	if (gpuConfig.FrameLimit)
		waitforrealtime();

	glXSwapBuffers(GLWin.dpy, GLWin.win);

	drawreccount++;
	drawreccount%=FRAMES;

	glNewList(drawrec+drawreccount,GL_COMPILE);
}

// update lace is called every VSync
void CALLBACK GPUupdateLace(void)
{
	//if(dispLace)
	//{
		drawingLines^=1;
		//if(psxDisp.modeX>0 && psxDisp.modeY>0)
		//{
			//gllog(0,"UPDATING LACE");
			updateDisplay();
		//}
	//}
}


// process read request from GPU status register
unsigned long CALLBACK GPUreadStatus(void)
{
	// return the status of the GPU
	/*
	- gp1 - GPU Status
          Mask
        04000000 - 1:Idle 0:Busy
        10000000 - 1:Ready 0:Not ready , to receive commands.
        80000000 - GPU is drawing 1:Odd 0:Even lines, in interlaced mode.
	*/
	//just'0x74000000' for now ! (idle, ready, even lines)
	if (drawingLines==1)
		return GPUstatusRet;
	else
		return GPUstatusRet|0x80000000;
	//return 0x74000000;
}


// processes data send to GPU status register
// these are always single packet commands.
void CALLBACK GPUwriteStatus(uint32_t gdata)
{
	switch((gdata>>24)&0xff)
	{
	case 0x00:
		memset(GPUInfoVals,0x00,16*sizeof(uint32_t));
		GPUstatusRet=0x14802000;
		psxDisp.disabled = 1;
		psxDraw.offsetX = psxDraw.offsetY = psxDraw.clipX1 = psxDraw.clipX2 = psxDraw.clipY1 = psxDraw.clipY2 = 0;
		texinfo.mirror=0;
		texinfo.x = texinfo.y = 0;
		texinfo.colormode = texinfo.abr = 0;
		psxDisp.colordepth24 = FALSE;
		psxDisp.interlaced = FALSE;
		psxDisp.changed = 1;
		return;
	case 0x03:  
		psxDisp.disabled = (gdata & 1);
		if(psxDisp.disabled) 
			GPUstatusRet|=GPUSTATUS_DISPLAYDISABLED;
		else GPUstatusRet&=~GPUSTATUS_DISPLAYDISABLED;
		return;
	case 0x04:
		//gllog(77," GPU_S: TR_MODE: %x\n",gdata&0xffffff);
		if((gdata&0xffffff)==0) imageTransfer=0;
		if((gdata&0xffffff)==2) imageTransfer=3;
		GPUstatusRet&=~GPUSTATUS_DMABITS;                 // Clear the current settings of the DMA bits
		GPUstatusRet|=(gdata << 29);                      // Set the DMA bits according to the received data
		return;
   
	case 0x05:
		
		oldpsxDisp.startY = psxDisp.startY;
		oldpsxDisp.startX = psxDisp.startX;

		psxDisp.startY = (gdata>>10)&0x3ff;
		psxDisp.startX = gdata & 0x3ff;
		//gllog(77," GPU_S: DISPLAY SET: X=%d Y=%d\n",dispPosX,dispPosY);

		if (oldpsxDisp.startY != psxDisp.startY || oldpsxDisp.startX != psxDisp.startX)
			psxDisp.changed = 1;

		updateScreenMode();
		
		//if (dispLace==0) updateDisplay();
		return;
	case 0x06:
		psxDisp.rangeX1 = ( short ) ( gdata & 0x7ff );
		psxDisp.rangeX2 = ( short ) ( ( gdata>>12 ) & 0xfff );
		return;
	case 0x07:
		psxDisp.rangeY1 = ( short ) ( gdata & 0x3ff );
		psxDisp.rangeY2 = ( short ) ( ( gdata>>10 ) & 0x3ff );
		return;
	case 0x08:
		psxDisp.modeX = dispWidths[ (gdata&0x3)|((gdata&0x40)>>4) ];

		if (gdata&0x04) psxDisp.modeY = 480;
		else psxDisp.modeY = 240;

		psxDisp.colordepth24 = (gdata>>4)&0x1; // if 1 - TrueColor
		if (psxDisp.colordepth24)
			GPUstatusRet|=GPUSTATUS_RGB24;
		else GPUstatusRet&=~GPUSTATUS_RGB24;

		psxDisp.pal = (gdata & 0x08)?TRUE:FALSE; // if 1 - PAL mode, else NTSC
		if (psxDisp.pal)
			GPUstatusRet|=GPUSTATUS_PAL;
		else GPUstatusRet&=~GPUSTATUS_PAL;

		psxDisp.interlaced = (gdata>>5)&0x01; // if 1 - Interlace
		//gllog(77,"!GPU! DISPLAY SET: W=%d Wo=%d H=%d TRUE=%d LACE=%d\n",dispHorNew,dispWidths[gdata&0x3],dispVerNew,dispColorNew,dispLaceNew);
		if(psxDisp.modeY==480 && psxDisp.interlaced==1) drawLace = 1;
		else drawLace = 0;

		psxDisp.changed = 1;

		return;
	case 0x10: // ask about GPU version
		gdata&=0xff;
		
		switch(gdata) 
		{
		case 0x02:
			GPUdataRet=GPUInfoVals[INFO_TW];              // tw infos
			return;
		case 0x03:
			GPUdataRet=GPUInfoVals[INFO_DRAWSTART];       // draw start
			return;
		case 0x04:
			GPUdataRet=GPUInfoVals[INFO_DRAWEND];         // draw end
			return;
		case 0x05:
		case 0x06:
			GPUdataRet=GPUInfoVals[INFO_DRAWOFF];         // draw offset
			return;
		case 0x07:
			if(0)
				GPUdataRet=0x01;
			else GPUdataRet=0x02;                          // gpu type
			return;
		case 0x08:
		case 0x0F:                                       // some bios addr?
			GPUdataRet=0xBFC03720;
			return;
		default:
			gllog(77,"STATUS=%08x\n",gdata);
			return;
		}
		return;
	}
	return;
}

unsigned long CALLBACK GPUreadData(void)
{
	if(imageTransfer==2)
	{
		// ****
		//imageTransfer = 0;
		
		// image transfer from VRAM
		if ((imTY>=0) && (imTY<512) && (imTX>=0) && (imTX<1024))
		{
			GPUdataRet=psxVul[imTY*512+imTX/2];
			//gllog(11,"RD: %08x",GPUdataRet);
		}
		imTX+=2;
		imTXc-=2;
		if(imTXc<=0)
		{
			imTX=imageX0;
			imTXc=imageX1;
			imTYc--;
			imTY++;
		}
		imSize--;          
		if(imSize <= 0)
		{
			GPUstatusRet&=0xf7ffffff;
			imageTransfer=0;
		}
	}
	return GPUdataRet;
}

void psx24torgba(char* s, int len)
{
	int i;
	char *out = s;
	char r1,g1,b1,r2,g2,b2;

	for (i=0; i<len; i+=2)
	{
		g1 = *s++;
		r1 = *s++;
		r2 = *s++;
		b1 = *s++;
		b2 = *s++;
		g2 = *s++;

		*out++ = r1;
		*out++ = g1;
		*out++ = b1;
		*out++ = r2;
		*out++ = g2;
		*out++ = b2;
	}
}

int PullFromPsxRam(uint32_t *pMem, int size)
{
	int count = 0;
	unsigned short *input = (unsigned short*)pMem;
	uint32_t *t = vramWrite.extratarget;
	uint16_t *st = (uint16_t*)t;

	short x2 = vramWrite.x + vramWrite.w;
	short y2 = vramWrite.y + vramWrite.h;
	
	unsigned short posx, posy;

	if (vramWrite.enabled == 0) {
		imageTransfer = 0;
		return 0;
	}

	size <<=1;	//multiply by 2 for int to short;

	while(vramWrite.cury < y2)
	{
		posy = (unsigned short) vramWrite.cury;
		if (posy >= 512)
			posy = 0;
		while(vramWrite.curx < x2)
		{
			posx = (unsigned short) vramWrite.curx;
			if (posx >= 1024)
				posx = 0;

			if (!psxDisp.colordepth24)
				*t++ = torgba[*input];
			else
				*st++ = *input;

			psxVuw[(posy<<10)+posx] = *input;
			
			vramWrite.curx++;
			count++;
			input++;
			
			if (count == size)
			{
				if (vramWrite.curx == x2)
				{
					vramWrite.cury++;
					vramWrite.curx=vramWrite.x;
				}
				goto NOMOREIMAGEDATA;
			}
		}
		vramWrite.cury++;
		vramWrite.curx=vramWrite.x;
	}

NOMOREIMAGEDATA:
	if (vramWrite.cury >= y2)
	{
		float x,w;

		vramWrite.enabled = 0;

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadIdentity();
		glScalef(1.0f/1024.0f,1.0f/512.0f,1.0f);

		if (psxDisp.colordepth24)
		{
			x=vramWrite.x*2/3;
			w=vramWrite.w*2/3;
			//psx24torgba((char*)vramWrite.extratarget, vramWrite.w*vramWrite.h*2/3);
			glBindTexture(GL_TEXTURE_2D,xferTexture24);
			glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,vramWrite.h,GL_RGB,GL_UNSIGNED_BYTE,vramWrite.extratarget);
		}
		else
		{
			x=vramWrite.x;
			w=vramWrite.w;
			glBindTexture(GL_TEXTURE_2D,xferTexture16);
			glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,vramWrite.h,GL_RGBA,GL_UNSIGNED_BYTE,vramWrite.extratarget);
		}

		glDisable(GL_BLEND);
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
		
		glColor3ub(255,255,255);
		
		glBegin(GL_POLYGON);
			glTexCoord2s(0, 0);
			glVertex2s(x,vramWrite.y);

			glTexCoord2s(w, 0);
			glVertex2s(x+w,vramWrite.y);

			glTexCoord2s(w, vramWrite.h);
			glVertex2s(x+w,vramWrite.y+vramWrite.h);

			glTexCoord2s(0, vramWrite.h);
			glVertex2s(x,vramWrite.y+vramWrite.h);
		glEnd();
		glEnable(GL_BLEND);

		glPopMatrix();
	
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
		glEnable(GL_CLIP_PLANE2);
		glEnable(GL_CLIP_PLANE3);

		
		free(vramWrite.extratarget);
		imageTransfer = 0;
		if (count%2 == 1)
			count++;
	}

	return count>>1;	//back from short to int
}

void CALLBACK GPUwriteDataMem(uint32_t * pMem, int iSize)
{
	unsigned char command;
	int i = 0;
	uint32_t gdata;
	
	GPUIsBusy;
	GPUIsNotReadyForCommands;

	for(;i<iSize;)
	{
		if((imageTransfer & 1) == 1)
		{
			i += PullFromPsxRam(pMem, iSize-i);
			if (i >= iSize)
				continue;
			pMem += i;
		}
		
		gdata=*pMem;
		GPUdataRet=gdata;
		pMem++;
		i++;
		
		if(gpuDataC == 0)
		{
			command = (unsigned char) (gdata>>24) & 0xff;
			if (primTableC[command])
			{
				gpuDataC = primTableC[command];
				gpuCommand = command;
				gpuData[0] = gdata;
				gpuDataP = 1;
			}
			else
				continue;
		}
		else
		{
			gpuData[gpuDataP] = gdata;
			if ( gpuDataC>128 )
			{
				if ( ( gpuDataC==254 && gpuDataP>=3 ) ||
					( gpuDataC==255 && gpuDataP>=4 && ! ( gpuDataP&1 ) ) )
				{
					if ( ( gpuData[gpuDataP] & 0xF000F000 ) == 0x50005000 )
						gpuDataP=gpuDataC-1;
				}
			}
			gpuDataP++;
		}
		if(gpuDataP == gpuDataC)
		{
			gpuDataC=gpuDataP=0;
			primTableJ[gpuCommand]((unsigned char *)gpuData);
		}
	}

	GPUdataRet=gdata;

	GPUIsReadyForCommands;
	GPUIsIdle;
}

void CALLBACK GPUwriteData(uint32_t gdata)
{
	GPUwriteDataMem(&gdata,1);
}


// this function will be removed soon
void CALLBACK GPUsetMode(uint32_t gdata)
{
	imageTransfer = gdata;
	return;
}

// this function will be removed soon
long CALLBACK GPUgetMode(void)
{
	return imageTransfer;
}


long CALLBACK GPUconfigure(void)
{
	ExecCfg ("CFG");
	return 0;
}

uint32_t lUsedAddr[3];

__inline bool CheckForEndlessLoop(uint32_t laddr)
{
 if(laddr==lUsedAddr[1]) return TRUE;
 if(laddr==lUsedAddr[2]) return TRUE;

 if(laddr<lUsedAddr[0]) lUsedAddr[1]=laddr;
 else                   lUsedAddr[2]=laddr;
 lUsedAddr[0]=laddr;
 return FALSE;
}

long CALLBACK GPUdmaChain(uint32_t * baseAddrL, uint32_t addr)
{
 uint32_t dmaMem;
 unsigned char * baseAddrB;
 short count;unsigned int DMACommandCounter = 0;

 lUsedAddr[0]=lUsedAddr[1]=lUsedAddr[2]=0xffffff;

 baseAddrB = (unsigned char*) baseAddrL;

 do
  {
   addr&=0x1FFFFC;
   if(DMACommandCounter++ > 2000000) break;
   if(CheckForEndlessLoop(addr)) break;

   count = baseAddrB[addr+3];

   dmaMem=addr+4;

   if(count>0) GPUwriteDataMem(&baseAddrL[dmaMem>>2],count);

   addr = baseAddrL[addr>>2] & 0xffffff;
  }
 while (addr != 0xffffff);

 return 0;
}

void CALLBACK GPUkeypressed(int keycode)
{
	switch ( keycode )
	{
	case 0xFFC9:			//X11 key: F12
	case ( ( 1<<29 ) | 0xFF0D ) :	//special keycode from pcsx-df: alt-enter
		//bChangeWinMode=TRUE;
		break;
	case 0xffc2:		//F5
		//GPUmakeSnapshot();
		break;
	case 0x60:	//backtick `
		gpuConfig.FrameLimit = !gpuConfig.FrameLimit;
		break;
	}
}


void CALLBACK GPUabout(void)
{
	ExecCfg ("ABOUT");
}

long CALLBACK GPUtest(void)
{
	// if test fails this function should return negative value for error (unable to continue)
	// and positive value for warning (can continue but output might be crappy)
	return 0;
}


struct TGA_HEADER
{
    u8  identsize;          // size of ID field that follows 18 u8 header (0 usually)
    u8  colourmaptype;      // type of colour map 0=none, 1=has palette
    u8  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    s16 colourmapstart;     // first colour map entry in palette
    s16 colourmaplength;    // number of colours in palette
    u8  colourmapbits;      // number of bits per palette entry 15,16,24,32

    s16 xstart;             // image x origin
    s16 ystart;             // image y origin
    s16 width;              // image width in pixels
    s16 height;             // image height in pixels
    u8  bits;               // image bits per pixel 8,16,24,32
    u8  descriptor;         // image descriptor bits (vh flip bits)
    
    // pixel data follows header
    
} __attribute__((packed));


bool SaveTGA(const char* filename, int width, int height, void* pdata)
{
    struct TGA_HEADER hdr;
    FILE* f = fopen(filename, "wb");
    if( f == NULL )
        return 0;

    assert( sizeof(struct TGA_HEADER) == 18 && sizeof(hdr) == 18 );
    
    memset(&hdr, 0, sizeof(hdr));
    hdr.imagetype = 2;
    hdr.bits = 32;
    hdr.width = width;
    hdr.height = height;
    hdr.descriptor |= 8|(1<<5); // 8bit alpha, flip vertical

    fwrite(&hdr, sizeof(hdr), 1, f);
    fwrite(pdata, width*height*4, 1, f);
    fclose(f);
    return 1;
}
