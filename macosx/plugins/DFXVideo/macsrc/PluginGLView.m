/***************************************************************************
    PluginGLView.m
    PeopsSoftGPU
  
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

#import <Cocoa/Cocoa.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#import "PluginGLView.h"
#import "SGPUPreferences.h"
#include "externals.h"
#undef BOOL
#include "gpu.h"
#include "swap.h"

#include <time.h>
extern time_t tStart;

static int mylog2(int val)
{
	int i;
	for (i=1; i<31; i++)
		if (val <= (1 << i))
			return (1 << i);
	
	return -1;
}


@implementation PluginGLView
{
	GLubyte  *image_base;
	GLubyte  *image[IMAGE_COUNT];
	
	GLboolean useShader;
	float	  shaderQuality;
	GLint     buffers;
	GLuint    vertexShader;
	GLuint    fragmentShader;
	GLuint	  program;
	//GLint     frame_rate;
	
	GLenum    texture_hint;
	GLboolean rect_texture;
	GLboolean client_storage;
	GLboolean texture_range;
	
	struct timeval cycle_time;
	
	BOOL noDisplay;
	BOOL drawBG;
	
	int image_width;
	int image_height;
	int image_width2;
	int image_height2;
	int image_depth;
	int image_type;
	float image_tx;
	float image_ty;
	int whichImage;
	BOOL isFullscreen;
}
@synthesize glLock;

//- (id)initWithFrame:(NSRect)frameRect
- (id) initWithCoder: (NSCoder *) coder
{
	if ((self = [super initWithCoder:coder]) == nil)
		return nil;
	
	glLock = [[NSLock alloc] init];
	if (nil == glLock) {
		return nil;
	}
	
	const GLubyte * strExt;
	
	// Init pixel format attribs
	static const NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		0
	};
	
	// Get pixel format from OpenGL
	NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
	if (!pixFmt)
	{
		NSLog(@"No Accelerated OpenGL pixel format found\n");
		
		static const NSOpenGLPixelFormatAttribute attrs2[] =
		{
			NSOpenGLPFANoRecovery,
			0
		};
		
		// Get pixel format from OpenGL
		pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs2];
		if (!pixFmt) {
			NSLog(@"No OpenGL pixel format found!\n");
			
			return nil;
		}
	}
	
	[self setPixelFormat:pixFmt];
	
	/*
	 long swapInterval = 1 ;
	 [[self openGLContext]
	 setValues:&swapInterval
	 forParameter:NSOpenGLCPSwapInterval];
	 */
	[glLock lock];
	[[self openGLContext] makeCurrentContext];
	
	// Init object members
	strExt = glGetString (GL_EXTENSIONS);
	texture_range  = gluCheckExtension ((const unsigned char *)"GL_APPLE_texture_range", strExt) ? GL_TRUE : GL_FALSE;
	texture_hint   = GL_STORAGE_SHARED_APPLE ;
	client_storage = gluCheckExtension ((const unsigned char *)"GL_APPLE_client_storage", strExt) ? GL_TRUE : GL_FALSE;
	rect_texture   = gluCheckExtension((const unsigned char *)"GL_EXT_texture_rectangle", strExt) ? GL_TRUE : GL_FALSE;
	
	// Setup some basic OpenGL stuff
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Loads the shaders
	
	if(isShaderEnabled()){
		rect_texture = GL_FALSE;
		// --- Params ---
		shaderQuality = PSXShaderQuality();
		vertexShader = [self loadShader:GL_VERTEX_SHADER location:PSXVertexShader()];
		fragmentShader = [self loadShader:GL_FRAGMENT_SHADER location:PSXFragmentShader()];
		
		//--- shader loading ---
		program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
		glUseProgram(program);
	}
	
	
	[NSOpenGLContext clearCurrentContext];
	[glLock unlock];
	
	image_width = 1024;
	image_height = 512;
	image_depth = 16;
	
	image_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
	image_base = (GLubyte *) calloc(((IMAGE_COUNT * image_width * image_height) / 3) * 4, image_depth >> 3);
	if (image_base == nil) {
		return nil;
	}
	
	// Create and load textures for the first time
	[self loadTextures:GL_TRUE];
	
	// Init fps timer
	//gettimeofday(&cycle_time, NULL);
	
	drawBG = YES;
	
	// Call for a redisplay
	noDisplay = YES;
	PSXDisplay.Disabled = 1;
	[self setNeedsDisplay:YES];
	
	return self;
}

