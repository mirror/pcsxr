//
//  LaunchArg.h
//  Pcsxr
//
//  Created by C.W. Betts on 7/8/13.
//
//

#import <Foundation/Foundation.h>
#import "ARCBridge.h"

typedef enum _LaunchArgOrder {
	LaunchArgPreRun = 0,
	LaunchArgRun = 200,
	LaunchArgPostRun = 400
}LaunchArgOrder;

@interface LaunchArg : NSObject
{
	LaunchArgOrder _launchOrder;
	dispatch_block_t _theBlock;
	NSString *_argument;
}
@property (readonly) LaunchArgOrder launchOrder;
@property (readonly, copy, nonatomic) dispatch_block_t theBlock;
@property (readonly, arcretain) NSString *argument;

- (id)initWithLaunchOrder:(LaunchArgOrder)order block:(dispatch_block_t)block argument:(NSString*)arg;
- (id)initWithLaunchOrder:(LaunchArgOrder)order argument:(NSString*)arg block:(dispatch_block_t)block;
- (void)addToDictionary:(NSMutableDictionary*)toAdd;
@end
