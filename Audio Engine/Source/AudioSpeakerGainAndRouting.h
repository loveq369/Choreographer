/*
 *  AudioRegionMixer.h
 *  AudioPlayer
 *
 *  Created by sam (sg@klangfreund.com) on 101014.
 *  Copyright 2010. All rights reserved.
 *
 */

#ifndef __AUDIOSPEAKERGAINANDROUTING_HEADER__
#define __AUDIOSPEAKERGAINANDROUTING_HEADER__

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioRegionMixer.h"
#include "AudioSourceAmbipanning.h"  // To get access to the SpeakerPosition class
#include "PinkNoiseGeneratorAudioSource.h"


//==============================================================================
/**
 Contains the AEP audio channel settings.
 */
class JUCE_API AepChannelSettings
{
public:
    AepChannelSettings();
    
    /**
     @param gain_               The gain of the AEP channel.
     @param speakerPosition_    The speaker position in space in cartesian
                                coordinates.
     */
    AepChannelSettings(const double& gain_,
                       const SpeakerPosition& speakerPosition_);
    
    AepChannelSettings(const double& gain_,
                       const bool& solo_,
                       const bool& mute_,
                       const bool& pinkNoise_,
                       const double& x_,
                       const double& y_,
                       const double& z_);

    /** The copy constructor.
     
     Needed to build an array of AepChannelSettings.   
     @param other   The source object.
     */
    AepChannelSettings(const AepChannelSettings& other);

    /** The assignment operator.
     
     Needed to build an array of AepChannelSettings.
     @param other   The source object from the right hand side.
     @return        This object to allow a=b=c like semantics.
     */
    const AepChannelSettings& operator=(const AepChannelSettings& other);
    
    void setGain(const double& gain_);
    
    void setLastGain(const double& lastGain_);
    
    void setSolo(const bool& engageSolo);
    
    void setMute(const bool& engageMute);
    
    void activatePinkNoise(const bool& turnPinkNoiseOn);
    
    void setSpeakerPosition(const SpeakerPosition& speakerPosition_);
    
    void setSpeakerPosition(const double& x,
                            const double& y,
                            const double& z);
    
    void enableMeasurement(const bool& turnMeasurementOn);
    
    void setMeasuredDecayingValue(const double& measurement);

    void setMeasuredPeakValue(const double& measurement);
    
    const double& getGain();
    
    const double& getLastGain();
    
    const bool& getSoloStatus();
    
    const bool&  getMuteStatus();
    
    const bool&  getPinkNoiseStatus();
    
    const SpeakerPosition& getSpeakerPosition();
    
    const bool&  getMeasurementStatus();
    
    const double& getMeasuredDecayingValue();
    
    const double& getMeasuredPeakValue();

private:
	/** The gain of the AEP channel.
	 */
	double gain;
	
	/** Used to apply a gain ramp in the getNextAudioBlock.
	 */	
	double lastGain;
	
	/** Engages the solo.
	 
	 It is allowed to have solo activated on multiple AEP
	 channels at the same time.
	 */
	bool solo;
	
	/** Engages the mute.
	 */
	bool mute;
	
	/** Activates the pink noise generator on this AEP channel.
	 */
	bool pinkNoise;
	
	/** Specifies the speaker position in space in cartesian coordinates.
	 */
	SpeakerPosition speakerPosition;
	
	/** Enables the measurement of the samples that are sent to
	 the audio driver.
	 
	 The measured values are measuredDecayingValue and
	 measuredPeakValue.
	 */
	bool measurementEnabled;
	
	/** If enableOutputMeasurement is set, the root mean square
	 with a fall-off is stored in this variable.
	 
	 Measured are the samples that are sent to the audio driver.
	 */
	double measuredDecayingValue;

	/** If enableOutputMeasurement is set, the peak value is 
	 stored in this variable.
	 
	 Measured are the samples that are sent to the audio driver.
	 */
	double measuredPeakValue;
	
	JUCE_LEAK_DETECTOR (AepChannelSettings);
};

