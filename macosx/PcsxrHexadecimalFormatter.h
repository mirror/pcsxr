//
//  PcsxrHexadecimalFormatter.h
//  Pcsxr
//
//  Created by C.W. Betts on 8/17/13.
//
//

#import <Foundation/Foundation.h>

@interface PcsxrHexadecimalFormatter : NSFormatter
{
	char hexPadding;
	NSString *hexFormatString;
}
@property (nonatomic) char hexPadding;

@end
