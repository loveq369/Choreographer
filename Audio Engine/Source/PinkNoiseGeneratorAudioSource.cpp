/*
  ==============================================================================

   This file is a modification of juce_ToneGeneratorAudioSource.cpp, which is
   a sine generator.

  ==============================================================================
*/



// #include "../../core/juce_StandardHeader.h"
#include "PinkNoiseGeneratorAudioSource.h"


//==============================================================================
PinkNoiseGeneratorAudioSource::PinkNoiseGeneratorAudioSource()
    : gain (1.0),
      b0 (0.0),
      b1 (0.0),
      b2 (0.0),
      juceRandom (Time::currentTimeMillis())
{
}

PinkNoiseGeneratorAudioSource::~PinkNoiseGeneratorAudioSource()
{
}

//==============================================================================
void PinkNoiseGeneratorAudioSource::setGain (const double gain_)
{
    gain = gain_;
}


//==============================================================================
void PinkNoiseGeneratorAudioSource::prepareToPlay (int samplesPerBlockExpected,
												   double sampleRate)
{
    return;
}

void PinkNoiseGeneratorAudioSource::releaseResources()
{
}

void PinkNoiseGeneratorAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
	int lastChannel = info.buffer->getNumChannels() - 1;
	
	// generate pink noise and put it on the last channel
	for (int i = 0; i < info.numSamples; ++i)
	{
		// calculation of white noise
		double white = (2.0 * juceRandom.nextDouble()) -1.0;
		// calculation of pink noise (by filtering the white noise)
		//  source: http://www.firstpr.com.au/dsp/pink-noise/
		//   -> Paul Kellet's economy method
		b0 = 0.99765 * b0 + white * 0.0990460; 
		b1 = 0.96300 * b1 + white * 0.2965164; 
		b2 = 0.57000 * b2 + white * 1.0526913; 
		double pink = b0 + b1 + b2 + white * 0.1848;
		
		// Wanna listen to white noise? Uncomment the next line.
		// pink = white;
		
		// Unity gain is at the nyquist frequency, so the volume needs to be lowered.
		// (This value has been figured out by measurement).
		double correctionFactorForUnityGain = 0.12354247; // = 1/8.0943819999999995
		pink = pink * correctionFactorForUnityGain;
		
		// Apply the amplitude
		pink = pink * gain;
		
		*info.buffer->getSampleData (lastChannel, info.startSample + i) = pink;
	}
	
	// put the (same) noise to the remaining channels
	if (lastChannel > 0)
	{
		for (int i = 0; i < lastChannel; i++)
		{
				info.buffer->copyFrom(i, info.startSample, *info.buffer, lastChannel, info.startSample, info.numSamples);
		}
	}
}