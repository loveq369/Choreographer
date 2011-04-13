//
//  ArrangerRulerView.h
//  Choreographer
//
//  Created by Philippe Kocher on 10.05.08.
//  Copyright 2010 Zurich University of the Arts. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "RulerView.h"

#define NUM_OF_LABELS 50

typedef enum _RulerMouseDraggingAction
{
	rulerDragNone = 0,
	rulerDragLoopRegionStart,
	rulerDragLoopRegionEnd,
	rulerDragMarker
} RulerMouseDraggingAction;


@interface ArrangerRulerView : RulerView
{
	IBOutlet id arrangerView;

	RulerMouseDraggingAction mouseDraggingAction;
}

- (void)update:(NSNotification *)notification;

- (void)mouseDownInLoopRegionArea:(NSPoint)localPoint;
- (void)mouseDownInMarkerArea:(NSPoint)localPoint;
- (void)mouseDownInPlayheadArea:(NSPoint)localPoint;

@end
