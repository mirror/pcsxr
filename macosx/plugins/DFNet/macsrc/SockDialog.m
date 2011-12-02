//
//  SockDialog.m
//  DFNet
//
//  Created by C.W. Betts on 2/18/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "SockDialog.h"
#include "dfnet.h"

void SysMessage(const char *fmt, ...) {
	va_list list;
	char msg[512];
	//char cmd[512];
	
	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);
	
	//sprintf(cmd, "message %s\n", msg);
	NSString *errString = [NSString stringWithUTF8String:msg];
	fprintf(stderr, "%s", msg);
	NSAlert *alert = [NSAlert alertWithMessageText:nil defaultButton:@"Exit" alternateButton:nil otherButton:nil informativeTextWithFormat:errString];
	[alert setAlertStyle:NSCriticalAlertStyle];
	//NSInteger result = NSRunAlertPanel(errString, nil, @"Okay", nil, nil);
	NSInteger result = [alert runModal];
	if (result == NSAlertDefaultReturn)
	{
		//TODO: Handle closing the emulator, but not quitting the program.
	}
}


static SockDialog *globalSock = nil;

void sockCreateWaitDlg() {
	if (globalSock == nil) {
		globalSock = [[SockDialog alloc] init];
	}
	NSWindow *tempWindow = [globalSock window];
	[tempWindow center];
	[globalSock showWindow:nil];
	[tempWindow makeKeyAndOrderFront:nil];

}

void sockDlgUpdate() {
	
}

long sockOpen()
{
	LoadConf();
	
	return 0;
}

void sockDestroyWaitDlg() {
	if (globalSock != nil) {
		[globalSock close];
		[globalSock release];
		globalSock = nil;
	}
}

@implementation SockDialog
- (IBAction)cancel:(id)sender {
	WaitCancel = 1;

}

- (id)init {
	if ((self = [super initWithWindowNibName:@"SockDialog"])) {
		return self;
	} else {
		[self autorelease];
		return nil;
	}
}

-(void)dealloc {
	
	[super dealloc];
}

@end