- (void)dealloc
{
	int i;
	
	[glLock lock];
	
	[[self openGLContext] makeCurrentContext];
	for(i = 0; i < IMAGE_COUNT; i++)
	{
		GLuint dt = i+1;
		glDeleteTextures(1, &dt);
	}
	if(texture_range)
		glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, IMAGE_COUNT * image_width * image_height * (image_depth >> 3), image_base);
	
	[NSOpenGLContext clearCurrentContext];
	[glLock unlock];
	
	if (image_base) {
		free(image_base);
	}
}

- (BOOL)isOpaque
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return NO;
}

- (void)drawRect:(NSRect)aRect
{
	// Check if an update has occured to the buffer
	if ([self lockFocusIfCanDraw]) {
		
		// Make this context current
		if (drawBG) {
			[[NSColor blackColor] setFill];
			[NSBezierPath fillRect:[self visibleRect]];
		}
		
		//glFinish() ;
		// Swap buffer to screen
		//[[self openGLContext] flushBuffer];
		
		[self unlockFocus];
	}
}

#if 0
- (void)update  // moved or resized
{
	NSRect rect;

	[super update];

	[[self openGLContext] makeCurrentContext];
	[[self openGLContext] update];

	rect = [self bounds];
	
	glViewport(0, 0, (int) rect.size.width, (int) rect.size.height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); 

	//[self setNeedsDisplay:true];
}
#endif

- (void)reshape	// scrolled, moved or resized
{
	[glLock lock];
	
	[super reshape];
	
	NSOpenGLContext *oglContext = [self openGLContext];
	NSRect rect;
	
	[oglContext makeCurrentContext];
	[oglContext update];
	
	rect = [[oglContext view] bounds];
	
	glViewport(0, 0, (int) rect.size.width, (int) rect.size.height);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	drawBG = YES;
	
	[NSOpenGLContext clearCurrentContext];
	
	//[self setNeedsDisplay:true];
	[self renderScreen];
	[glLock unlock];
}

- (void)renderScreen
{
	int bufferIndex = whichImage;
	
	if (1/*[glLock tryLock]*/) {
		// Make this context current
		[[self openGLContext] makeCurrentContext];
		
		if (PSXDisplay.Disabled) {
			glClear(GL_COLOR_BUFFER_BIT);
		} else {
			// Bind, update and draw new image
			if(rect_texture && isShaderEnabled() == NO) // cant go in there if we use shaders
			{
				//printf("Texture Rectangle\n");
				//glActiveTexture(bufferIndex+1);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_RECTANGLE_EXT, bufferIndex+1);
				
				glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, 0, 0, image_width, image_height, GL_BGRA, image_type, image[bufferIndex]);
				
				
				glBegin(GL_QUADS);
				{
					glTexCoord2f(0.0f, 0.0f);
					glVertex2f(-1.0f, 1.0f);
					
					glTexCoord2f(0.0f, image_height);
					glVertex2f(-1.0f, -1.0f);
					
					glTexCoord2f(image_width, image_height);
					glVertex2f(1.0f, -1.0f);
					
					glTexCoord2f(image_width, 0.0f);
					glVertex2f(1.0f, 1.0f);
				}
				glEnd();
			}
			else
			{
				NSRect rect = [[[self openGLContext] view] bounds];
				//printf("Texture 2D normale de taille : %d, %d sur un ecran : %f x %f \n",image_width,image_height,rect.size.width,rect.size.height);
				//glActiveTexture(whichImage+1);
				glBindTexture(GL_TEXTURE_2D, whichImage+1);
				
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_width2, image_height2, GL_BGRA, image_type, image[bufferIndex]);
				
				
				if(isShaderEnabled()){
					glUseProgram(program);
					
					int loc = glGetUniformLocation(program, "OGL2Texture");
					glUniform1i(loc,0);
					int loc2 = glGetUniformLocation(program, "OGL2Param");
					float param[4];
					param[2] = shaderQuality;
					param[0] = param[2] / image_width;
					param[1] = param[2] / image_height;
					//param[2]=2.0;
					param[3] = 0.0;
					int loc3 = glGetUniformLocation(program, "OGL2Size");
					float size[4];
					//NSRect rect = [[[self openGLContext] view] bounds];
					size[0] = image_width;
					size[1] = image_height;
					size[2] = rect.size.width;
					size[3] = rect.size.height;
					int loc4 = glGetUniformLocation(program, "OGL2InvSize");
					float invSize[4];
					invSize[0] = 1.0/size[0];
					invSize[1] = 1.0/size[1];
					invSize[2] = 1.0/size[2];
					invSize[3] = 1.0/size[3];
					//invSize[4]=1.0/size[4]; //Did we goof here?
					glUniform4fv(loc2, 1, param);
					glUniform4fv(loc3, 1, size);
					glUniform4fv(loc4, 1, invSize);
				}
				
				glBegin(GL_QUADS);
				{
					glTexCoord2f(0.0f, 0.0f);
					glVertex2f(-1.0f, 1.0f);
					
					glTexCoord2f(0.0f, image_ty);
					glVertex2f(-1.0f, -1.0f);
					
					glTexCoord2f(image_tx, image_ty);
					glVertex2f(1.0f, -1.0f);
					
					glTexCoord2f(image_tx, 0.0f);
					glVertex2f(1.0f, 1.0f);
				}
				glEnd();
			}
		}
		
		// FPS Display
		if(ulKeybits&KEY_SHOWFPS)
		{
			int len, i;
			if(szDebugText[0] && ((time(NULL) - tStart) < 2))
			{
				strlcpy(szDispBuf, szDebugText, 63);
			}
			else
			{
				szDebugText[0]=0;
				if (szMenuBuf) {
					strncat(szDispBuf, szMenuBuf, 63 - strlen(szDispBuf));
				}
			}
			
			NSRect rect = [[[self openGLContext] view] bounds];
			len = (int) strlen(szDispBuf);
			
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			
			gluOrtho2D(0.0, rect.size.width, 0.0, rect.size.height);
			glDisable(rect_texture ? GL_TEXTURE_RECTANGLE_EXT : GL_TEXTURE_2D);
			
			glColor4f(0.0, 0.0, 0.0, 0.5);
			glRasterPos2f(3.0, rect.size.height - 14.0);
			for (i = 0; i < len; i++) {
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, szDispBuf[i]);
			}
			
			glColor3f(1.0, 1.0, 1.0);
			glRasterPos2f(2.0, rect.size.height - 13.0);
			for (i = 0; i < len; i++) {
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, szDispBuf[i]);
			}
			
			glEnable(rect_texture ? GL_TEXTURE_RECTANGLE_EXT : GL_TEXTURE_2D);
			glPopMatrix();
		}
		
		//printProgramInfoLog(program);
		//printf("\n\n\n");
		[[self openGLContext] flushBuffer];
		[NSOpenGLContext clearCurrentContext];
		//[glLock unlock];
	}
}

