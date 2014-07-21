//
//  PcsxrHexadecimalFormatter.m
//  Pcsxr
//
//  Created by C.W. Betts on 8/17/13.
//
//

#import "PcsxrHexadecimalFormatter.h"

@interface PcsxrHexadecimalFormatter ()
@property (strong) NSString *hexFormatString;
@end

@implementation PcsxrHexadecimalFormatter
@synthesize hexPadding;
@synthesize hexFormatString;

- (void)setHexPadding:(char)_hexPadding
{
	hexPadding = _hexPadding;
	self.hexFormatString = [NSString stringWithFormat:@"0x%%0%ilx", hexPadding];
}

- (instancetype)init
{
	if (self = [super init]) {
#ifdef __LP64__
		self.hexPadding = 16;
#else
		self.hexPadding = 8;
#endif
	}
	return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
	if (self = [super initWithCoder:aDecoder]) {
#ifdef __LP64__
		self.hexPadding = 16;
#else
		self.hexPadding = 8;
#endif
	}
	return self;
}

- (NSString *)stringForObjectValue:(id)obj
{
	if ([obj isKindOfClass:[NSNumber class]]) {
		return [NSString stringWithFormat:self.hexFormatString, (long)[obj integerValue]];
	} else return nil;
}

- (NSString *)editingStringForObjectValue:(id)obj
{
	if ([obj isKindOfClass:[NSNumber class]]) {
		return [NSString stringWithFormat:@"%lx", (long)[obj integerValue]];
	} else return nil;
}

- (BOOL)getObjectValue:(out id *)obj forString:(NSString *)string errorDescription:(out NSString **)error
{
	NSString *tmpstr = nil;
	unsigned int tmpNum;
	NSScanner *theScan = [[NSScanner alloc] initWithString:string];
	if ([theScan scanHexInt:&tmpNum]) {
		*obj = @(tmpNum);
		return YES;
	} else {
		if ([string hasPrefix:@"0x"]) {
			NSRange zeroXRange = [string rangeOfString:@"0x"];
			tmpstr = [string stringByReplacingCharactersInRange:zeroXRange withString:@""];
		}else {
			tmpstr = string;
		}
		long tmpNum = 0;
		if (sscanf([tmpstr UTF8String], "%lx", &tmpNum) == 1) {
			*obj = @(tmpNum);
			return YES;
		} else {
			return NO;
		}
	}
}

@end
