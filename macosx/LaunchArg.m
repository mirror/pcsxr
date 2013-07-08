//
//  LaunchArg.m
//  Pcsxr
//
//  Created by C.W. Betts on 7/8/13.
//
//

#import "LaunchArg.h"

@interface LaunchArg ()
@property (readwrite) LaunchArgOrder launchOrder;
@property (readwrite, copy, nonatomic) dispatch_block_t theBlock;
@property (readwrite, arcretain) NSString *argument;

@end

@implementation LaunchArg
@synthesize argument = _argument;
@synthesize launchOrder = _launchOrder;
@synthesize theBlock = _theBlock;
- (void)setTheBlock:(dispatch_block_t)theBlock
{
#if __has_feature(objc_arc)
	_theBlock = [theBlock copy];
#else
	if (_theBlock == theBlock) {
		return;
	}
	dispatch_block_t tmpBlock = _theBlock
	_theBlock = [theBlock copy];
	[tmpBlock release];
#endif
}

- (id)initWithLaunchOrder:(LaunchArgOrder)order block:(dispatch_block_t)block argument:(NSString*)arg
{
	if (self = [super init]) {
		self.launchOrder = order;
		self.theBlock = block;
		self.argument = arg;
	}
	return self;
}

- (void)addToDictionary:(NSMutableDictionary*)toAdd
{
	[toAdd setObject:self forKey:self.argument];
}

- (NSString*)description
{
	return [NSString stringWithFormat:@"Arg: %@, order: %u, block addr: %p", _argument, _launchOrder, _theBlock];
}

#if !__has_feature(objc_arc)
- (void)dealloc
{
	self.theBlock = nil;
	self.argument = nil;
	
	[super dealloc];
}
#endif

@end
