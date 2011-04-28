//
//  RotationTrajectory.h
//  Choreographer
//
//  Created by Philippe Kocher on 16.06.10.
//  Copyright 2011 Zurich University of the Arts. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Trajectory.h"


@interface RotationTrajectory : Trajectory
{
	SpatialPosition	*rotationCentre;
	float			speed;
}

// accessors
- (void)setSpeed:(float)val;

//- (void)initialPosition:(SpatialPosition *)pos;
//- (void)rotationCentre:(SpatialPosition *)pos;

@end
