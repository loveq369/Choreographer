//
//  AudioSettingsWindowController.h
//  Choreographer
//
//  Created by Philippe Kocher on 09.12.11.
//  Copyright 2011 Zurich University of the Arts. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AudioSettingsWindowController : NSWindowController
{
	NSArray *bufferSizes;
	NSInteger selectedBufferSizeIndex;
}


- (void)setSelectedBufferSizeIndex:(NSUInteger)index;


@end
