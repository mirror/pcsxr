//
//  PgxpController.m
//  Pcsxr
//
//  Created by MrLavender on 29/06/2017.
//

#import "PgxpController.h"
#include "psxcommon.h"

#define kPGXP_GTE     0
#define kPGXP_Cache   1
#define kPGXP_Texture 2

NSString* kPGXP_GTE_Key     = @"PGXP_GTE";
NSString* kPGXP_Cache_Key   = @"PGXP_Cache";
NSString* kPGXP_Texture_Key = @"PGXP_Texture";
NSString* kPGXP_Mode_Key    = @"PGXP_Mode";

NSString* infoText[] = {
	@"Disabled\n\nPGXP is no longer mirroring any functions.",
	@"Memory operations only\n\nPGXP is mirroring load, store and processor transfer operations of the CPU and GTE.",
	@"Memory and CPU arithmetic operations\n\nPGXP is mirroring load, store and transfer operations of the CPU and GTE and arithmetic/logic functions of the PSX CPU.\n\n(WARNING: This mode is currently unfinished and may cause incorrect behaviour in some games)"
};

@interface PgxpController ()
@end

@implementation PgxpController

- (void)awakeFromNib
{
	self.vertexCreation.state = Config.PGXP_GTE;
	self.vertexCaching.state = Config.PGXP_Cache;
	self.perspectiveCorrect.state = Config.PGXP_Texture;

	[self.pgxpModeButton selectItemAtIndex:Config.PGXP_Mode];
	[self setInfoTextForPgxpMode];
}

- (IBAction)onOptionChange:(NSButton*)sender
{
	switch (sender.tag) {
		case kPGXP_GTE:
			Config.PGXP_GTE = sender.state;
			break;
		case kPGXP_Cache:
			Config.PGXP_Cache = sender.state;
			break;
		case kPGXP_Texture:
			Config.PGXP_Texture = sender.state;
			break;
		default:
			break;
	}
	[PgxpController savePgxpSettings];
}

- (IBAction)onModeChange:(NSPopUpButton*)sender
{
	Config.PGXP_Mode = (u32)sender.indexOfSelectedItem;
	[self setInfoTextForPgxpMode];
	[PgxpController savePgxpSettings];
}

- (void)setInfoTextForPgxpMode
{
	self.pgxpModeLabel.stringValue = infoText[Config.PGXP_Mode];
}

+ (void)loadPgxpSettings
{
	NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];

	[userDefaults registerDefaults:@{
		kPGXP_GTE_Key: @YES,
		kPGXP_Cache_Key: @YES,
		kPGXP_Texture_Key: @YES,
		kPGXP_Mode_Key: @0
	}];

	Config.PGXP_GTE = [userDefaults boolForKey:kPGXP_GTE_Key];
	Config.PGXP_Cache = [userDefaults boolForKey:kPGXP_Cache_Key];
	Config.PGXP_Texture = [userDefaults boolForKey:kPGXP_Texture_Key];
	Config.PGXP_Mode = (u32)[userDefaults integerForKey:kPGXP_Mode_Key];
}

+ (void)savePgxpSettings
{
	NSUserDefaults* userDefaults = [NSUserDefaults standardUserDefaults];

	[userDefaults setBool:Config.PGXP_GTE forKey:kPGXP_GTE_Key];
	[userDefaults setBool:Config.PGXP_Cache forKey:kPGXP_Cache_Key];
	[userDefaults setBool:Config.PGXP_Texture forKey:kPGXP_Texture_Key];
	[userDefaults setInteger:Config.PGXP_Mode forKey:kPGXP_Mode_Key];
}

@end
