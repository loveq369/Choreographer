//
//  AudioEngine.m
//  Choreographer
//
//  Created by Philippe Kocher on 28.03.10.
//  Copyright 2011 Zurich University of the Arts. All rights reserved.
//

#import "AudioEngine.h"


@interface AudioEngine ()

// private methods
- (void)setGainAutomation:(id)audioRegion;
- (void)setSpatialAutomation:(id)audioRegion;

@end




@implementation AudioEngine

static AudioEngine *sharedAudioEngine = nil;


+ (AudioEngine *)sharedAudioEngine
{
    if (!sharedAudioEngine)
	{
        sharedAudioEngine = [[AudioEngine alloc] init];
		[sharedAudioEngine setup];
    }
    return sharedAudioEngine;
}

+ (void)release
{
	[sharedAudioEngine release];
    sharedAudioEngine = nil;
}


- (id) init
{
	self = [super init];
	if (self)
	{
	}
	return self;
}

- (void)setup
{
	[NSBundle loadNibNamed:@"AudioEngineMainMenu" owner:self];
	
	// insert a menu item in the application's main menu
	NSMenuItem *newItem = [[[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""] autorelease];
	[newItem setSubmenu:menu];
	
	NSMenu *mainMenu = [[NSApplication sharedApplication] mainMenu];
	NSInteger index = [mainMenu numberOfItems] - 2;
	
	[mainMenu insertItem:newItem atIndex:index];
	
	// Juce
	initialiseJuce_GUI();
	
	// instantiate ambisonicsAudioEngine
	ambisonicsAudioEngine = new AmbisonicsAudioEngine();
	
	// instantiate the Speaker Setup Window
	speakerSetupWindowController = [[SpeakerSetupWindowController alloc] init];
	
	// instantiate the Meter Bridge Window
	meterBridgeWindowController = [[MeterBridgeWindowController alloc] init];
	
	// settings
	[self setTestNoiseVolume:-12];

	
	regionIndex = 0;	
	volumeLevelMeasurementClientCount = 0;
}

- (void)dealloc
{
	// Juce
	delete ambisonicsAudioEngine;
	
	shutdownJuce_GUI();
	NSLog(@"AudioEngine.mm: dealloc");
	
	[speakerSetupWindowController release];
	[super dealloc];
}


#pragma mark -
#pragma mark Menu (UI Actions)
// -----------------------------------------------------------

- (IBAction)showHardwareSetup:(id)sender
{
	[self stopAudio];
	
	ambisonicsAudioEngine->showAudioSettingsWindow();
}

- (IBAction)showSpeakerSetup:(id)sender
{	
	[speakerSetupWindowController showWindow:nil];
}

- (IBAction)showMeterBridge:(id)sender
{	
	[meterBridgeWindowController showWindow:nil];
}
	
#pragma mark -
#pragma mark auxiliary playback
// -----------------------------------------------------------

- (void)audioRegionPreview:(id)region;
{
	// play a single audio region
	// output through physical channels 1 + 2
}

- (void)testNoise:(BOOL)enable forChannelatIndex:(NSUInteger)index
{
	ambisonicsAudioEngine->activatePinkNoise(index, enable);
}

#pragma mark -
#pragma mark transport
// -----------------------------------------------------------

- (void)startAudio:(unsigned long)value
{
	// [value] = milliseconds.
	// [value * 0.001] = seconds.
	int positionInSamples = (int)(ambisonicsAudioEngine->getCurrentSampleRate() * 0.001 * (double)value);
	ambisonicsAudioEngine->setPosition(positionInSamples);
	isPlaying = YES;
	
	ambisonicsAudioEngine->start();
}

- (void)stopAudio
{
	isPlaying = NO;

	ambisonicsAudioEngine->stop();
}

- (void)setLoopStart:(unsigned long)start end:(unsigned long)end
{
	ambisonicsAudioEngine->enableArrangerLoop(ambisonicsAudioEngine->getCurrentSampleRate() * 0.001 * (double)start, ambisonicsAudioEngine->getCurrentSampleRate() * 0.001 * (double)end, 0.005);
}

- (void)unsetLoop
{
	ambisonicsAudioEngine->disableArrangerLoop();
}

- (void)bounceToDisk:(NSURL *)URL start:(unsigned long)start end:(unsigned long)end
{
	String absolutePathToAudioFile = [[URL path] cStringUsingEncoding:NSASCIIStringEncoding];
	int bitsPerSample = 16;
	String description("Test bounce");
	String originator("Sam");
	String originatorRef("Choreographer");
	String codingHistory; 
    int sampleRate = (int)ambisonicsAudioEngine->getCurrentSampleRate();
	double fromMsToSamples = 0.001*sampleRate;
	int startSample = start * fromMsToSamples;
	int numberOfSamplesToRead = (end - start) * fromMsToSamples;
	bool succ = ambisonicsAudioEngine->bounceToDisc(absolutePathToAudioFile, 
													bitsPerSample, 
													description,
													originator, 
													originatorRef,
													codingHistory, 
													startSample, 
													numberOfSamplesToRead);
	
	NSLog(@"successful %d", succ);
}


#pragma mark -
#pragma mark getter
// -----------------------------------------------------------


- (BOOL)isPlaying
{
	return isPlaying;
}

- (unsigned long)playbackLocation
{
	return ambisonicsAudioEngine->getCurrentPosition()     // in samples
	       / ambisonicsAudioEngine->getCurrentSampleRate() // now in seconds
		   * 1000;									       // and now in ms.
}

- (unsigned int)sampleRate
{
	return (int)ambisonicsAudioEngine->getCurrentSampleRate();
}

- (unsigned short)numberOfSpeakerChannels
{
	return [[speakerSetupWindowController valueForKeyPath:@"speakerSetups.selectedPreset"] countSpeakerChannels];
}


- (unsigned short)numberOfHardwareDeviceOutputChannels
{
	return ambisonicsAudioEngine->getNumberOfHardwareOutputChannels();
}

- (double)cpuUsage
{
	return ambisonicsAudioEngine->getCpuUsage();
}


#pragma mark -
#pragma mark setter
// -----------------------------------------------------------

- (void)setMasterVolume:(float)dbValue
{
	float gain = pow(10, 0.05 * dbValue);
	ambisonicsAudioEngine->setMasterGain(gain);
}

- (void)setAmbisonicsOrder:(short)order
{
	ambisonicsAudioEngine->setAEPOrder(order);
}

- (void)setdBUnit:(double)unit
{
}

- (void)setUseHipassFilter:(BOOL)filter
{
}

- (void)setUseDelay:(BOOL)delay
{
}

- (void)setTestNoiseVolume:(float)dbValue
{
	float gain = pow(10, 0.05 * dbValue);
	ambisonicsAudioEngine->setAmplitudeOfPinkNoiseGenerator(gain);
}

#pragma mark -
#pragma mark scheduled playback
// -----------------------------------------------------------  

- (void)addAudioRegion:(id)audioRegion
{
	// first this region is given a unique index to identify it in the future
	unsigned int index = regionIndex++;
		
	[audioRegion setValue:[NSNumber numberWithInt:index] forKey:@"playbackIndex"];
	
    int sampleRate = (int)ambisonicsAudioEngine->getCurrentSampleRate();
	double fromMsToSamples = 0.001*sampleRate;
	
	unsigned long  startTime = [[audioRegion valueForKey:@"startTime"] unsignedLongValue] * fromMsToSamples;
	unsigned long  duration = [[audioRegion valueForKey:@"duration"] unsignedLongValue] * fromMsToSamples;
	unsigned long  offsetInFile = [[audioRegion valueForKeyPath:@"audioItem.offsetInFile"] unsignedLongLongValue] * fromMsToSamples;

	NSString *filePath = [audioRegion valueForKeyPath:@"audioItem.audioFile.filePathString"];

	NSLog(@"addAudioRegion(%d) %@", index, filePath);

	
	// add the audio region to the scheduler	
	ambisonicsAudioEngine->addAudioRegion(index,
										  startTime,
										  duration,
										  offsetInFile,
										  [filePath UTF8String]);
	
	
	[self setGainAutomation:audioRegion];	
	[self setSpatialAutomation:audioRegion];	
}

- (void)modifyAudioRegion:(id)audioRegion
{
	unsigned int index = [[audioRegion valueForKey:@"playbackIndex"] unsignedIntValue];

	NSLog(@"modifyAudioRegion(%d) %@", index, [audioRegion valueForKeyPath:@"audioItem.node.name"]);

	int sampleRate = (int)ambisonicsAudioEngine->getCurrentSampleRate();
	double fromMsToSamples = 0.001*sampleRate;
	
	// These new values are measured in samples
	unsigned long  newStartTime = [[audioRegion valueForKey:@"startTime"] unsignedLongValue] * fromMsToSamples;
	unsigned long  newDuration = [[audioRegion valueForKey:@"duration"] unsignedLongValue] * fromMsToSamples;
	unsigned long  newOffsetInFile = [[audioRegion valueForKeyPath:@"audioItem.offsetInFile"] unsignedLongLongValue] * fromMsToSamples;

	ambisonicsAudioEngine->modifyAudioRegion(index, newStartTime, newDuration, newOffsetInFile);

	[self setGainAutomation:audioRegion];	
	[self setSpatialAutomation:audioRegion];	
}

- (void)deleteAudioRegion:(id)audioRegion
{
	unsigned int index = [[audioRegion valueForKey:@"playbackIndex"] unsignedIntValue];
	NSLog(@"deleteAudioRegion(%d) %@", index, [audioRegion valueForKeyPath:@"audioItem.node.name"]);
	
	ambisonicsAudioEngine->removeRegion(index);
}

- (void)deleteAllAudioRegions
{
	ambisonicsAudioEngine->removeAllRegions();
}


- (void)setGainAutomation:(id)audioRegion
{
	unsigned int index = [[audioRegion valueForKey:@"playbackIndex"] unsignedIntValue];
	Array<void*> gainEnvelope;
	float gain;
	
	if([[audioRegion valueForKey:@"muted"] boolValue])
	{
		AudioEnvelopePoint* audioEnvelopePoint = new AudioEnvelopePoint(0, 0.0);
		gainEnvelope.add(audioEnvelopePoint);
	}
	else
	{
		int sampleRate = (int)ambisonicsAudioEngine->getCurrentSampleRate();
		for(id bp in [audioRegion valueForKeyPath:@"gainBreakpointArray"])
		{
			//NSLog(@"gain bp: %d %f", [[bp valueForKey:@"time"] longValue], [[bp valueForKey:@"value"] floatValue]);
			gain = pow(10, 0.05 * [[bp valueForKey:@"value"] floatValue]);
			AudioEnvelopePoint* audioEnvelopePoint = new AudioEnvelopePoint([[bp valueForKey:@"time"] longValue] * 0.001 * sampleRate, gain);
			gainEnvelope.add(audioEnvelopePoint);		
		}
	}
	
	ambisonicsAudioEngine->setGainEnvelopeForRegion(index, gainEnvelope);
	// The gainEnvelope will be deleted in the setGainEnvelope(..) of AudioSourceGainEnvelope
	// or in the destructor of AudioSourceGainEnvelope
}

- (void)setSpatialAutomation:(id)audioRegion
{
	unsigned int index = [[audioRegion valueForKey:@"playbackIndex"] unsignedIntValue];
	Array<SpacialEnvelopePoint> spacialEnvelope;
	
//	NSLog(@"setSpatialAutomation for AudioRegion(%d) %@", index, [audioRegion valueForKeyPath:@"audioItem.node.name"]);
    
    const double spacialFactor = 10.0; // 1 unit in the GUI = 10 units in the audio engine.
	
	int sampleRate = (int)ambisonicsAudioEngine->getCurrentSampleRate();
	double fromMsToSamples = 0.001*sampleRate;
	unsigned long  offsetInFile = [[audioRegion valueForKeyPath:@"audioItem.offsetInFile"] unsignedLongLongValue] * fromMsToSamples;
	for(id bp in [audioRegion valueForKey:@"playbackBreakpointArray"])
	{
		// Reminder from the documentation of ambisonicsAudioEngine->setSpacialEnvelopeForRegion(..):
		//  The spacial envelope contains points. Such a point holds four values: The position
		//  (the time information, measured in samples, starting at the beginning of the audiofile
		//   - not at the beginning of a region with an offset) and the x, y and z coordinates.
		spacialEnvelope.add(SpacialEnvelopePoint([[bp valueForKey:@"time"] longValue] * fromMsToSamples + offsetInFile,
                                                 [[bp valueForKey:@"x"] floatValue] * spacialFactor,
                                                 [[bp valueForKey:@"y"] floatValue] * spacialFactor,
                                                 [[bp valueForKey:@"z"] floatValue] * spacialFactor));
            // The "+ offsetInFile" is needed because the GUI wants to start with the spacial envelope at
            // the start of a region, whereas the audio engine wants it to start at the beginning of the
            // audio file.
		
//		NSLog(@"time: %ld x: %f y: %f z: %f",
//		[[bp valueForKey:@"time"] longValue],
//		[[bp valueForKey:@"x"] floatValue],
//		[[bp valueForKey:@"y"] floatValue],
//		[[bp valueForKey:@"z"] floatValue]);
	}
	
	ambisonicsAudioEngine->setSpacialEnvelopeForRegion(index, spacialEnvelope);
	// The spacialEnvelope will be deleted by AudioSourceAmbipanning
}	


#pragma mark -
#pragma mark hardware
// -----------------------------------------------------------

- (NSArray *)availableAudioDeviceNames
{
	NSLog(@"--- available devices...");
	const StringArray juceStringArray = ambisonicsAudioEngine->getAvailableAudioDeviceNames();
	NSMutableArray *array = [[[NSMutableArray alloc] init] autorelease];
	
	int maxBufferSizeBytes = 120;
	char audioDeviceName[maxBufferSizeBytes];
	
	for (int i = 0; i < juceStringArray.size(); ++i)
	{
		juceStringArray[i].copyToUTF8(audioDeviceName, maxBufferSizeBytes);
		[array addObject:[NSString stringWithCString:audioDeviceName encoding:NSUTF8StringEncoding]];
	}
	
	return [NSArray arrayWithArray:array];
	//	return [NSArray arrayWithObjects:@"device 1", @"second device", @"third device", nil];
}

- (NSString *)nameOfHardwareOutputDevice
{
	String nameOfCurrentAudioDevice = ambisonicsAudioEngine->getNameOfCurrentAudioDevice ();
	
	int maxBufferSizeBytes = 120;
	char audioDeviceName[maxBufferSizeBytes];
	nameOfCurrentAudioDevice.copyToUTF8(audioDeviceName, maxBufferSizeBytes);
	
	return [NSString stringWithCString:audioDeviceName encoding:NSUTF8StringEncoding];
}

- (void)setHardwareOutputDevice:(NSString *)deviceName
{
	String audioDeviceName = [deviceName UTF8String];
	ambisonicsAudioEngine->setAudioDevice(audioDeviceName);	
}


#pragma mark -
#pragma mark speaker setup
// -----------------------------------------------------------

- (void)removeAllSpeakerChannels
{
	ambisonicsAudioEngine->removeAllRoutingsAndAllAepChannels();
	  // Sam an Philippe: Ich verstehe nicht, weshalb das beim Programmstart aufgerufen wird.
}

- (void)addSpeakerChannel:(SpeakerChannel *)channel atIndex:(NSUInteger)index
{	
	double gainLinear = pow(10, 0.05 * channel.dbGain);

	ambisonicsAudioEngine->addAepChannel(index,
										 gainLinear,
										 channel.solo,
										 channel.mute,
										 false,
										 channel.position.x,
										 channel.position.y,
										 channel.position.z);

    ambisonicsAudioEngine->setNewRouting(index, channel.hardwareDeviceOutputChannel);
}

- (void)validateSpeakerSetup
{
	// after adding or removing speaker channels, this method is called
	// to rebuild the audio buffers
	
	ambisonicsAudioEngine->enableNewRouting();

	if(volumeLevelMeasurementClientCount > 0)
	{
		[self enableVolumeLevelMeasurement:YES];		
	}
}


- (void)updateParametersForChannel:(SpeakerChannel *)channel atIndex:(NSUInteger)index
{
	double gain = pow(10, 0.05 * channel.dbGain);
	ambisonicsAudioEngine->setGain(index, gain);
	ambisonicsAudioEngine->setSolo(index, channel.solo);
	ambisonicsAudioEngine->setMute(index, channel.mute);
	
	//	NSLog(@"update channel %i x=%f y=%f z=%f out:%i", index, channel.position.x, channel.position.y, channel.position.z, channel.hardwareDeviceOutputChannel + 1);
	//	NSLog(@" - gain=%f solo=%i mute=%i", gain, channel.solo, channel.mute);
}

- (void)updatePositionForChannel:(SpeakerChannel *)channel atIndex:(NSUInteger)index
{
    ambisonicsAudioEngine->setSpeakerPosition(index,
    										  channel.position.x,
    										  channel.position.y,
    										  channel.position.z);
}

- (void)updateRoutingForChannel:(SpeakerChannel *)channel atIndex:(NSUInteger)index
{
    ambisonicsAudioEngine->setNewRouting(index, channel.hardwareDeviceOutputChannel);
    ambisonicsAudioEngine->enableNewRouting();
}

#pragma mark -
#pragma mark level meter
// -----------------------------------------------------------


- (void)volumeLevelMeasurementClient:(BOOL)val
{
	if(val && volumeLevelMeasurementClientCount == 0)
	{
		[self enableVolumeLevelMeasurement:YES];		
	}
	else if(!val && volumeLevelMeasurementClientCount == 1)
	{
		[self enableVolumeLevelMeasurement:NO];
	}

	if(val)volumeLevelMeasurementClientCount++;
	else volumeLevelMeasurementClientCount--;
	
}

- (void)enableVolumeLevelMeasurement:(BOOL)val
{
	int i;

	for(i=0;i<[self numberOfSpeakerChannels];i++)
	{
		ambisonicsAudioEngine->enableMeasurement(i, val);
	}
}


- (void)resetVolumePeakLevel:(NSUInteger)channel
{
	ambisonicsAudioEngine->resetMeasuredPeakValue(channel);
}

- (float)volumeLevel:(NSUInteger)channel
{
	float gain = ambisonicsAudioEngine->getMeasuredDecayingValue(channel);
	return 20 * log10(gain);
}

- (float)volumePeakLevel:(NSUInteger)channel
{
	float gain = ambisonicsAudioEngine->getMeasuredPeakValue(channel);
	return 20 * log10(gain);
}

 
#pragma mark -
#pragma mark settings
// -----------------------------------------------------------

- (void)setPersistentSetting:(id)data forKey:(NSString *)key
{
	NSDictionary *dict = [NSDictionary dictionaryWithObject:data forKey:key];
	NSUserDefaults *def = [[NSUserDefaults alloc] init];
	[def setPersistentDomain:dict forName:@"net.icst.choreographer.audioEngine"];
	[def release];	
}

- (id)persistentSettingForKey:(NSString *)key
{
	NSUserDefaults *def = [[NSUserDefaults alloc] init];
	NSDictionary *dict = [def persistentDomainForName:@"net.icst.choreographer.audioEngine"];
	id data = [dict objectForKey:key];
	[def release];
	return data;
}


#pragma mark -
#pragma mark real time playback
// -----------------------------------------------------------
//  
/*
- (void)setVolume:(float)volume forVoice:(unsigned int)voice
{
}

- (void)setPosition:(Position *)position forVoice:(unsigned int)voice
{
}
*/

@end