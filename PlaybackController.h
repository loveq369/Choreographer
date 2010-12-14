//
//  PlaybackController.h
//  Choreographer
//
//  Created by Philippe Kocher on 28.03.10.
//  Copyright 2010 Zurich University of the Arts. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ProjectSettings.h"
#import "AudioEngine.h"
#import "RulerPlayhead.h"


@interface PlaybackController : NSObject
{
    ProjectSettings *projectSettings;

	NSMutableSet *updateRegions;
	
	BOOL loopMode;
	BOOL isPlaying;

	unsigned long locator;
	unsigned long startLocator;

	NSTimer		 *playbackTimer;
	
	IBOutlet id counter;
	IBOutlet RulerPlayhead *rulerPlayhead;
	IBOutlet id playhead;
	
	float masterVolume;
}

// accessors
- (void)setProjectSettings:(NSManagedObject *)settings;
- (void)setLoopMode:(BOOL)val;
- (void)setIsPlaying:(BOOL)val;
- (void)setLocator:(unsigned long)sampleTime;
- (unsigned long)locator;
- (void)setMasterVolume:(float)value;

// actions
- (void)startStop;
- (void)startPlayback;
- (void)stopPlayback;
- (void)pausePlayback;
- (void)returnToZero;
- (void)tick:(id)sender;
- (void)setLoop;

// prepare content
- (void)updateAudioEngine:(NSNotification *)notification;
- (void)recursivelyAddUpdateRegions:(id)region;

// update GUI
- (void)updateLocator;

@end