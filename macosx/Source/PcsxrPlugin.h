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

- (instancetype)initWithPath:(NSString *)aPath NS_DESIGNATED_INITIALIZER;

@property (readonly, copy) NSString *displayVersion;
- (BOOL)hasAboutAs:(int)type;
- (BOOL)hasConfigureAs:(int)type;
- (long)runAs:(int)aType;
- (long)shutdownAs:(int)aType;
- (void)aboutAs:(int)type;
- (void)configureAs:(int)type;
@property (readonly) BOOL verifyOK;

@end