- (void)loadTextures:(GLboolean)first
{
	GLint i;
	printf("Loading texture\n");
	//[glLock lock];
	[[self openGLContext] makeCurrentContext];
	
	image_width = PreviousPSXDisplay.Range.x1;
	image_height = PreviousPSXDisplay.DisplayMode.y;
	if (PSXDisplay.RGB24) {
		image_depth = 32;
		image_type = GL_UNSIGNED_INT_8_8_8_8_REV;
	} else {
		image_depth = 16;
		image_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		//image_width >>= 1;
	}
	
	if (image_width * image_height * (image_depth >> 3) > ((1024*512*2)/3)*4)
		printf("Fatal error: desired dimension are too large! (%ix%i %ibpp)\n",
			   image_width, image_height, image_depth);
	
	for(i = 0; i < IMAGE_COUNT; i++)
		image[i] = image_base + i * image_width * image_height * (image_depth >> 3);
	
	if(rect_texture)
	{
		image_width2 = image_width;
		image_height2 = image_height;
		image_tx = (float)image_width;
		image_ty = (float)image_height;
		
		if(texture_range) glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, IMAGE_COUNT * image_width * image_height * (image_depth >> 3), image_base);
		else              glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, 0, NULL);
		
		for(i = 0; i < IMAGE_COUNT; i++)
		{
			if(!first)
			{
				GLuint dt = i+1;
				glDeleteTextures(1, &dt);
			}
			
			glDisable(GL_TEXTURE_2D);
			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_RECTANGLE_EXT);
			glBindTexture(GL_TEXTURE_RECTANGLE_EXT, i+1);
			
			
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE , texture_hint);
			glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, client_storage);
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			
			glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, image_width,
						 image_height, 0, GL_BGRA, image_type, image[i]);
		}
	}
	else
	{
		image_width2 = mylog2(image_width);
		image_height2 = mylog2(image_height);
		image_tx = (float)image_width/(float)image_width2;
		image_ty = (float)image_height/(float)image_height2;
		
		glTextureRangeAPPLE(GL_TEXTURE_RECTANGLE_EXT, 0, NULL);
		if(texture_range) glTextureRangeAPPLE(GL_TEXTURE_2D, IMAGE_COUNT * image_width2 * image_height2 * (image_depth >> 3), image_base);
		else              glTextureRangeAPPLE(GL_TEXTURE_2D, 0, NULL);
		
		for(i = 0; i < IMAGE_COUNT; i++)
		{
			if(!first)
			{
				GLuint dt = i+1;
				glDeleteTextures(1, &dt);
			}
			
			glDisable(GL_TEXTURE_RECTANGLE_EXT);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, i+1);
			
			//if(texture_range) glTextureRangeAPPLE(GL_TEXTURE_2D, IMAGE_COUNT * image_width2 * image_height2 * (image_depth >> 3), image_base);
			//else              glTextureRangeAPPLE(GL_TEXTURE_2D, 0, NULL);
			
			glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE , texture_hint);
			glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, client_storage);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width2,
						 image_height2, 0, GL_BGRA, image_type, image[i]);
		}
	}
	
	[NSOpenGLContext clearCurrentContext];
	//[glLock unlock];
}

