//
//  Region.m
//  Choreographer
//
//  Created by Philippe Kocher on 28.08.09.
//  Copyright 2010 Zurich University of the Arts. All rights reserved.
//

#import "Region.h"
#import "AudioRegion.h"
#import "CHProjectDocument.h"
#import "Breakpoint.h"

@implementation Region

/*
 the superclass for all regions
 - audio region
 - group region
 
 does graphic stuff too
 is a lightweight view class (not a subclass of NSView)

 */
- (void)awakeFromInsert
{
	[self commonAwake];
	gainBreakpointArray = [[NSMutableArray alloc] init];
	[gainBreakpointView setValue:gainBreakpointArray forKey:@"breakpointArray"];

	frame = NSMakeRect(0, 0, 0, 0);
	contentOffset = 0;

	// set one gain breakpoint at 0 dB (= overall volume of this region)
	Breakpoint *bp;
	
	bp = [[[Breakpoint alloc] init] autorelease];
	[bp setValue:0];
	[bp setTime:0];
	[gainBreakpointArray addObject:bp];	
	
	[self archiveData];
}

- (void)awakeFromFetch
{
	[self commonAwake];
	[self unarchiveData];
	
	frame = NSMakeRect(0, 0, 0, 0);
	
//	[[self valueForKey:@"trajectoryItem"] addObserver:self
//										   forKeyPath:@"trajectoryData"
//											  options:0
//											  context:NULL];
	
	[self calculatePositionBreakpoints];
}

- (void)commonAwake
{
	displaysTrajectoryPlaceholder = NO;
	
	// initialize breakpoint view
	gainBreakpointView = [[BreakpointView alloc] init];
	[gainBreakpointView setValue:self forKey:@"owningRegion"];
	[gainBreakpointView setValue:[[NSColor whiteColor] colorWithAlphaComponent:0.25] forKey:@"backgroundColor"];
	[gainBreakpointView setValue:[NSColor blackColor] forKey:@"lineColor"];
	[gainBreakpointView setValue:[NSColor blackColor] forKey:@"handleColor"];
	[gainBreakpointView setUpdateCallbackObject:self selector:@selector(archiveData)];
	[gainBreakpointView setValue:@"time: %0.0f vol: %0.2f dB" forKey:@"toolTipString"];
	
	// register for notifications
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(setZoom:)
												 name:@"arrangerViewZoomFactorDidChange" object:nil];
}	


- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	
	[projectSettings release];	

	[gainBreakpointArray release];
	[BreakpointView release];

	[super dealloc];
}

#pragma mark -
#pragma mark copying
// -----------------------------------------------------------




#pragma mark -
#pragma mark drawing
// -----------------------------------------------------------

