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

#define kPCSXRGetLibName "PSEgetLibName"
#define kPCSXRGetLibVersion "PSEgetLibVersion"
#define kPCSXRGetLibType "PSEgetLibType"

@interface PcsxrPlugin ()
@property (readwrite, copy) NSString *path;
@property (readwrite, arcstrong) NSDate *modDate;
@property (readwrite, arcstrong) NSString *name;
@property (readwrite, arcstrong) NSString *fullPlugPath;
@end

@implementation PcsxrPlugin

@synthesize path;
@synthesize name;
@synthesize fullPlugPath;
@synthesize modDate;

+ (NSString *)prefixForType:(int)aType
{
	switch (aType) {
		case PSE_LT_GPU: return @"GPU"; break;
		case PSE_LT_CDR: return @"CDR"; break;
		case PSE_LT_SPU: return @"SPU"; break;
		case PSE_LT_PAD: return @"PAD"; break;
		case PSE_LT_NET: return @"NET"; break;
		case PSE_LT_SIO1: return @"SIO1"; break;
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
		case PSE_LT_SIO1:
			return [NSString stringWithFormat:@"Plugin%@", [self prefixForType:aType]];
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
	static char *sio1[2] = {(char *)&Config.Sio1, NULL};
	
	switch (aType) {
		case PSE_LT_GPU: return (char **)gpu;
		case PSE_LT_CDR: return (char **)cdr;
		case PSE_LT_SPU: return (char **)spu;
		case PSE_LT_PAD: return (char **)pad;
		case PSE_LT_NET: return (char **)net;
		case PSE_LT_SIO1: return (char **)sio1;
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
	
	NSFileManager *fm = [NSFileManager defaultManager];
	
	pluginRef = NULL;
	name = nil;
	self.path = aPath;
	NSString *goodPath = nil;
	if ([aPath isAbsolutePath]) {
		goodPath = aPath;
	} else {
		long tempVers = 0;
		for (NSString *plugDir in [PcsxrPlugin pluginsPaths])
		{
			NSString *fullPath = [plugDir stringByAppendingPathComponent:path];
			if ([fm fileExistsAtPath:fullPath]) {
				void *tempHandle = SysLoadLibrary([fullPath fileSystemRepresentation]);
				if (tempHandle != NULL)
				{
					PSEgetLibVersion tempLibVersion = SysLoadSym(tempHandle, kPCSXRGetLibVersion);
					if (SysLibError() == NULL)
					{
						long tempVers2 = tempLibVersion();
						if (tempVers < tempVers2 ){
							goodPath = fullPath;
							tempVers = tempVers2;
							if (![plugDir isEqualToString:[fm stringWithFileSystemRepresentation:Config.PluginsDir length:strlen(Config.PluginsDir)]]) {
								self.path = goodPath;
							}
						}
					}
					SysCloseLibrary(tempHandle);
				} else {
					SysLibError();
				}
			}
		}
	}
	
	if (goodPath == nil) {
		AUTORELEASEOBJNORETURN(self);
		return nil;
	}
	
	pluginRef = SysLoadLibrary([goodPath fileSystemRepresentation]);
	if (pluginRef == NULL) {
		SysLibError();
		AUTORELEASEOBJNORETURN(self);
		return nil;
	}
	
	// TODO: add support for plugins with multiple functionalities???
	PSE_getLibType = (PSEgetLibType) SysLoadSym(pluginRef, kPCSXRGetLibType);
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
		else if (([path rangeOfString: @"sio1" options:NSCaseInsensitiveSearch]).length != 0)
			type = PSE_LT_SIO1;
		else {
			AUTORELEASEOBJNORETURN(self);
			return nil;
		}
	} else {
		type = (int)PSE_getLibType();
		if (type != PSE_LT_GPU && type != PSE_LT_CDR && type != PSE_LT_SPU && type != PSE_LT_PAD && type != PSE_LT_NET && type != PSE_LT_SIO1) {
			AUTORELEASEOBJNORETURN(self);
			return nil;
		}
	}
	
	PSE_getLibName = (PSEgetLibName) SysLoadSym(pluginRef, kPCSXRGetLibName);
	if (SysLibError() == nil) {
		self.name = @(PSE_getLibName());
	}
	
	PSE_getLibVersion = (PSEgetLibVersion) SysLoadSym(pluginRef, kPCSXRGetLibVersion);
	if (SysLibError() == nil) {
		version = PSE_getLibVersion();
	} else {
		version = -1;
	}
	
	// save the current modification date
	NSDictionary *fattrs = [fm attributesOfItemAtPath:[goodPath stringByResolvingSymlinksInPath] error:NULL];
	self.modDate = [fattrs fileModificationDate];
	self.fullPlugPath = goodPath;
	
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
	
#if !__has_feature(objc_arc)
	self.modDate = nil;
	self.path = nil;
	self.name = nil;
	self.fullPlugPath = nil;
	
	[super dealloc];
#endif
}

#define PluginSymbolName(type, theName) [[PcsxrPlugin prefixForType:type] stringByAppendingString:theName]

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
	long (*init)();
	long (*initArg)(long arg);
	long res = PSE_ERR_FATAL;
	
	if ((active & aType) == aType) {
		return 0;
	}
	
	init = initArg = SysLoadSym(pluginRef, [PluginSymbolName(aType, @"init")
											cStringUsingEncoding:NSASCIIStringEncoding]);
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
								NSLocalizedString(@"Pcsxr failed to initialize the selected %@ plugin (error=%i).\nThe plugin might not work with your system.", nil),
								nil, nil, nil, [PcsxrPlugin prefixForType:aType], res);
	}
	
	return res;
}

