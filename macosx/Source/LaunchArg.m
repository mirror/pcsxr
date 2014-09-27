//
//  LaunchArg.m
//  Pcsxr
//
//  Created by C.W. Betts on 7/8/13.
//
//

#import "LaunchArg.h"

@interface LaunchArg ()
@property (readwrite) unsigned launchOrder;
@property (readwrite, copy, nonatomic) dispatch_block_t theBlock;
@property (readwrite, strong) NSString *argument;
@end

@implementation LaunchArg
@synthesize argument = _argument;
@synthesize launchOrder = _launchOrder;
@synthesize theBlock = _theBlock;
- (void)setTheBlock:(dispatch_block_t)theBlock
{
	_theBlock = [theBlock copy];
}

- (instancetype)initWithLaunchOrder:(unsigned)order argument:(NSString*)arg block:(dispatch_block_t)block
{
	if (self = [super init]) {
		self.launchOrder = order;
		self.theBlock = block;
		self.argument = arg;
	}
	return self;
}

- (instancetype)initWithLaunchOrder:(unsigned)order block:(dispatch_block_t)block argument:(NSString*)arg
{
	return [self initWithLaunchOrder:order argument:arg block:block];
}

- (void)addToDictionary:(NSMutableDictionary*)toAdd
{
	toAdd[self.argument] = self;
}

- (NSString*)description
{
	return [NSString stringWithFormat:@"Arg: %@, order: %u, block addr: %p", _argument, _launchOrder, _theBlock];
}

@end
