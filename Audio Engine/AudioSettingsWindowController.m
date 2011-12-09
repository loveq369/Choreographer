//
//  AudioSettingsWindowController.m
//  Choreographer
//
//  Created by Philippe Kocher on 09.12.11.
//  Copyright 2011 Zurich University of the Arts. All rights reserved.
//

#import "AudioSettingsWindowController.h"
#import "AudioEngine.h"

@implementation AudioSettingsWindowController

- (id)init
{
	self = [self initWithWindowNibName:@"AudioSettingsWindow"];
	if(self)
	{
        bufferSizes = [[[AudioEngine sharedAudioEngine] availableBufferSizes] retain];
		NSUInteger bufferSize = [[AudioEngine sharedAudioEngine] bufferSize];
        
        // find current buffer size and set appropriate index
        for (NSNumber *num in bufferSizes)
        {
            if(bufferSize == [num unsignedIntValue])
            {
                selectedBufferSizeIndex = [bufferSizes indexOfObject:num];
                break;
            }
            
        }
		

        [self setWindowFrameAutosaveName:@"AudioSettingsWindow"];
    }
    
	return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}


- (void)dealloc
{
    [bufferSizes release];
    
    [super dealloc];
}



- (void)setSelectedBufferSizeIndex:(NSUInteger)index
{
	if(index != selectedBufferSizeIndex)
	{
		selectedBufferSizeIndex = index;
		[[AudioEngine sharedAudioEngine] setBufferSize:[[bufferSizes objectAtIndex:index] unsignedIntValue]];
        
		// send notification
		[[NSNotificationCenter defaultCenter] postNotificationName:@"hardwareDidChange" object:self];
	}
}


@end
