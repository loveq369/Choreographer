//
//  PlaybackController.m
//  Choreographer
//
//  Created by Philippe Kocher on 28.03.10.
//  Copyright 2010 Zurich University of the Arts. All rights reserved.
//

#import "PlaybackController.h"
#import "AudioRegion.h"
#import "TrajectoryItem.h"
#import "EditorContent.h"


@implementation PlaybackController

- (id) init
{
	self = [super init];
	if (self)
	{
		// register for notifications
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(updateAudioEngine:)
													 name:NSManagedObjectContextObjectsDidChangeNotification object:nil];		

		// initialise variables
		locator = 0;
		[self setMasterVolume:0];

	}
	return self;
}

- (void)dealloc
{
	NSLog(@"PlaybackController: dealloc");
	
	[[NSNotificationCenter defaultCenter] removeObserver:self];	
	[playbackEvents release];
	
	[super dealloc];
}


#pragma mark -
#pragma mark accessors
// -----------------------------------------------------------

- (void)setProjectSettings:(ProjectSettings *)settings
{
	[projectSettings release];
	[settings retain];
	projectSettings = settings;
	
	NSManagedObjectContext *context = nil;

	NSLog(@"setProjectSettings: %x %x", projectSettings, context);
	
	// update local variable and gui objects that observe it
	[self setValue:[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopMode"] forKey:@"loopMode"];
	
}


- (void)setLoopMode:(BOOL)val
{
	//	NSLog(@"setLoopMode: %i", val);
	
	loopMode = val;
	
	if(loopMode != [[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopMode"] boolValue])
	{
		[projectSettings setValue:[NSNumber numberWithBool:loopMode] forKeyPath:@"projectSettingsDictionary.loopMode"];
		[self setLoop];
	}
}


- (void)setIsPlaying:(BOOL)val
{
	//	NSLog(@"setIsPlaying: %i", val);
	
	isPlaying = val;
	
	if([[AudioEngine sharedAudioEngine] isPlaying] != val)
		[self startStop];
}


- (void)setLocator:(unsigned long)sampleTime
{
//	NSLog(@"setLocator: %d", sampleTime);
	locator = sampleTime;
	[self updateLocator];

	if(![[AudioEngine sharedAudioEngine] isPlaying] && !loopMode)
	{
		startLocator = sampleTime;
	}
}

- (unsigned long)locator
{
	return locator;
}

- (void)setMasterVolume:(float)value
{
	masterVolume = value;

	[[AudioEngine sharedAudioEngine] setMasterVolume:value];
}


#pragma mark -
#pragma mark actions
// -----------------------------------------------------------

- (void)startStop // spacebar
{
	if(![[AudioEngine sharedAudioEngine] isPlaying])
	{
		[self startPlayback];
	}
	else
	{
		[self pausePlayback];
	}

	// update local variable and all gui objects that observe it
	[self setValue:[NSNumber numberWithBool:[[AudioEngine sharedAudioEngine] isPlaying]] forKey:@"isPlaying"];
}

- (void)startPlayback
{	
	if(playbackTimer)
	{
		[playbackTimer invalidate];
	}
	
	playbackTimer = [NSTimer timerWithTimeInterval:0.01
											target:self
										  selector:@selector(tick:)
										  userInfo:nil
										   repeats:YES];
	
	[[NSRunLoop currentRunLoop] addTimer:playbackTimer forMode:NSRunLoopCommonModes];

	[[AudioEngine sharedAudioEngine] startAudio:locator];

	// update local variable and all gui objects that observe it
	[self setValue:[NSNumber numberWithBool:[[AudioEngine sharedAudioEngine] isPlaying]] forKey:@"isPlaying"];
}

- (void)stopPlayback
{
	[[AudioEngine sharedAudioEngine] stopAudio];
	[playbackTimer invalidate];
	playbackTimer = nil;
	
	// return to start position / loop start
	locator = startLocator;
	[self updateLocator];

	// update local variable and all gui objects that observe it
	[self setValue:[NSNumber numberWithBool:[[AudioEngine sharedAudioEngine] isPlaying]] forKey:@"isPlaying"];
}

- (void)pausePlayback
{
	[[AudioEngine sharedAudioEngine] stopAudio];
	[playbackTimer invalidate];
	playbackTimer = nil;

	locator = [[AudioEngine sharedAudioEngine] playbackLocation];

	[self updateLocator];

	// update local variable and all gui objects that observe it
	[self setValue:[NSNumber numberWithBool:[[AudioEngine sharedAudioEngine] isPlaying]] forKey:@"isPlaying"];
}

- (void)returnToZero
{
	[self stopPlayback];
	[self setLocator:0];
}

- (void)tick:(id)sender
{
	if(!rulerPlayhead.inDraggingSession)
	{
		if(![[AudioEngine sharedAudioEngine] isPlaying])
		{
			[playbackTimer invalidate];
			playbackTimer = nil;
		}
		else
		{
			locator = [[AudioEngine sharedAudioEngine] playbackLocation];
			[self updateLocator];
		}
	}
}

- (void)setLoop
{
//	NSLog(@"mode:%d bounds:%d %d",  [[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopMode"] integerValue],
//									[[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopRegionStart"] integerValue],
//									[[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopRegionEnd"] integerValue]);
//
	if([[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopMode"] integerValue] &&
	   [[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopRegionStart"] integerValue] < [[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopRegionEnd"] integerValue])
	{
		[[AudioEngine sharedAudioEngine] setLoopStart:[[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopRegionStart"] integerValue] end:[[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopRegionEnd"] integerValue]];
		startLocator = [[projectSettings valueForKeyPath:@"projectSettingsDictionary.loopRegionStart"] integerValue];

	}
	else
	{
		[[AudioEngine sharedAudioEngine] unsetLoop];			
	}
}

#pragma mark -
#pragma mark audio engine
// -----------------------------------------------------------

- (void)updateAudioEngine:(NSNotification *)notification
{
	NSDictionary *info = [notification userInfo];

	for (id key in info)
	{
		if(key == NSInsertedObjectsKey)
		{
			for(id object in [info objectForKey:key])
			{		
				if([object isKindOfClass:[TrajectoryItem class]])
				{
					for(id region in [object valueForKey:@"regions"])
					{
						// if the trajectory is newly created and immediately
						// attached to a region, the regions position breakpoint
						// have to be calculated
						[region calculatePositionBreakpoints];
					}
				}
				else if([object isKindOfClass:[AudioRegion class]])
				{
					// update audio engine for this region
					[[AudioEngine sharedAudioEngine] addAudioRegion:object];
				}
			}
		}
		else if(key == NSUpdatedObjectsKey)
		{
			NSMutableSet *updateRegions = [[[NSMutableSet alloc] init] autorelease]; 
			for(id object in [info objectForKey:key])
			{		
				if([object isKindOfClass:[TrajectoryItem class]])
				{
					NSSet *regions = [object valueForKey:@"regions"];
					
					for(id region in regions)
					{
						// update audio engine for all regions this trajectory
						// is attached to
						[region calculatePositionBreakpoints];
						[updateRegions addObject:region];
					}
				}
				else if([object isKindOfClass:[AudioRegion class]])
				{
					[updateRegions addObject:object];
				}
			}
			
			for(id region in updateRegions)
			{
				// update audio engine
				[[AudioEngine sharedAudioEngine] modifyAudioRegion:region];			
			}
		}
		else if(key == NSDeletedObjectsKey)
		{
			for(id object in [info objectForKey:key])
			{		
				if([object isKindOfClass:[AudioRegion class]])
				{
					// update audio engine for these regions
					[[AudioEngine sharedAudioEngine] deleteAudioRegion:object];
				}	
			}
		}
	}
}


#pragma mark -
#pragma mark update GUI
// -----------------------------------------------------------

- (void)updateLocator
{
	[counter setLocator:locator];
	[rulerPlayhead setLocator:locator];
	[playhead setLocator:locator];
	
	[[EditorContent sharedEditorContent] synchronizeWithLocator:locator];
}

@end
