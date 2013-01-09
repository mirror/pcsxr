//
//  PcsxrPlugin.m
//  Pcsxr
//
//  Created by Gil Pedersen on Fri Oct 03 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PcsxrPlugin.h"
#include "psxcommon.h"
#include "plugins.h"
#import "ARCBridge.h"

@implementation PcsxrPlugin

+ (NSString *)prefixForType:(int)aType
{
    switch (aType) {
        case PSE_LT_GPU: return @"GPU";
        case PSE_LT_CDR: return @"CDR";
        case PSE_LT_SPU: return @"SPU";
        case PSE_LT_PAD: return @"PAD";
        case PSE_LT_NET: return @"NET";
    }
    
    return @"";
}

+ (NSString *)defaultKeyForType:(int)aType
{
    //return @"Plugin" [PcsxrPlugin prefixForType:aType];
    switch (aType) {
        case PSE_LT_GPU:
        case PSE_LT_CDR:
        case PSE_LT_SPU:
        case PSE_LT_PAD:
        case PSE_LT_NET:
            return [NSString stringWithFormat:@"Plugin%@", [PcsxrPlugin prefixForType:aType]];
            break;
        default:
            return @"";
            break;
    }
}

+ (char **)configEntriesForType:(int)aType
{
	static char *gpu[2] = {(char *)&Config.Gpu, NULL};
	static char *cdr[2] = {(char *)&Config.Cdr, NULL};
	static char *spu[2] = {(char *)&Config.Spu, NULL};
	static char *pad[3] = {(char *)&Config.Pad1, (char *)&Config.Pad2, NULL};
	static char *net[2] = {(char *)&Config.Net, NULL};

    switch (aType) {
        case PSE_LT_GPU: return (char **)gpu;
        case PSE_LT_CDR: return (char **)cdr;
        case PSE_LT_SPU: return (char **)spu;
        case PSE_LT_PAD: return (char **)pad;
        case PSE_LT_NET: return (char **)net;
    }

    return nil;
}

+ (NSArray *)pluginsPaths
{
    static NSArray *returnArray = nil;
    if (returnArray == nil)
    {
        NSURL *supportURL = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:NULL];
        NSURL *libraryURL = [[NSFileManager defaultManager] URLForDirectory:NSLibraryDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:NULL];
        NSURL *localSupportURL = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSLocalDomainMask appropriateForURL:nil create:YES error:NULL];
        NSURL *localLibraryURL = [[NSFileManager defaultManager] URLForDirectory:NSLibraryDirectory inDomain:NSLocalDomainMask appropriateForURL:nil create:YES error:NULL];
    
        NSMutableArray *mutArray = [NSMutableArray arrayWithCapacity:5];
    
        [mutArray addObject:[[NSFileManager defaultManager] stringWithFileSystemRepresentation:Config.PluginsDir length:strlen(Config.PluginsDir)]];
        NSURL *url = [localLibraryURL URLByAppendingPathComponent:@"Playstation Emulator Plugins"];
        if ([url checkResourceIsReachableAndReturnError:NULL])
            [mutArray addObject:[url path]];
        url = [[localSupportURL URLByAppendingPathComponent:@"Pcsxr"] URLByAppendingPathComponent:@"PlugIns"];
        if ([url checkResourceIsReachableAndReturnError:NULL])
            [mutArray addObject:[url path]];
        url = [libraryURL URLByAppendingPathComponent:@"Playstation Emulator Plugins"];
        if ([url checkResourceIsReachableAndReturnError:NULL])
            [mutArray addObject:[url path]];
        url = [[supportURL URLByAppendingPathComponent:@"Pcsxr"] URLByAppendingPathComponent:@"PlugIns"];
        if ([url checkResourceIsReachableAndReturnError:NULL])
            [mutArray addObject:[url path]];
        returnArray = [[NSArray alloc] initWithArray:mutArray];
    }
    return returnArray;
}