//==============================================================================
/**
 Represents a connection between an AEP audio channel and a physical
 output of the audio hardware. This is actually just a pair of numbers.
 */
class JUCE_API  AepChannelHardwareOutputPair
{
public:
    AepChannelHardwareOutputPair()
    {
    }
    
    AepChannelHardwareOutputPair(const int& aepChannel_, 
                                 const int& hardwareOutputChannel_)
    :   aepChannel(aepChannel_),
        hardwareOutputChannel(hardwareOutputChannel_)
    {
    }
    
    void setAepChannel(const int& aepChannel_)
    {
        aepChannel = aepChannel_;
    }
    
    void setHardwareOutputChannel(const int& hardwareOutputChannel_)
    {
        hardwareOutputChannel = hardwareOutputChannel_;
    }
    
    const int& getAepChannel()
    {
        return aepChannel;
    }
    
    const int& getHardwareOutputChannel()
    {
        return hardwareOutputChannel;
    }

private:
    /** 
	 The AEP audio channel.
     */
    int aepChannel;
	
    /**
	 The (physical) hardware output channel
     */
    int hardwareOutputChannel;
	
	JUCE_LEAK_DETECTOR (AepChannelHardwareOutputPair);
};

//==============================================================================
/**
 Represents a connection between an AEP audio channel and a physical
 output of the audio hardware. This is actually just a pair of numbers.
 */
class JUCE_API  AepChannelHardwareOutputPairs
{
public:
	AepChannelHardwareOutputPairs();
	
	~AepChannelHardwareOutputPairs();
		
	/**
	 Deletes the (AEP channel, hardware output channel)-pair
	 from the array pairs, if it contains the given AEP channel.
	 Since an AEP channel can have at most one connection,
	 there is at most one pair, that can be removed.
	 */
	void deletePairIfItContainsAEPChannel(const int& aepChannel);
	
	/**
	 Deletes the (AEP channel, hardware output channel)-pair from
	 the array pairs, if it contains the given hardware output channel.
	 Since a hardware output channel can have at most one connection,
	 there is at most one pair, that can be removed.
	 */
	void deletePairIfItContainsHardwareOutputChannel(const int& hardwareOutputChannel);
	
	/**
	 Generates the pair (aepChannel, hardwareOutputChannel). A pointer to
	 this pair is added to the internal array (called pairs).
	 This internal array is ordered according to the second value of
	 the pairs.
	 */
	void add(const int& aepChannel, const int& hardwareOutputChannel);
	
	/** Returns true if the given AEP channel number is part of a pair.	 
	 */
	bool containsAEPChannel(const int& aepChannel);
	
	/** Returns true if the given hardware device channel number is part of a pair.	 
	 */
	bool containsHardwareOutputChannel(const int& hardwareOutputChannel);
	
    int size();
	
	void clear();
	
	const int& getAepChannel(const int& positionOfPairInArray);
	
private:
	Array<AepChannelHardwareOutputPair> pairs;
	
	JUCE_LEAK_DETECTOR (AepChannelHardwareOutputPairs);
};
	

//==============================================================================
//==============================================================================
/**
 A PositionableAudioSource that ...
 
 TODO
 */
class JUCE_API  AudioSpeakerGainAndRouting  : public AudioSource
{
public:
    //==============================================================================
    /** Constructor: Creates an AudioSpeakerGainAndRouting.
	 */
	AudioSpeakerGainAndRouting();
    AudioSpeakerGainAndRouting(AudioRegionMixer* audioRegionMixer_);
	
    /** Destructor. */
    ~AudioSpeakerGainAndRouting();
    
		
    //==============================================================================
    /** Sets the AudioSource that is being used as the input source.
	 
	 The source passed in will not be deleted by this object, so must be managed by
	 the caller.
	 
	 @param newSource             The new input source to use. This might be zero.
	 */
    void setSource (AudioSource* const newAudioRegionMixer);
    
    /** Get the number of available hardware output channels of the
     current audio device.
     
     This value is determined in the method enableNewRouting().
     */
    int getNumberOfHardwareOutputChannels();

