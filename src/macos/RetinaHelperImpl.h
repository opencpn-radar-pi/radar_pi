// https://forums.wxwidgets.org/viewtopic.php?t=36684
//
// This is separate as it is Objective-C++
//
// file RetinaHelperImpl.h

#import "RetinaHelper.h"
#import <Cocoa/Cocoa.h>

// forward declaration
class wxEvtHandler;


@interface RetinaHelperImpl : NSObject
{
   NSView *view;
   wxEvtHandler* handler;
}

// designated initializer
-(id)initWithView:(NSView *)view handler:(wxEvtHandler *)handler;
-(void)setViewWantsBestResolutionOpenGLSurface:(BOOL)value;
-(BOOL)getViewWantsBestResolutionOpenGLSurface;
-(float)getBackingScaleFactor;
@end

