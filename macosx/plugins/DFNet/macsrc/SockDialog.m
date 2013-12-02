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

void SysMessage(const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	NSString *errString = [[NSString alloc] initWithFormat:@(fmt) arguments:list];
	va_end(list);
	
	fprintf(stderr, "message %s\n", [errString UTF8String]);
	NSAlert *alert = [NSAlert alertWithMessageText:@"Error" defaultButton:@"Stop" alternateButton:nil otherButton:nil informativeTextWithFormat:@"%@", errString];
	[alert setAlertStyle:NSCriticalAlertStyle];
	NSInteger result = [alert runModal];
	if (result == NSAlertDefaultReturn)
	{
		Class theEmuClass = NSClassFromString(@"EmuThread");
		if (theEmuClass) {
			[theEmuClass stop];
		} else {
			NSLog(@"Unable to stop emulation because the Objective-C class \"EmuThreaed\" was not found.");
			NSLog(@"Are you using a different emulator than PCSXR?");
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
	return self = [self initWithWindowNibName:@"SockDialog"];
}

@end
