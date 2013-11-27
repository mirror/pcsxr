//
//  GL3Code.m
//  Pcsxr
//
//  Created by C.W. Betts on 11/26/13.
//
//

#import <Cocoa/Cocoa.h>
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#import <GLKit/GLKit.h>
#import "PluginGLView.h"
#import "SGPUPreferences.h"
#include "externals.h"
#undef BOOL
#include "gpu.h"
#include "swap.h"

static int mylog2(int val)
{
	int i;
	for (i=1; i<31; i++)
		if (val <= (1 << i))
			return (1 << i);
	
	return -1;
}

@implementation PluginGLView (GL3)

- (BOOL)setupOpenGL3
{
	static const NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAOpenGLProfile,
		NSOpenGLProfileVersion3_2Core,
		0
	};
	
	// Get pixel format from OpenGL
	NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
	if (!pixFmt)
		return NO;
	
	[self setPixelFormat:pixFmt];

	return NO;
}

- (void)cleanupGL3
{
	
}

- (void)reshapeGL3
{
	NSOpenGLContext *oglContext = [self openGLContext];
	NSRect rect;
	
	[oglContext makeCurrentContext];
	[oglContext update];
	
	rect = [[oglContext view] bounds];
	
	glViewport(0, 0, (int) rect.size.width, (int) rect.size.height);
	
	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	
	drawBG = YES;
	
	[NSOpenGLContext clearCurrentContext];
}

- (void)renderScreenGL3
{
	NSRect rect = [[[self openGLContext] view] bounds];

	GLKMatrix4 matrix = GLKMatrix4MakeOrtho(0, rect.size.width, 0, rect.size.height, 0, 0);
}

- (void)loadTexturesGL3:(GLboolean)first
{
	
}

- (void)swapBufferGL3
{
	
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
		fprintf(stderr, "impossible de creer le shader\n");
		return 0;
	}
	
	/* chargement du code source */
	src = [self loadSource:filename];
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
		log = malloc(logsize + 1);
		if(log == NULL)
		{
			fprintf(stderr, "impossible d'allouer de la memoire !\n");
			return 0;
		}
		/* initialisation du contenu */
		memset(log, '\0', logsize + 1);
		
		glGetShaderInfoLog(myShader, logsize, &logsize, log);
		fprintf(stderr, "impossible de compiler le shader '%s' :\n%s",
				[[filename path] UTF8String], log);
		
		/* ne pas oublier de liberer la memoire et notre shader */
		free(log);
		glDeleteShader(myShader);
		
		return 0;
	}
    return myShader;
}

@end

void printProgramInfoLog(GLuint obj)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;
	
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
	
	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
		free(infoLog);
	}
}
