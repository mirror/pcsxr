//
//  MemBadgeView.m
//  Pcsxr
//
//  Created by C.W. Betts on 7/6/13.
//
//

#import "MemBadgeView.h"
#import "ARCBridge.h"

@implementation MemBadgeView

//TODO: also include the memory count in the view as well.
- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
	NSRect drawToRect = dirtyRect;
	NSImage *tmpDraw = nil;
	if (!NSEqualSizes(self.frame.size, dirtyRect.size)) {
		drawToRect = (NSRect) {NSZeroPoint, self.frame.size};
		tmpDraw = [[NSImage alloc] initWithSize:drawToRect.size];
		[tmpDraw lockFocus];
	}
	[[NSColor whiteColor] set];
	[[NSBezierPath bezierPathWithOvalInRect:drawToRect] fill];
	[[NSColor redColor] set];
	NSRect smallerRect = drawToRect;
	smallerRect.origin.x += 2;
	smallerRect.origin.y += 2;
	smallerRect.size.height -= 4;
	smallerRect.size.width -= 4;
	[[NSBezierPath bezierPathWithOvalInRect:smallerRect] fill];

	if (tmpDraw) {
		[tmpDraw unlockFocus];
		
		[tmpDraw drawInRect:dirtyRect fromRect:dirtyRect operation:NSCompositeSourceOver fraction:1.0];
		
		RELEASEOBJ(tmpDraw);
	}
}

@end
