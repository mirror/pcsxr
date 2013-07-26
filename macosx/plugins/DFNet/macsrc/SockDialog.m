//
//  SockDialog.m
//  DFNet
//
//  Created by C.W. Betts on 2/18/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "SockDialog.h"
#include "dfnet.h"

#import "EmuThread.h"
#import "ARCBridge.h"

void SysMessage(const char *fmt, ...)
{
	va_list list;
	char msg[512];
	//char cmd[512];
	
	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);
	
	//sprintf(cmd, "message %s\n", msg);
	NSString *errString = @(msg);
	fprintf(stderr, "%s", msg);
	NSAlert *alert = [NSAlert alertWithMessageText:@"Error" defaultButton:@"Exit" alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", errString];
	[alert setAlertStyle:NSCriticalAlertStyle];
	//NSInteger result = NSRunAlertPanel(errString, nil, @"Okay", nil, nil);
	NSInteger result = [alert runModal];
	if (result == NSAlertDefaultReturn)
	{
		Class theEmuClass = NSClassFromString(@"EmuThread");
		if (theEmuClass) {
			[theEmuClass stop];
		} else {
			NSLog(@"Unable to stop emulation because the Objective-C class \"EmuThreaed\" was not found. Are you using a different emulator than PCSXR?");
		}
	}
}

static inline void RunOnMainThreadSync(dispatch_block_t block)
{
	if ([NSThread isMainThread]) {
		block();
	} else {
		dispatch_sync(dispatch_get_main_queue(), block);
	}
}

static SockDialog *globalSock = nil;

void sockCreateWaitDlg()
{
	RunOnMainThreadSync(^{
		if (globalSock == nil) {
			globalSock = [[SockDialog alloc] init];
		}
		NSWindow *tempWindow = [globalSock window];
		[tempWindow center];
		[globalSock showWindow:nil];
		[tempWindow makeKeyAndOrderFront:nil];
	});
}

void sockDlgUpdate()
{
	
}

long sockOpen()
{
	LoadConf();
	
	return 0;
}

void sockDestroyWaitDlg()
{
	RunOnMainThreadSync(^{
		if (globalSock != nil) {
			[globalSock close];
			
			RELEASEOBJ(globalSock);
			globalSock = nil;
		}
	});
}

@implementation SockDialog

- (IBAction)cancel:(id)sender {
	WaitCancel = 1;
}

- (id)init
{
	if ((self = [super initWithWindowNibName:@"SockDialog"])) {
		
	}
	return self;
}

#if !__has_feature(objc_arc)
-(void)dealloc
{
	
	[super dealloc];
}
#endif

@end
