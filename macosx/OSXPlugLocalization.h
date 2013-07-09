//
//  OSXPlugLocalization.h
//  Pcsxr
//
//  Created by C.W. Betts on 7/8/13.
//
//

#ifndef Pcsxr_OSXPlugLocalization_h
#define Pcsxr_OSXPlugLocalization_h

#define PLUGLOCIMP(klass) \
char* PLUGLOC(char *toloc) \
{ \
NSBundle *mainBundle = [NSBundle bundleForClass:klass]; \
NSString *origString = @(toloc), *transString = nil; \
transString = [mainBundle localizedStringForKey:origString value:@"" table:nil]; \
return (char*)[transString UTF8String]; \
}

#endif
