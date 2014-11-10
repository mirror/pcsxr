//
//  LaunchArg.h
//  Pcsxr
//
//  Created by C.W. Betts on 7/8/13.
//
//

#import <Foundation/Foundation.h>

typedef enum _LaunchArgOrder {
	LaunchArgPreRun = 0,
	LaunchArgRun = 200,
	LaunchArgPostRun = 400
}LaunchArgOrder;

@interface LaunchArg : NSObject
@property (readonly) unsigned launchOrder;
@property (readonly, copy, nonatomic) dispatch_block_t theBlock;
@property (readonly, copy) NSString *argument;

- (instancetype)initWithLaunchOrder:(unsigned)order block:(dispatch_block_t)block argument:(NSString*)arg;
- (instancetype)initWithLaunchOrder:(unsigned)order argument:(NSString*)arg block:(dispatch_block_t)block NS_DESIGNATED_INITIALIZER;
- (void)addToDictionary:(NSMutableDictionary*)toAdd;
@end