- (id)initWithPath:(NSString *)aPath 
{
    if (!(self = [super init])) {
        return nil;
    }
    
    PSEgetLibType    PSE_getLibType = NULL;
    PSEgetLibVersion PSE_getLibVersion = NULL;
    PSEgetLibName    PSE_getLibName = NULL;
    
    pluginRef = nil;
    name = nil;
    path = [aPath copy];
    long tempVers = 0;
    NSString *goodPath = nil;
    for (NSString *plugDir in [PcsxrPlugin pluginsPaths]) 
    {
        NSString *fullPath = [plugDir stringByAppendingPathComponent:path];
        if ([[NSFileManager defaultManager] fileExistsAtPath:fullPath]) {
            void *tempHandle = SysLoadLibrary([fullPath fileSystemRepresentation]);
            if (tempHandle != NULL)
            {
                PSEgetLibVersion tempLibVersion = SysLoadSym(tempHandle, "PSEgetLibVersion");
                if (SysLibError() == NULL)
                {
                    long tempVers2 = tempLibVersion();
                    if (tempVers <= tempVers2 ){
                        goodPath = fullPath;
                        tempVers = tempVers2;
                    }
                }
                SysCloseLibrary(tempHandle);
            }
        }
    }
	
    if (goodPath == nil) {
        RELEASEOBJ(self);
        return nil;
    }
	
    pluginRef = SysLoadLibrary([goodPath fileSystemRepresentation]);
    if (pluginRef == nil) {
        RELEASEOBJ(self);
        return nil;
    }

    // TODO: add support for plugins with multiple functionalities???
    PSE_getLibType = (PSEgetLibType) SysLoadSym(pluginRef, "PSEgetLibType");
    if (SysLibError() != nil) {
        if (([path rangeOfString: @"gpu" options:NSCaseInsensitiveSearch]).length != 0)
            type = PSE_LT_GPU;
        else if (([path rangeOfString: @"cdr" options:NSCaseInsensitiveSearch]).length != 0)
            type = PSE_LT_CDR;
        else if (([path rangeOfString: @"spu" options:NSCaseInsensitiveSearch]).length != 0)
            type = PSE_LT_SPU;
        else if (([path rangeOfString: @"pad" options:NSCaseInsensitiveSearch]).length != 0)
            type = PSE_LT_PAD;
        else if (([path rangeOfString: @"net" options:NSCaseInsensitiveSearch]).length != 0)
            type = PSE_LT_NET;
        else {
            RELEASEOBJ(self);
            return nil;
        }
    } else {
        type = (int)PSE_getLibType();
        if (type != PSE_LT_GPU && type != PSE_LT_CDR && type != PSE_LT_SPU && type != PSE_LT_PAD && type != PSE_LT_NET) {
            RELEASEOBJ(self);
            return nil;
        }
    }
    
    PSE_getLibName = (PSEgetLibName) SysLoadSym(pluginRef, "PSEgetLibName");
    if (SysLibError() == nil) {
        name = [[NSString alloc] initWithCString:PSE_getLibName() encoding:NSUTF8StringEncoding];
    }
    
    PSE_getLibVersion = (PSEgetLibVersion) SysLoadSym(pluginRef, "PSEgetLibVersion");
    if (SysLibError() == nil) {
        version = PSE_getLibVersion();
    } else {
        version = -1;
    }
    
    // save the current modification date
    NSDictionary *fattrs = [[NSFileManager defaultManager] attributesOfItemAtPath:[goodPath stringByResolvingSymlinksInPath] error:NULL];
    modDate = RETAINOBJ([fattrs fileModificationDate]);
    fullPlugPath = RETAINOBJ(goodPath);
    
    active = 0;
    
    return self;
}

- (void)dealloc
{
    int i;
    
    // shutdown if we had previously been inited
    for (i=0; i<32; i++) {
        if (active & (1 << i)) {
            [self shutdownAs:(1 << i)];
        }
    }
    
    if (pluginRef) SysCloseLibrary(pluginRef);
    
    RELEASEOBJ(modDate);
    RELEASEOBJ(path);
    RELEASEOBJ(name);
    RELEASEOBJ(fullPlugPath);
    
    SUPERDEALLOC;
}

- (void)runCommand:(id)arg
{
    @autoreleasepool {
        NSString *funcName = [arg objectAtIndex:0];
        long (*func)(void);
        
        func = SysLoadSym(pluginRef, [funcName cStringUsingEncoding:NSASCIIStringEncoding]);
        if (SysLibError() == nil) {
            func();
        } else {
            NSBeep();
        }
        
        RELEASEOBJ(arg);
        return;
    }
}

