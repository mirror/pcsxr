//
//  PcsxrCheatHandler.m
//  Pcsxr
//
//  Created by C.W. Betts on 8/1/13.
//
//

#import "PcsxrCheatHandler.h"
#import "CheatController.h"
#import "PcsxrController.h"
#include "psxcommon.h"
#include "cheat.h"

@implementation PcsxrCheatHandler

+ (NSArray *)supportedUTIs
{
	static NSArray *utisupport;
	if (utisupport == nil) {
		utisupport = @[@"com.codeplex.pcsxr.cheat"];
	}
	return utisupport;
}

- (BOOL)handleFile:(NSString *)theFile
{
	LoadCheats([theFile fileSystemRepresentation]);

	if ([(PcsxrController*)[NSApp delegate] cheatController]) {
		[[(PcsxrController*)[NSApp delegate] cheatController] refresh];
	}
	return YES;
}

@end
