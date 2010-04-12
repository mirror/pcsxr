//
//  main.m
//
//  Created by Gil Pedersen on Fri Jun 06 2003.
//  Copyright (c) 2003 SoftWorkz. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import "EmuThread.h"
#include <dlfcn.h>
//#import <sys/param.h>
#import <unistd.h>
#include "psxcommon.h"
#include "sio.h"

static BOOL sysInited = NO;
//#define EMU_LOG

int main(int argc, const char *argv[])
{
    if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
        char parentdir[MAXPATHLEN];
        char *c;

        strncpy ( parentdir, argv[0], sizeof(parentdir) );
        c = (char*) parentdir;

        while (*c != '\0')     /* go to end */
               c++;

        while (*c != '/')      /* back up to parent */
               c--;

        *c++ = '\0';           /* cut off last part (binary name) */

        assert ( chdir (parentdir) == 0 );   /* chdir to the binary app's parent */
        assert ( chdir ("../../../") == 0 ); /* chdir to the .app's parent */
    }

    strcpy(Config.BiosDir,    "Bios/");
    strcpy(Config.PatchesDir, "Patches/");

    // Setup the X11 window
    if (getenv("DISPLAY") == NULL)
        setenv("DISPLAY", ":0.0", 0); // Default to first local display

    return NSApplicationMain(argc, argv);
}

int SysInit() {
	if (!sysInited) {
#ifdef EMU_LOG
#ifndef LOG_STDOUT
		emuLog = fopen("emuLog.txt","wb");
#else
		emuLog = stdout;
#endif
		setvbuf(emuLog, NULL, _IONBF, 0);
#endif

		if (psxInit() != 0)
			return -1;

		sysInited = YES;
	}

	if (LoadPlugins() == -1) {
		return -1;
	}

	LoadMcds(Config.Mcd1, Config.Mcd2);

	return 0;
}

void SysReset() {
    [EmuThread resetNow];
    //psxReset();
}

void SysPrintf(char *fmt, ...) {
    va_list list;
    char msg[512];

    va_start(list, fmt);
    vsprintf(msg, fmt, list);
    va_end(list);

    if (Config.PsxOut) printf ("%s", msg);
#ifdef EMU_LOG
#ifndef LOG_STDOUT
    fprintf(emuLog, "%s", msg);
#endif
#endif
}

void SysMessage(char *fmt, ...)
{
	va_list list;
	char msg[512];

	NSString *locFmtString = NSLocalizedString([NSString stringWithCString:fmt], nil);

	va_start(list, fmt);
	vsprintf(msg, [locFmtString lossyCString], list);
	va_end(list);

	NSRunAlertPanel(NSLocalizedString(@"Error!", nil),
		[NSString stringWithCString:msg], 
		nil, nil, nil);
}

#if 1
void *SysLoadLibrary(char *lib) {
	NSBundle *bundle = [NSBundle bundleWithPath:[NSString stringWithCString:lib]];
	if (bundle != nil) {
		return dlopen([[bundle executablePath] fileSystemRepresentation], RTLD_LAZY /*RTLD_NOW*/);
	}
	return dlopen(lib, RTLD_LAZY);
}

void *SysLoadSym(void *lib, char *sym) {
	return dlsym(lib, sym);
}

const char *SysLibError() {
	return dlerror();
}

void SysCloseLibrary(void *lib) {
	//dlclose(lib);
}
#else
static int LoadResult = 0;
void *SysLoadLibrary(char *lib)
{
    CFBundleRef myBundle;
    CFURLRef    bundleURL;
    CFStringRef	path;

    path = CFStringCreateWithCString(kCFAllocatorDefault, lib, CFStringGetSystemEncoding());
    if (!path)
        goto error;

    bundleURL = CFURLCreateWithFileSystemPath( 
                    kCFAllocatorDefault, 
                    path,
                    kCFURLPOSIXPathStyle,
                    true );
    if (!bundleURL)
        goto error;

    myBundle = CFBundleCreate( kCFAllocatorDefault, bundleURL );

    if (!CFBundleLoadExecutable(myBundle))
        goto error;

good:
    if (path) CFRelease(path);
    if (bundleURL) CFRelease(bundleURL);

    return myBundle;
    
error:
    if (LoadResult == 0) LoadResult = -1;
    myBundle = NULL;
    goto good;
}

void *SysLoadSym(void *lib, char *sym)
{
    CFStringRef funcName;
    void *func;
    
    funcName = CFStringCreateWithCString(kCFAllocatorDefault, sym, CFStringGetSystemEncoding());
    func = CFBundleGetFunctionPointerForName((CFBundleRef)lib, funcName);
    if (func == nil) {
        LoadResult = -2;
    }
    
    CFRelease(funcName);
    return func;
}

const char *SysLibError()
{
    char *result = NULL;
    
    if (LoadResult != 0)
        result = "Error loading code";
    
    LoadResult = 0;
    return result;
}
void SysCloseLibrary(void *lib)
{
    if (lib) {
        CFBundleUnloadExecutable((CFBundleRef)lib);
        CFRelease((CFBundleRef)lib);
    }
}
#endif

void PADhandleKey(int key);

// Called periodically from the emu thread
void SysUpdate()
{
	UpdateSystemActivity(UsrActivity);

	PADhandleKey(PAD1_keypressed());
	//PADhandleKey(PAD2_keypressed());

	[emuThread handleEvents];
}

void SysRunGui() // Returns to the Gui
{
    
}
void SysClose() // Close mem and plugins
{
    psxShutdown();
    ReleasePlugins();

    if (emuLog != NULL) fclose(emuLog);

    sysInited = NO;
}

void OnFile_Exit()
{
    SysClose();
    exit(0);
}
