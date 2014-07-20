//
//  PcsxrPlugin.h
//  Pcsxr
//
//  Created by Gil Pedersen on Fri Oct 03 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface PcsxrPlugin : NSObject
@property (readonly, copy) NSString *path;
@property (readonly, strong) NSString *name;
@property (readonly) int type;

+ (NSString *)prefixForType:(int)type;
+ (NSString *)defaultKeyForType:(int)type;
+ (char **)configEntriesForType:(int)type;
+ (NSArray *)pluginsPaths;

- (id)initWithPath:(NSString *)aPath;

- (NSString *)displayVersion;
- (BOOL)hasAboutAs:(int)type;
- (BOOL)hasConfigureAs:(int)type;
- (long)runAs:(int)aType;
- (long)shutdownAs:(int)aType;
- (void)aboutAs:(int)type;
- (void)configureAs:(int)type;
- (BOOL)verifyOK;

@end