- (void)drawRect:(NSRect)rect
{
	// get stored settings
	if(!projectSettings)
	{
		id document = [[NSDocumentController sharedDocumentController] currentDocument];
		projectSettings = [[document valueForKey:@"projectSettings"] retain];
	}
	
	// color
	NSColor *backgroundColor;
	NSColor *frameColor;
	
	color = [self color];
	frameColor	= color;

	if([[self valueForKey:@"selected"] boolValue])
	{
		color = [NSColor	colorWithCalibratedHue:[color hueComponent]
							saturation:[color saturationComponent] * 0.75
							brightness: [color brightnessComponent] * 1.5
							alpha: 0.85];

	}
	else
	{
		color = [NSColor	colorWithCalibratedHue:[color hueComponent]
							saturation:[color saturationComponent] * 0.75
							brightness: [color brightnessComponent] * 0.75
							alpha: 0.7];
	}

	backgroundColor = color;
	
	// locked / unlocked
	if([[self valueForKey:@"locked"] boolValue])
	{
		frameColor = [NSColor blackColor];
	}
	
	// muted / unmuted
	if([[self valueForKey:@"muted"] boolValue])
	{
		backgroundColor = [color colorWithAlphaComponent:0.15];
	}
	
	

	// background
	if(![[self valueForKey:@"childRegions"] count])
	{
		[[NSGraphicsContext currentContext] setShouldAntialias:YES];
		[backgroundColor set];
		[[NSBezierPath bezierPathWithRoundedRect:frame xRadius:5 yRadius:5] fill];
	}
	
	// draw child regions
	NSEnumerator *enumerator = [[self valueForKey:@"childRegions"] objectEnumerator];
	Region *region;

	while (region = [enumerator nextObject])
	{
		[region drawRect:rect];
	}


	// draw trajectory
	if([self valueForKey:@"trajectoryItem"])// && ![[self valueForKey:@"trajectoryItem"] isFault] && !displaysTrajectoryPlaceholder)
	{
		// NSLog(@"region %x draws trajectory item %@", self, [self valueForKey:@"trajectoryItem"]);
		NSData *theData;
		NSColor *trajectoryRegionColor;
		
		if([self valueForKey:@"parentRegion"])
		{
			trajectoryRegionColor = [color colorWithAlphaComponent:1];
		}		
		else
		{
			theData = [[NSUserDefaults standardUserDefaults] dataForKey:@"trajectoryRegionColor"];
			trajectoryRegionColor = (NSColor *)[NSUnarchiver unarchiveObjectWithData:theData];
		}

		if([[self valueForKey:@"selected"] boolValue])
		{
			trajectoryRegionColor = [NSColor	colorWithCalibratedHue:[trajectoryRegionColor hueComponent]
										 saturation:[trajectoryRegionColor saturationComponent] * 0.75
										 brightness: [trajectoryRegionColor brightnessComponent] * 1.5
											  alpha: 0.85];
		}
		else
		{
			trajectoryRegionColor = [NSColor	colorWithCalibratedHue:[trajectoryRegionColor hueComponent]
										 saturation:[trajectoryRegionColor saturationComponent] * 0.75
										 brightness: [trajectoryRegionColor brightnessComponent] * 0.75
											  alpha: 0.7];
		}

		NSRect r = frame;
		r.size.height = frame.size.height > REGION_NAME_BLOCK_HEIGHT ? REGION_TRAJECTORY_BLOCK_HEIGHT : frame.size.height * 0.5;
		r.origin.y += frame.size.height > REGION_NAME_BLOCK_HEIGHT ? frame.size.height - REGION_TRAJECTORY_BLOCK_HEIGHT : frame.size.height * 0.5;
		
		r = NSInsetRect(r, 2.0, 1.0);

		[trajectoryRegionColor set];
		[[NSBezierPath bezierPathWithRoundedRect:r xRadius:2 yRadius:2] fill];

		r = NSInsetRect(r, 2.0, 1.0);
		
		if([self valueForKeyPath:@"trajectoryItem.node.name"])
		{
		   NSString *label = [NSString stringWithString:[self valueForKeyPath:@"trajectoryItem.node.name"]];
			NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
			[attrs setObject:[NSFont systemFontOfSize: 10] forKey:NSFontAttributeName];
			[attrs setObject:[NSColor blackColor] forKey:NSForegroundColorAttributeName];
			[label drawInRect:r withAttributes:attrs];
		}
	}
	
	// draw frame
	[frameColor set];
	[[NSBezierPath bezierPathWithRoundedRect:frame xRadius:5 yRadius:5] stroke];
}

- (void)drawGainEnvelope:(NSRect)rect
{
	NSRect r = frame;
	if(![self valueForKey:@"trajectoryItem"] && !displaysTrajectoryPlaceholder)
	{
		r.origin.y += REGION_NAME_BLOCK_HEIGHT + 2;
		r.size.height -= REGION_NAME_BLOCK_HEIGHT + 4;
	}
	else
	{
		r.origin.y += REGION_NAME_BLOCK_HEIGHT + 2;
		r.size.height -= REGION_NAME_BLOCK_HEIGHT + REGION_TRAJECTORY_BLOCK_HEIGHT + 4;
	}
	
	r.origin.x += 2;
	r.size.width -= 4;
	
	gainBreakpointView.xAxisValueKeypath = @"time";
	gainBreakpointView.yAxisValueKeypath = @"value";
	
	gainBreakpointView.xAxisMin = 0;
	gainBreakpointView.xAxisMax = [[self valueForKeyPath:@"duration"] intValue];
	
	gainBreakpointView.yAxisMin = -72;
	gainBreakpointView.yAxisMax = 18;
	
	[gainBreakpointView drawInRect:r];
}