- (void)swapBuffer
{
	RunOnMainThreadSync(^{
		//printf("y=%i",PSXDisplay.DisplayPosition.y);
		
		unsigned char * surf;
		long x = PSXDisplay.DisplayPosition.x;
		long y = PSXDisplay.DisplayPosition.y;
		GLuint lu;
		unsigned short row,column;
		unsigned short dx=PreviousPSXDisplay.Range.x1;
		unsigned short dy=PreviousPSXDisplay.DisplayMode.y;
		long lPitch;
		
		if ([glLock tryLock]) {
			// make sure the texture area is ready to be written to
			glFinishObjectAPPLE(GL_TEXTURE, 2-whichImage);
			
			if ((image_width != PreviousPSXDisplay.Range.x1) ||
				(image_height != PreviousPSXDisplay.DisplayMode.y) ||
				((PSXDisplay.RGB24 ? 32 : 16) != image_depth)) {
				[self loadTextures:GL_FALSE];
			}
			
			surf = image[1-whichImage];
			lPitch=image_width2<<(image_depth >> 4);
			
			if(PreviousPSXDisplay.Range.y0)                       // centering needed?
			{
				surf+=PreviousPSXDisplay.Range.y0*lPitch;
				dy-=PreviousPSXDisplay.Range.y0;
			}
			
			if(PSXDisplay.RGB24)
			{
				unsigned char * pD;
				size_t startxy;
				
				surf+=PreviousPSXDisplay.Range.x0<<2;
				
				for(column=0;column<dy;column++)
				{
					startxy = (1024 * (column + y)) + x;
					pD = (unsigned char *)&psxVuw[startxy];
					
					row = 0;
					// make sure the reads are aligned
					while ((intptr_t)pD & 0x3) {
						*((unsigned long *)((surf)+(column*lPitch)+(row<<2))) =
						(*(pD+0)<<16)|(*(pD+1)<<8)|*(pD+2);
						
						pD+=3;
						row++;
					}
					
					for(;row<dx;row+=4)
					{
						GLuint lu1 = *((GLuint *)pD);
						GLuint lu2 = *((GLuint *)pD+1);
						GLuint lu3 = *((GLuint *)pD+2);
						GLuint *dst = ((GLuint *)((surf)+(column*lPitch)+(row<<2)));
#ifdef __BIG_ENDIAN__
						*(dst)=
						(((lu1>>24)&0xff)<<16)|(((lu1>>16)&0xff)<<8)|(((lu1>>8)&0xff));
						*(dst+1)=
						(((lu1>>0)&0xff)<<16)|(((lu2>>24)&0xff)<<8)|(((lu2>>16)&0xff));
						*(dst+2)=
						(((lu2>>8)&0xff)<<16)|(((lu2>>0)&0xff)<<8)|(((lu3>>24)&0xff));
						*(dst+3)=
						(((lu3>>16)&0xff)<<16)|(((lu3>>8)&0xff)<<8)|(((lu3>>0)&0xff));
#else
						*(dst)=
						(((lu1>>0)&0xff)<<16)|(((lu1>>8)&0xff)<<8)|(((lu1>>16)&0xff));
						*(dst+1)=
						(((lu1>>24)&0xff)<<16)|(((lu2>>0)&0xff)<<8)|(((lu2>>8)&0xff));
						*(dst+2)=
						(((lu2>>16)&0xff)<<16)|(((lu2>>24)&0xff)<<8)|(((lu3>>0)&0xff));
						*(dst+3)=
						(((lu3>>8)&0xff)<<16)|(((lu3>>16)&0xff)<<8)|(((lu3>>24)&0xff));
#endif
						pD+=12;
					}
					
					//for(;row<dx;row+=4)
					/*while (pD&0x3) {
					 *((unsigned long *)((surf)+(column*lPitch)+(row<<2)))=
					 (*(pD+0)<<16)|(*(pD+1)<<8)|(*(pD+2)&0xff));
					 pD+=3;
					 row++;
					 }*/
				}
			}
			else
			{
				long LineOffset,SurfOffset;
				GLuint * SRCPtr = (GLuint *)(psxVuw + (y << 10) + x);
				GLuint * DSTPtr =
				((GLuint *)surf) + (PreviousPSXDisplay.Range.x0 >> 1);
				
				dx >>= 1;
				
				LineOffset = 512 - dx;
				SurfOffset = (lPitch >> 2) - dx;
				
				for(column=0;column<dy;column++)
				{
					for(row=0;row<dx;row++)
					{
#ifdef __BIG_ENDIAN__
						lu=GETLE16D(SRCPtr++);
#else
						lu=*SRCPtr++;
#endif
						*DSTPtr++=
						((lu << 10) & 0x7c007c00)|
						((lu) & 0x3e003e0)|
						((lu >> 10) & 0x1f001f);
					}
					SRCPtr += LineOffset;
					DSTPtr += SurfOffset;
				}
			}
			
			// Swap image buffer
			whichImage = 1 - whichImage;
			
			[self renderScreen];
			[glLock unlock];
		}

	});
}