- (long)runAs:(int)aType
{
    char symbol[255];
    long (*init)();
    long (*initArg)(long arg);
    int res = PSE_ERR_FATAL;
    
    if ((active & aType) == aType) {
        return 0;
    }

    sprintf(symbol, "%sinit", [[PcsxrPlugin prefixForType:aType] cStringUsingEncoding:NSASCIIStringEncoding]);
    init = initArg = SysLoadSym(pluginRef, symbol);
    if (SysLibError() == nil) {
        if (aType != PSE_LT_PAD) {
            res = init();
        } else {
            res = initArg(1|2);
        }
    }
    
    if (0 == res) {
        active |= aType;
    } else {
        NSRunCriticalAlertPanel(NSLocalizedString(@"Plugin Initialization Failed!", nil),
            [NSString stringWithFormat:NSLocalizedString(@"Pcsxr failed to initialize the selected %@ plugin (error=%i).\nThe plugin might not work with your system.", nil), [PcsxrPlugin prefixForType:aType], res], 
			nil, nil, nil);
    }
    
    return res;
}

- (long)shutdownAs:(int)aType
{
    char symbol[255];
    long (*shutdown)(void);

    sprintf(symbol, "%sshutdown", [[PcsxrPlugin prefixForType:aType] cStringUsingEncoding:NSASCIIStringEncoding]);
    shutdown = SysLoadSym(pluginRef, symbol);
    if (SysLibError() == nil) {
        active &= ~aType;
        return shutdown();
    }
    
    return PSE_ERR_FATAL;
}

- (BOOL)hasAboutAs:(int)aType
{
    char symbol[255];

    sprintf(symbol, "%sabout", [[PcsxrPlugin prefixForType:aType] cStringUsingEncoding:NSASCIIStringEncoding]);
    SysLoadSym(pluginRef, symbol);
    
    return (SysLibError() == nil);
}

- (BOOL)hasConfigureAs:(int)aType
{
    char symbol[255];

    sprintf(symbol, "%sconfigure", [[PcsxrPlugin prefixForType:aType] cStringUsingEncoding:NSASCIIStringEncoding]);
    SysLoadSym(pluginRef, symbol);
    
    return (SysLibError() == nil);
}

- (void)aboutAs:(int)aType
{
    NSArray *arg;
    char symbol[255];

    sprintf(symbol, "%sabout", [[PcsxrPlugin prefixForType:aType] cStringUsingEncoding:NSASCIIStringEncoding]);
    arg = [[NSArray alloc] initWithObjects:[NSString stringWithCString:symbol encoding:NSASCIIStringEncoding], 
                    [NSNumber numberWithInt:0], nil];
    
    // detach a new thread
    [NSThread detachNewThreadSelector:@selector(runCommand:) toTarget:self 
            withObject:arg];
    //NOTE: the runCommand releases the arg command. Probably not the best way to do it...
}

- (void)configureAs:(int)aType
{
    NSArray *arg;
    char symbol[255];
    
    sprintf(symbol, "%sconfigure", [[PcsxrPlugin prefixForType:aType] cStringUsingEncoding:NSASCIIStringEncoding]);
    arg = [[NSArray alloc] initWithObjects:[NSString stringWithCString:symbol encoding:NSASCIIStringEncoding], 
                    [NSNumber numberWithInt:1], nil];
    
    // detach a new thread
    [NSThread detachNewThreadSelector:@selector(runCommand:) toTarget:self 
            withObject:arg];
    //NOTE: the runCommand releases the arg command. Probably not the best way to do it...
}

- (NSString *)displayVersion
{
    if (version == -1)
        return @"";
    
	 return [NSString stringWithFormat:@"v%ld.%ld.%ld", version>>16,(version>>8)&0xff,version&0xff];
}

- (int)type
{
    return type;
}

- (NSString *)path
{
	return path;
}

- (NSUInteger)hash
{
    return [path hash];
}

- (NSString *)description
{
    if (name == nil)
        return [path lastPathComponent];
    
    return [NSString stringWithFormat:@"%@ %@ [%@]", name, [self displayVersion], [path lastPathComponent]];
}

// the plugin will check if it's still valid and return the status
- (BOOL)verifyOK
{
    // check that the file is still there with the same modification date
    NSFileManager *dfm = [NSFileManager defaultManager];
    if (![dfm fileExistsAtPath:fullPlugPath])
        return NO;
    
    NSDictionary *fattrs = [dfm attributesOfItemAtPath:[fullPlugPath stringByResolvingSymlinksInPath] error:NULL];
    return [[fattrs fileModificationDate] isEqualToDate:modDate];
}

@end