	/** Get the number of aep channels with a speaker configuration assigned
	 to them (by setSpeakerPosition).
	 */	
	int getNumberOfAepChannels();
	
	/** Adds a new AEP channel to the array of AEP channels.
	 
	 If the specified channel has already been added, this operation aborts.	 
	 If n := aepChannel and m := array.size < n, then n-m additional channels
	 between the m'th channel and the n'th channel are added with the initial
	 values [gain = 1.0, solo = FALSE, mute = FALSE, activatePinkNoise = FALSE,
	 x = 0.0, y = 1.0, z = 0.0]
	 */
	bool addAepChannel(int aepChannel, double gain, bool solo, bool mute,
					   bool activatePinkNoise, double x, double y, double z);
	
	/** Removes an AEP channel from the array of AEP channels.
	 
	 THIS FUNCTION IS NOT YET IMPLEMENTED.
	 */
	bool removeAepChannel(int aepChannel);
	
    /** Set the speaker position for an AEP channel.
	 
	 If there are AEP channels lower than the specified aepChannel without a
	 speaker position assigned before, they get the position (1,0,0).
	 
	 @param aepChannel	The AEP channel that gets the specified speaker position.
	 @param x			The cartesian x coordinate for the speaker position.
	 @param y			The cartesian y coordinate for the speaker position.
	 @param z			The cartesian z coordinate for the speaker position.
	 */	
	bool setSpeakerPosition(int aepChannel, double x, double y, double z);
	
	/** Sets the gain of the chosen aepChannel.
	 */
	bool setGain(int aepChannel, double gain);

	/** Enables or disables the solo mode of the chosen aepChannel.
	 
	 Multiple aepChannels can have the solo engaged.
	 Solo has a higher priority than mute. E.g. if you solo a muted
	 aepChannel, the audio will get through.
	 */
	bool setSolo(int aepChannel, bool enable);

	/** Enables or disables the mute mode of the chosen aepChannel.
	 */
	bool setMute(int aepChannel, bool enable);

	/** Enables or disables the output of pink noise on the chosen aepChannel.
	 */
	bool activatePinkNoise(int aepChannel, bool enable);
	
	/** Sets the amplitude of the pink noise generator.
	 * This affects the volume of all pink noises on all channels.
	 * Setting a new amplitude won't result in a click, since a gain
	 * ramp is applied to avoid it.
	 */
	void setAmplitudeOfPinkNoiseGenerator(const double newAmplitude_);
										  
	/** Enables or disables the measurement for the chosen aepChannel.
	 */
	bool enableMeasurement(int aepChannel, bool enable);

	/** Resets the measured peak value to zero.
	 */
	bool resetMeasuredPeakValue(int aepChannel);

	/** Returns the measured decaying value.
	 
	 This method is intended to be used by some visualization element
	 e.g. by a VU meter.
	 */
	float getMeasuredDecayingValue(int aepChannel);

	/** Returns the measured peak value.
	 
	 This method is intended to be used by some visualization element
	 e.g. by a VU meter.
	 */
	float getMeasuredPeakValue(int aepChannel);
	
	
	/**
	 Connects an aepChannel and a hardwareDeviceChannel. Each of
	 them can participate in one connection at most. If it happens
	 that there also exist connections with the given
	 aepChannel or with the given hardwareDeviceChannel, these
	 old connections will be removed.
	 
	 To remove the connection with aepChannel, call
	 sotNewRouting(aepChannel, 0).

	 To remove the connection with hardwareOutputChannel, call
	 sotNewRouting(0, hardwareOutputChannel).
	 
	 To enable the new routing, call enableNewRouting.
	 */
	void setNewRouting(int aepChannel, int hardwareOutputChannel);

	/** Removes all connections between the AEP channels and
	 the audio hardware output channels.
	 
	To enable this routing free state, call enableNewRouting.
	*/	
	void removeAllRoutings();
	
