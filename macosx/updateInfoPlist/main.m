//
//  main.m
//  updateInfoPlist
//
//  Created by C.W. Betts on 10/23/13.
//  Code based on the RubyCocoa script used by the MPlayerX team
//  Original code can be found at http://blog.mplayerx.org/blog/2013/05/10/use-version-number-with-xcode-and-git/
//
//

#import <Foundation/Foundation.h>

int main(int argc, const char * argv[])
{
	@autoreleasepool {
		if (argc != 2) {
			NSLog(@"Usage: %s \"path to plist\"", argv[0]);
			abort();
			return EXIT_FAILURE;
		}
		NSString *plistLocation =[[NSFileManager defaultManager] stringWithFileSystemRepresentation:argv[1] length:strlen(argv[1])];
	    NSMutableDictionary *plistDict = [[NSMutableDictionary alloc] initWithContentsOfFile:plistLocation];
		if (!plistDict) {
			return EXIT_FAILURE;
		}
		NSTask *versionTask = [[NSTask alloc] init];
		[versionTask setLaunchPath:@"/bin/bash"];
		versionTask.arguments = @[@"version.sh"];
		NSPipe *soPipe = [NSPipe new];
		versionTask.standardOutput = soPipe;
		
		[versionTask launch];
		[versionTask waitUntilExit];
		
		NSData *outData = [soPipe.fileHandleForReading readDataToEndOfFile];
		versionTask = nil;
		NSString *outString = [[NSString alloc] initWithData:outData encoding:NSUTF8StringEncoding];
		outString = [outString stringByTrimmingCharactersInSet:[NSCharacterSet newlineCharacterSet]];
		if ([outString isEqualToString:@"unknown"]) {
			NSLog(@"Version is invalid!");
			return EXIT_FAILURE;
		}
		plistDict[@"CFBundleVersion"] = outString;

		[plistDict writeToFile:[NSString stringWithFormat:@"%@vers.plist", plistLocation] atomically:NO];
	}
    return 0;
}