- (NSColor *)color
{
	NSData *theData;

	if([self valueForKey:@"parentRegion"])
	{
		return [[self valueForKey:@"parentRegion"] valueForKey:@"color"];
	}		
	if([[self valueForKey:@"childRegions"] count])
	{
		theData = [[NSUserDefaults standardUserDefaults] dataForKey:@"groupRegionColor"];
		return (NSColor *)[NSUnarchiver unarchiveObjectWithData:theData];
	}
	else
	{
		theData = [[NSUserDefaults standardUserDefaults] dataForKey:@"audioRegionColor"];
		return (NSColor *)[NSUnarchiver unarchiveObjectWithData:theData];
	}
}

#pragma mark -
#pragma mark mouse
// -----------------------------------------------------------

- (void)mouseDown:(NSPoint)location
{
	[gainBreakpointView mouseDown:location];
}

- (NSPoint)proposedMouseDrag:(NSPoint)delta
{
	return [gainBreakpointView proposedMouseDrag:delta];
}

- (void)mouseDragged:(NSPoint)delta
{
	[gainBreakpointView mouseDragged:delta];
}

- (void)mouseUp:(NSEvent *)event
{
	[gainBreakpointView mouseUp:event];	
}



#pragma mark -
#pragma mark drag & crop
// -----------------------------------------------------------

- (void)moveByDeltaX:(float)deltaX deltaY:(float)deltaY
{
	frame.origin.x += deltaX;
	frame.origin.y += deltaY;
	
	// move child views too
    NSEnumerator *enumerator = [[self valueForKey:@"childRegions"] objectEnumerator];
	Region *region;
	
    while (region = [enumerator nextObject])
	{
		[region moveByDeltaX:deltaX deltaY:deltaY];
    }
}

- (void)cropByDeltaX1:(float)deltaX1 deltaX2:(float)deltaX2
{
	frame.origin.x += deltaX1;
	contentOffset += deltaX1;
	frame.size.width -= deltaX1 - deltaX2;
}


- (void)updateGainEnvelope
{
	NSLog(@"Region: updateGainEnvelope ---------------");
	
	NSMutableArray *tempArray = [gainBreakpointArray mutableCopy];
	
//	float lastValue = [(Breakpoint *)[gainBreakpointArray objectAtIndex:0] value];
	for(Breakpoint* bp in tempArray)
	{
		if(bp.time < contentOffset / zoomFactorX ||
		   bp.time > contentOffset / zoomFactorX + frame.size.width / zoomFactorX)
		{
			[gainBreakpointArray removeObject:bp];
		}
		else
		{
			bp.time -= contentOffset / zoomFactorX;
		}
	}
	
	// temp
	// make sure that the breakpoint array is not empty
	if(![gainBreakpointArray count])
	{
		Breakpoint *bp = [[[Breakpoint alloc] init] autorelease];
		[bp setValue:0];
		[bp setTime:0];
		[gainBreakpointArray addObject:bp];	
	}
	
	[self archiveData];
}

- (void)updateTimeInModel
{
	NSLog(@"Region: updateTimeInModel");
	// synchronize data with new position of the regionView after dragging
	
	[self setValue:[NSNumber numberWithLong:(frame.origin.x - ARRANGER_OFFSET) / zoomFactorX] forKey:@"startTime"];
	[self setValue:[NSNumber numberWithLong:frame.origin.y / zoomFactorY] forKey:@"yPosInArranger"];

	if([self isKindOfClass:[AudioRegion class]])
	{
		[self setValue:[NSNumber numberWithLong:frame.size.width / zoomFactorX] forKeyPath:@"audioItem.duration"];
		[self setValue:[NSNumber numberWithLong:contentOffset / zoomFactorX] forKeyPath:@"audioItem.offsetInFile"];
	}
	
	// synchronize children
    NSEnumerator *enumerator = [[self valueForKey:@"childRegions"] objectEnumerator];
	Region *region;
	
    while (region = [enumerator nextObject])
	{
		[region updateTimeInModel];
    }
	
	// call the undoable refresh view method
//	[self undoableRefreshView];
	
}