	/** It activates the hardware channels needed for the desired
	 routing and assignes the corresponding AEP settings to them.
	 
	 Make sure the playback has been stopped before calling this.
	  
	 The resulting assignments of speaker configurations (for the 
	 active hardware channels) are transmitted to the AudioRegionMixer.
	 
	 @return	The number of active hardware output channels.
	 */
	int enableNewRouting(AudioDeviceManager *audioDeviceManager);

	/** Removes all connections between the AEP channels and
	 the audio hardware output channels as well as all AEP
	 channels.
	 
	 Use this with care! Make sure that you have called
	 audioTransportSource.setSource (0, 0, AUDIOTRANSPORT_BUFFER);
	 before you call this method.
	 */
	void removeAllRoutingsAndAllAepChannels();
	
	/** Puts the AudioSpeakerGainAndRouting object into the bounce mode.
	 
	 Only enable this if the object is not called back by a hardware
	 device! In bounce mode, virtual hardware outputs are generated. The
	 number of virtual hardware outputs is equal to the number of aep
	 channels and they are connected one to one with the aep channels.
	 */
	int switchToBounceMode(bool bounceMode);

    //==============================================================================
    /** Implements the AudioSource method. */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
	
    /** Implements the AudioSource method. */
    void releaseResources();
	
    /** Implements the AudioSource method. */
    void getNextAudioBlock (const AudioSourceChannelInfo& info);
	
    //==============================================================================

	
private:
    /** Updates the private object AudioSpeakerGainAndRouting::positionOfSpeakers
     and passes it to the audioRegionMixer.
     
     This should always be called if a change of a speaker parameter has occured.
     */
    void updateThePositionOfSpeakers();
    
	/** Indicates if the given aepChannel is connected with a
	    hardware output.
	 
	 @return	Indicates if the given aepChannel is connected with
				a hardware output.
	 */
	bool aepChannelIsConnectedWithActiveHardwareChannel(int aepChannel);
	
	AudioSource* audioSource;
	AudioRegionMixer* audioRegionMixer;

	AepChannelHardwareOutputPairs aepChannelHardwareOutputPairs;
			///< This object
			///  hosts an array of aepChannel - hardwareOutputChannel
			///  pairs. These pairs determine the desired connections
            ///  between the AEP- and the hardware channels.
            ///  It's possible that a certain hardware channel does not
            ///  even exist on the current audio device.
            ///  The actual connections are made when enableNewRouting()
            ///  is called. The actual connections are "hardwired" by
            ///  The AudioSpeakerGainAndRouting::aepChannelSettingsOrderedByActiveHardwareChannels.
	
	
	OwnedArray<AepChannelSettings> aepChannelSettingsOrderedByAepChannel;
	Array<AepChannelSettings*> aepChannelSettingsOrderedByActiveHardwareChannels;
	
	Array<AepChannelSettings*> aepChannelSettingsOrderedByActiveHardwareChannelsBackup;
			///< This backup object is only used by switchToBounceMode().
	bool bounceMode;
	
	int numberOfHardwareOutputChannels;
	
	int numberOfMutedChannels; 
	int numberOfSoloedChannels;
	int numberOfChannelsWithActivatedPinkNoise;
	int numberOfChannelsWithEnabledMeasurement;
	
	Array<SpeakerPosition> positionOfSpeakers;
			///< This class needs to know it, because it is responsible
			///  for the deallocation of the SpeakerPosition's. Which happens
			///  in enableNewRouting().
	
	AudioSampleBuffer pinkNoiseBuffer;  ///< Used in getNextAudioBlock .
	AudioSourceChannelInfo pinkNoiseInfo; ///< Used in getNextAudioBlock .
	PinkNoiseGeneratorAudioSource pinkNoiseGeneratorAudioSource;
	
	CriticalSection connectionLock; ///< Used in enableNewRouting .
	CriticalSection audioSpeakerGainAndRoutingLock;

	JUCE_LEAK_DETECTOR (AudioSpeakerGainAndRouting);
};


#endif   // __AUDIOSPEAKERGAINANDROUTING_HEADER__