- (long)shutdownAs:(int)aType
{
	long (*shutdown)(void);
	
	shutdown = SysLoadSym(pluginRef, [PluginSymbolName(aType, @"shutdown")
									  cStringUsingEncoding:NSASCIIStringEncoding]);
	if (SysLibError() == nil) {
		active &= ~aType;
		return shutdown();
	}
	
	return PSE_ERR_FATAL;
}

#define PluginSymbolNameConfigure(type) PluginSymbolName(type, @"configure")
#define PluginSymbolNameAbout(type) PluginSymbolName(type, @"about")

- (BOOL)hasAboutAs:(int)aType
{
	SysLoadSym(pluginRef, [PluginSymbolNameAbout(aType)
						   cStringUsingEncoding:NSASCIIStringEncoding]);
	
	return (SysLibError() == nil);
}

- (BOOL)hasConfigureAs:(int)aType
{
	SysLoadSym(pluginRef, [PluginSymbolNameConfigure(aType)
						   cStringUsingEncoding:NSASCIIStringEncoding]);
	
	return (SysLibError() == nil);
}

- (void)aboutAs:(int)aType
{
	NSArray *arg;
	
	NSString *aboutSym = PluginSymbolNameAbout(aType);
	arg = @[aboutSym, @0];
	RETAINOBJNORETURN(arg);
	
	// detach a new thread
	[NSThread detachNewThreadSelector:@selector(runCommand:) toTarget:self
						   withObject:arg];
	//NOTE: the runCommand releases the arg command. Probably not the best way to do it...
	//This is not an issue with ARC (64-bit) code
}

- (void)configureAs:(int)aType
{
	NSArray *arg;
	
	NSString *configSym = PluginSymbolNameConfigure(aType);
	arg = @[configSym, @1];
	RETAINOBJNORETURN(arg);
	
	// detach a new thread
	[NSThread detachNewThreadSelector:@selector(runCommand:) toTarget:self
						   withObject:arg];
	//NOTE: the runCommand releases the arg command. Probably not the best way to do it...
	//This is not an issue with ARC (64-bit) code
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

- (NSString*)debugDescription
{
	if (name == nil) {
		return fullPlugPath;
	}
	return [NSString stringWithFormat:@"%@, %@ [%@]", name, [self displayVersion], fullPlugPath];
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