- (void)clearBuffer:(BOOL)display
{
	if (display == NO) {
		//[[self openGLContext] makeCurrentContext];
		//glClear(GL_COLOR_BUFFER_BIT);
		//[self loadTextures:NO];
	} else {
		noDisplay = YES;
		//[self setNeedsDisplay:true];
	}
}

- (GLuint)loadShader:(GLenum)type location:(NSURL*)filename
{
	GLuint myShader = 0;
	
	GLsizei logsize = 0;
	GLint compile_status = GL_TRUE;
	char *log = NULL;
	char *src = NULL;
	
	/* creation d'un shader de sommet */
	myShader = glCreateShader(type);
	if(myShader == 0)
	{
		NSLog(@"impossible de creer le shader");
		return 0;
	}
	
	/* chargement du code source */
	src = [PluginGLView loadSource:filename];
	if(src == NULL)
	{
		/* theoriquement, la fonction LoadSource a deja affiche un message
		 d'erreur, nous nous contenterons de supprimer notre shader
		 et de retourner 0 */
		
		glDeleteShader(myShader);
		return 0;
	}
	
	/* assignation du code source */
	glShaderSource(myShader, 1, (const GLchar**)&src, NULL);
	
	/* compilation du shader */
	glCompileShader(myShader);
	
	/* liberation de la memoire du code source */
	free(src);
	src = NULL;
	
	/* verification du succes de la compilation */
	glGetShaderiv(myShader, GL_COMPILE_STATUS, &compile_status);
	if(compile_status != GL_TRUE)
	{
		/* erreur a la compilation recuperation du log d'erreur */
		
		/* on recupere la taille du message d'erreur */
		glGetShaderiv(myShader, GL_INFO_LOG_LENGTH, &logsize);
		
		/* on alloue un espace memoire dans lequel OpenGL ecrira le message */
		log = calloc(logsize + 1, 1);
		if(log == NULL)
		{
			NSLog(@"impossible d'allouer de la memoire!");
			return 0;
		}
		
		glGetShaderInfoLog(myShader, logsize, &logsize, log);
		NSLog(@"impossible de compiler le shader '%@' :\n%s",
			  [filename path], log);
		
		/* ne pas oublier de liberer la memoire et notre shader */
		free(log);
		glDeleteShader(myShader);
		
		return 0;
	}
	return myShader;
}

+ (char*)loadSource:(NSURL *)filename
{
	//Since we're passing Cocoa NSURLs, let's use Cocoa's methods
	if (filename == nil) {
		return NULL;
	}
	
	NSUInteger len;
	NSMutableData *shaderData = [[NSMutableData alloc] initWithContentsOfURL:filename];
	[shaderData appendBytes:"\0" length:1];
	len = [shaderData length];
	char *shaderText = malloc(len);
	[shaderData getBytes:shaderText length:len];
	return shaderText;
}

@end