//- (void)undoableRefreshView
//{	
//	// undo	
//	NSManagedObjectContext *managedObjectContext = [[[NSDocumentController sharedDocumentController] currentDocument] managedObjectContext];
//	[[[managedObjectContext undoManager] prepareWithInvocationTarget:self] undoableRefreshView];
//	
//	[self recalcFrame];
//}

#pragma mark -
#pragma mark gain envelope
// -----------------------------------------------------------

- (void)setGainBreakpointArray:(NSArray *)array
{
	if(gainBreakpointArray != array)
	{
		[gainBreakpointArray release];
		gainBreakpointArray = [array retain];
		
		[gainBreakpointView setValue:gainBreakpointArray forKey:@"breakpointArray"];
	}
}

- (void)removeSelectedGainBreakpoints
{
	[gainBreakpointView removeSelectedBreakpoints];
	
	if([gainBreakpointArray count] == 0)
	{
		Breakpoint *bp;
		
		bp = [[[Breakpoint alloc] init] autorelease];
		[bp setValue:0];
		[bp setTime:0];
		[gainBreakpointArray addObject:bp];	
	}
}


#pragma mark -
#pragma mark abstract methods
// -----------------------------------------------------------
- (void)recalcFrame {}
- (void)recalcWaveform {}
- (float)offset { return contentOffset; }
- (void)removeFromView {}
- (void)calculatePositionBreakpoints {}


#pragma mark -
#pragma mark accessors
// -----------------------------------------------------------

- (void)setSelected:(BOOL)flag
{
	selected = flag;	
	if(!selected) [gainBreakpointView deselectAll];
}

- (NSRect)frame { return frame; }
- (void)setFrame:(NSRect)rect { frame = rect; }	

- (void)setSuperview:(ArrangerView *)view
{
	superview = view;

	// initialize zoom
	zoomFactorX = [[superview valueForKey:@"zoomFactorX"] floatValue];
	zoomFactorY = [[superview valueForKey:@"zoomFactorY"] floatValue];		
}


- (NSNumber *)duration { return nil; }


#pragma mark -
#pragma mark notifications
// -----------------------------------------------------------

- (void)setZoom:(NSNotification *)notification
{	
	zoomFactorX = [[notification object] zoomFactorX];
	zoomFactorY = [[notification object] zoomFactorY];		
	
	[self recalcFrame];
	[self recalcWaveform];
}


#pragma mark -
#pragma mark serialisation
// -----------------------------------------------------------

- (void)archiveData
{ 
	NSMutableData *data;
	NSKeyedArchiver *archiver;
	
	// archive volume data
	data = [NSMutableData data];
	archiver = [[NSKeyedArchiver alloc] initForWritingWithMutableData:data];
	[archiver encodeObject:gainBreakpointArray forKey:@"gainBpArray"];
	[archiver finishEncoding];
	
	[self setValue:data forKey:@"gainEnvelopeData"];
	[archiver release];
}

- (void)unarchiveData
{
	NSLog(@"Region: unarchiveData");

	NSMutableData *data;
	NSKeyedUnarchiver* unarchiver;
	
	// unarchive volume data
	[gainBreakpointArray release];
	data = [self valueForKey:@"gainEnvelopeData"];
	if(!data)
	{
		gainBreakpointArray = [[NSMutableArray alloc] init];
	}
	else
	{
		unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:data];
		
		gainBreakpointArray = [[unarchiver decodeObjectForKey:@"gainBpArray"] retain];
		[unarchiver finishDecoding];
		[unarchiver release];
	}

	[gainBreakpointView setValue:gainBreakpointArray forKey:@"breakpointArray"];
}	



@end



@implementation PlaceholderRegion

+ (PlaceholderRegion *)placeholderRegionWithFrame:(NSRect)rect
{
	PlaceholderRegion *region = [[[PlaceholderRegion alloc] init] autorelease];
	[region setFrame:rect];
	
	return region;
}

// drawing
- (void)draw
{
	[[NSColor whiteColor] set];
	[[NSBezierPath bezierPathWithRoundedRect:frame xRadius:5 yRadius:5] stroke];
}

// accessors
- (NSRect)frame { return frame; }
- (void)setFrame:(NSRect)rect { frame = rect; }	

@end

