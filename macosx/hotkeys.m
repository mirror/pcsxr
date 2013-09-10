//
//  hotkeys.m
//  Pcsxr
//
//  Created by Nicolas Pepin-Perreault on 12-12-12.
//
//

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import "hotkeys.h"
#import "EmuThread.h"
#include "plugins.h"
#include "ExtendedKeys.h"
#import "PcsxrController.h"

#define HK_MAX_STATE 10
static id monitor;
static id gpuMonitor;
static int currentState = 0;
static NSMutableDictionary *hotkeys = nil;
enum {
    HK_FAST_FORWARD,
    HK_SAVE_STATE,
    HK_LOAD_STATE,
    HK_NEXT_STATE,
    HK_PREV_STATE,
    HK_FRAME_LIMIT
};

void nextState() {
    currentState++;
    if(currentState == HK_MAX_STATE) {
        currentState = 0;
    }
}

void prevState() {
    currentState--;
    if(currentState < 0) {
        currentState = HK_MAX_STATE-1;
    }
}

BOOL handleHotkey(NSString* keyCode) {
    if([EmuThread active]) { // Don't catch hotkeys if there is no emulation
        NSNumber *ident = hotkeys[keyCode];

        if(ident != nil) {
            switch([ident intValue]) {
                case HK_FAST_FORWARD:
                    // We ignore FastForward requests if the emulation is paused
                    if(![EmuThread isPaused]) {
                        GPU_keypressed(GPU_FAST_FORWARD);
                    }
                    break;
                case HK_FRAME_LIMIT:
                    // Ignore FrameLimit requests if paused
                    if(![EmuThread isPaused]) {
                        GPU_keypressed(GPU_FRAME_LIMIT);
                    }
                    break;
                case HK_SAVE_STATE:
                    [PcsxrController saveState:currentState];
                    break;
                case HK_LOAD_STATE:
                    [PcsxrController loadState:currentState];
                    break;
                case HK_NEXT_STATE:
                    nextState();
                    GPU_displayText((char*)[[NSString stringWithFormat:@"State Slot: %d", currentState] UTF8String]);
                    break;
                case HK_PREV_STATE:
                    prevState();
                    GPU_displayText((char*)[[NSString stringWithFormat:@"State Slot: %d", currentState] UTF8String]);
                    break;
                default:
                    NSLog(@"Invalid hotkey identifier %i.", [ident intValue]);
            }
        
            return YES;
        }
    }
    
    return NO;
}

void setupHotkey(int hk, NSString *label, NSDictionary *binding) {
	if(binding != nil)
		hotkeys[binding[@"keyCode"]] = @(hk);
}

void setupHotkeys() {
    NSDictionary *bindings = [[NSUserDefaults standardUserDefaults] objectForKey:@"HotkeyBindings"];
    hotkeys = [[NSMutableDictionary alloc] initWithCapacity:[bindings count]];
    
    setupHotkey(HK_FAST_FORWARD, @"FastForward", bindings[@"FastForward"]);
    setupHotkey(HK_SAVE_STATE, @"SaveState", bindings[@"SaveState"]);
    setupHotkey(HK_LOAD_STATE, @"LoadState", bindings[@"LoadState"]);
    setupHotkey(HK_NEXT_STATE, @"NextState", bindings[@"NextState"]);
    setupHotkey(HK_PREV_STATE, @"PrevState", bindings[@"PrevState"]);
    setupHotkey(HK_FRAME_LIMIT, @"FrameLimit", bindings[@"FrameLimit"]);
    
    currentState = 0;
}

void attachHotkeys() {
    // Configurable hotkeys
    NSEvent* (^handler)(NSEvent*) = ^(NSEvent *event) {
        if(handleHotkey([NSString stringWithFormat:@"%d", [event keyCode]])) {
            return (NSEvent*)nil; // handled
        }
        
        // Not handled
        return event;
    };
    setupHotkeys();
    monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSKeyUpMask handler:handler];
    
    // GPU key presses
    NSEvent* (^gpuKeypress)(NSEvent*) = ^(NSEvent *event) {
        GPU_keypressed([event keyCode]);
        return (NSEvent*)nil;
    };
    gpuMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:(NSKeyUpMask | NSControlKeyMask) handler:gpuKeypress];
}

void detachHotkeys() {
	hotkeys = nil;
    [NSEvent removeMonitor:monitor];
    [NSEvent removeMonitor:gpuMonitor];
    monitor = nil;
    gpuMonitor = nil;
}
