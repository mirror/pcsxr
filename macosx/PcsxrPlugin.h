//
//  PcsxrPlugin.h
//  Pcsxr
//
//  Created by Gil Pedersen on Fri Oct 03 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface PcsxrPlugin : NSObject {
    void *pluginRef;
    
    NSString *path;
	NSDate *modDate;
    NSString *name;
	NSString *fullPlugPath;
    long version;
    int type;
	int active;
}

+ (NSString *)prefixForType:(int)type;
+ (NSString *)defaultKeyForType:(int)type;
+ (char **)configEntriesForType:(int)type;
+ (NSArray *)pluginsPaths;

- (id)initWithPath:(NSString *)aPath;

- (NSString *)displayVersion;
- (int)type;
- (NSString *)path;
- (NSUInteger)hash;
- (NSString *)description;
- (BOOL)hasAboutAs:(int)type;
- (BOOL)hasConfigureAs:(int)type;
- (long)runAs:(int)aType;
- (long)shutdownAs:(int)aType;
- (void)aboutAs:(int)type;
- (void)configureAs:(int)type;
- (BOOL)verifyOK;

@end
