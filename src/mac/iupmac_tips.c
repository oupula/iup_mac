/** \file
 * \brief MAC Driver TIPS management
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>

#include <Cocoa/Cocoa.h>

#include "iup.h" 

#include "iup_object.h" 
#include "iup_str.h" 
#include "iup_attrib.h" 
#include "iup_image.h" 

#include "iupmac_drv.h"

@interface TooltipWindow : NSWindow
{
    NSTimer *closeTimer;
    id tooltipObject;
}
+ (id)tipWithString:(NSString *)tip frame:(NSRect)frame display:(BOOL)display;
+ (id)tipWithAttributedString:(NSAttributedString *)tip frame:(NSRect)frame display:(BOOL)display;

// returns the approximate window size needed to display the tooltip string.
+ (NSSize)suggestedSizeForTooltip:(id)tooltip;

// setting and getting the default duration..
+ (void)setDefaultDuration:(NSTimeInterval)inSeconds;
+ (NSTimeInterval)defaultDuration;

// setting and getting the default bgColor
+ (void)setDefaultBackgroundColor:(NSColor *)bgColor;
+ (NSColor *)defaultBackgroundColor;

- (id)init;

- (id)tooltip;
- (void)setTooltip:(id)tip;

- (void)orderFrontWithDuration:(NSTimeInterval)seconds;

@end

float defaultDuration;
BOOL doneInitialSetup;
NSDictionary *textAttributes;
NSColor *backgroundColor;

@implementation TooltipWindow

+ (void)setDefaultBackgroundColor:(NSColor *)bgColor
{
    [backgroundColor release];
    backgroundColor = [bgColor retain];
}
+ (NSColor *)defaultBackgroundColor
{
    if (!backgroundColor)
        [TooltipWindow setDefaultBackgroundColor: [NSColor colorWithDeviceRed:1.0 green:0.96 blue:0.76 alpha:1.0]];

    return backgroundColor;
}


+ (void)setDefaultDuration:(NSTimeInterval)inSeconds
{
    doneInitialSetup = YES;
    defaultDuration = inSeconds;
}

+ (NSTimeInterval)defaultDuration
{
    return defaultDuration;
}


+ (id)tipWithString:(NSString *)tip frame:(NSRect)frame display:(BOOL)display
{
    return [TooltipWindow tipWithAttributedString:[[[NSAttributedString alloc] initWithString:tip] autorelease] frame:frame display:display];
}

+ (id)tipWithAttributedString:(NSAttributedString *)tip frame:(NSRect)frame display:(BOOL)display
{
    TooltipWindow *window = [[TooltipWindow alloc] init]; // blank slate

    // if the defaultDuration hasn't been set we set it to a default of 5 seconds    // we like to cache the text attributes used for plain string drawing..
    if (!doneInitialSetup) {
            [TooltipWindow setDefaultDuration:5];
            [window setTooltip:@" "]; // Just having at least 1 char to allow the next message...
            textAttributes = [[[[window contentView] attributedStringValue] attributesAtIndex:0 effectiveRange:nil] retain
];
    }

            [window setTooltip:tip]; // set the tip            [window setReleasedWhenClosed:display]; // if we display right away we release on close
            [window setFrame:frame display:YES];

        if (display) {
            [window orderFrontWithDuration:[TooltipWindow defaultDuration]]; // this is affectively autoreleasing the window after 'defaultDuration'
            return window;
       } else // it's not set to auto-magically go away so make the caller responsible..
            return [window autorelease];

    return nil; // should never get here
}

+ (NSSize)suggestedSizeForTooltip:(id)tooltip
{
    NSSize tipSize = NSZeroSize;

    if ([tooltip isKindOfClass:[NSAttributedString class]]) tipSize = [tooltip size];
    else
    if ([tooltip isKindOfClass:[NSString class]]) tipSize = [tooltip sizeWithAttributes:textAttributes];
    else
        return tipSize; // we don't know how to get the size of 'tooltip' so return NSZeroSize

        if (!NSEqualSizes(tipSize, NSZeroSize))
            tipSize.width += 4;

return tipSize;

}


- (id)init
{
    self = [super initWithContentRect:NSMakeRect(0,0,0,0)
                              styleMask:NSBorderlessWindowMask
                                backing:NSBackingStoreBuffered
                                  defer:NO];
     { // window setup...
        [self setAlphaValue:0.90];
        [self setOpaque:NO];
        [self setBackgroundColor:[TooltipWindow defaultBackgroundColor]];
        [self setHasShadow:YES];
        [self setLevel:NSStatusWindowLevel];
        [self setHidesOnDeactivate:YES];
        [self setIgnoresMouseEvents:YES];
     }

     { // textfield setup...
        NSTextField *field = [[NSTextField alloc] initWithFrame:NSMakeRect(0,0,0,0)];

            [field setEditable:NO];
            [field setSelectable:NO];
            [field setBezeled:NO];
            [field setBordered:NO];
            [field setDrawsBackground:NO];
            [field setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
            [self setContentView:field];
            [self setFrame:[self frameRectForContentRect:[field frame]] display:NO];

                if (!doneInitialSetup) {
                    [TooltipWindow setDefaultDuration:5];
                    [field setStringValue:@" "]; // Just having at least 1 char to allow the next message...
                    textAttributes = [[[field attributedStringValue] attributesAtIndex:0 effectiveRange:nil] retain];
                }

            [field release];
        }

    return self;
}


- (void)dealloc
{
        if (closeTimer) {
            [closeTimer invalidate];
            [closeTimer release];
        }
    [tooltipObject release];
    [super dealloc];
}

- (id)tooltip { return tooltipObject; }

- (void)setTooltip:(id)tip
{
    id contentView = [self contentView];

    [tooltipObject release];
    tooltipObject = [tip retain];

    if ([contentView isKindOfClass:[NSTextField class]]) {
        if ([tip isKindOfClass:[NSString class]]) [contentView setStringValue:tip];
        else
        if ([tip isKindOfClass:[NSAttributedString class]]) [contentView setAttributedStringValue:tip];
    }
}

- (void)orderFrontWithDuration:(NSTimeInterval)seconds
{
    [super orderFront:nil];

    if (closeTimer) { [closeTimer invalidate]; [closeTimer release]; }
    closeTimer = [[NSTimer scheduledTimerWithTimeInterval:seconds target:self selector:@selector(close) userInfo:nil repeats:NO] retain];
}


- (NSString *)description
{
    return [NSString stringWithFormat:@"TooltipWindow:\n%@", [[self contentView] stringValue]];
}

@end

int iupdrvBaseSetTipAttrib(Ihandle* ih, const char* value)
{
  id tmpTip = (id)iupAttribGet(ih, "_IUPMAC_TIPSWIN");
  if (!tmpTip)
  {
    NSRect tipFrame;
    id theTipString = [NSString stringWithUTF8String:value];
    BOOL animate = NO;

    tipFrame.origin = [NSEvent mouseLocation];
    tipFrame.size = [TooltipWindow suggestedSizeForTooltip:theTipString];
    [TooltipWindow setDefaultDuration:5];

    tmpTip = [TooltipWindow tipWithAttributedString:theTipString frame:tipFrame display:NO];
    iupAttribSetStr(ih, "_IUPMAC_TIPSWIN", (char*)tmpTip);
  }

  if (value==NULL)
    [tmpTip release];

  return 1;
}

int iupdrvBaseSetTipVisibleAttrib(Ihandle* ih, const char* value)
{
  id tmpTip = (id)iupAttribGet(ih, "_IUPMAC_TIPSWIN");
  if (!tmpTip)
    return 0;

  /* must use IupGetAttribute to use inheritance */
  if (!IupGetAttribute(ih, "TIP"))
    return 0;

  NSRect tipFrame = [tmpTip frame];
  if (iupStrBoolean(value)) {
    unsigned char r, g, b;
    if (iupStrToRGB(IupGetAttribute(ih, "TIPBGCOLOR"), &r, &g, &b))
    {
      [tmpTip setDefaultBackgroundColor:[NSColor colorWithDeviceRed:(r/255.0) green:(g/255.0) blue:(g/255.0) alpha:1.0]];
    }
    tipFrame.origin = [NSEvent mouseLocation];
    [tmpTip frame:tipFrame display:YES];
  } else
    [tmpTip frame:tipFrame display:NO];

  return 0;
}

