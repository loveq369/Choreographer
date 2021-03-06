/*
 *  AudioSourceDopplerEffect.cpp
 *  Choreographer
 *
 *  Created by Samuel Gaehwiler on 120104.
 *  Copyright 2012. All rights reserved.
 *
 */

#include "AudioSourceDopplerEffect.h"
#include <float.h> // To be able to use DBL_MAX

//==============================================================================
// SpacialPosition
const double SpacialPosition::oneOverSpeedOfSound = 0.00294; // = 1.0/340.0 m/s
// Initialisation (and memory allocation) of the static variables
double SpacialPosition::unitScaleFactor = 1000.0;

//==============================================================================

AudioSourceDopplerEffect::AudioSourceDopplerEffect (AudioSourceGainEnvelope& audioSourceGainEnvelope_, double sampleRate_)
  : audioSourceGainEnvelope (audioSourceGainEnvelope_),
    sampleRate (sampleRate_),
    newSpacialEnvelopeSet (false),
    previousSpacialPoint (),
    nextSpacialPoint (),
    currentSpacialPosition (),
    sourceBuffer(1,0)
{
	// DEB("AudioSourceDopplerEffect: constructor called.");
    
    sourceInfo.buffer = &sourceBuffer;
    oneOverSampleRate = 1/sampleRate_;
    
    // Define an initial spacial envelope.
	newSpacialEnvelope.add(new SpacialEnvelopePoint(0,      // time (in samples)
                                                    0,      // x
                                                    0,      // y
                                                    0));	// z);
	spacialEnvelope.addCopiesOf(newSpacialEnvelope);
	constantSpacialPosition = true;
    
    // This is quite crappy, since every time a new instance is created, the
    // static array valuesOfH will be recalculated.
    // But I couldn't figure out a more elegant solution...
    recalculateH(sampleRate_);
    
    // // Temp
    // DEB("AudioSourceDopplerEffect constructor: ")
    // for (double k = -halfTheInterpolationOrder; k < halfTheInterpolationOrder + 1.0; k += 0.1)
    // {
    //     DEB("h(" + String(k) + ") = " + String(h(k)))
    // }
    
}

AudioSourceDopplerEffect::~AudioSourceDopplerEffect()
{
	// DEB("AudioSourceDopplerEffect: destructor called.");
}

/** Implementation of the AudioSource method. */
void AudioSourceDopplerEffect::prepareToPlay (int samplesPerBlockExpected_, double sampleRate_)
{
    sampleRate = sampleRate_;
    oneOverSampleRate = 1/sampleRate_;
    samplesPerBlockExpected = samplesPerBlockExpected_;
    
    // It is assumed that
	//      audioSourceGainEnvelope.prepareToPlay (samplesPerBlockExpected, sampleRate);
    // is called outside.
}

/** Implementation of the AudioSource method. */
void AudioSourceDopplerEffect::releaseResources()
{
    // It is assumed that
	//      audioSourceGainEnvelope.releaseResources();
    // is called outside.
}

/** Implements the PositionableAudioSource method. */
void AudioSourceDopplerEffect::setNextReadPosition (int64 newPosition)
{
//    DEB("AudioSourceDopplerEffect.setNextReadPosition: newPosition = " + String(newPosition))
    
	// If the newPosition is not at the expected position, right after the end
	// of the last audio block
	if (newPosition != audioBlockEndPosition)
	{
		// DEB("AudioSourceDopplerEffect.setNextReadPosition: newPosition = "
        //     + String(newPosition))
		// DEB("AudioSourceDopplerEffect.setNextReadPosition: expected newPosition: " 
		// 	+ String(audioBlockEndPosition))
		
        if (!constantSpacialPosition)
        {
            // Figure out between which audioEnvelopePoints we are right now
            // and set up all variables needed by getNextAudioBlock(..)
            nextSpacialPointIndex = 1;
            prepareForNewPosition(newPosition, 
                                  &nextSpacialPointIndex,
                                  &previousSpacialPoint,
                                  &nextSpacialPoint,
                                  &currentSpacialPosition);
        }
	}
	
	nextPlayPosition = newPosition;
    
    // Set the nextPlayPosition for the audioSourceGainEnvelope
    // --------------------------------------------------------
    if (constantSpacialPosition)
    { 
        audioSourceGainEnvelope.setNextReadPosition(nextPlayPosition - constantSpacialPositionDelayTimeInSamples);
        // If the argument (nextPlayPosition - 
        // constantSpacialPositionDelayTimeInSamples) is smaller than zero,
        // this will set the playhead to the position (argument mod fileLength).
    }
    // If the spacial envelope contains multiple points.
    else
    {
        // audioSourceGainEnvelope.setNextReadPosition will be called in the
        // getNextAudioBlock.
    }
}

/** Implementation of the AudioSource method. */
void AudioSourceDopplerEffect::getNextAudioBlock (const AudioSourceChannelInfo& info)
{
//    DEB("AudioSourceDopplerEffect::getNextAudioBlock: nextPlayPosition = " + String(nextPlayPosition))
//    DEB("AudioSourceDopplerEffect::getNextAudioBlock: info.numSamples = " + String(info.numSamples))

    
    int audioBlockStartPosition = nextPlayPosition;
    // To be precise, audioBlockEndPosition is actually the
    // audioBlockStartPosition of the next audio block.
    audioBlockEndPosition = nextPlayPosition + info.numSamples;
    
    // Note:
    // Thanks to the previous call of getNextAudioBlock or setNextReadPosition()
    // we can be asured that
    // - nextSpacialPointIndex
    // - previousSpacialPoint
    // - nextSpacialPoint
    // - currentSpacialPosition
    // correspond to the audioBlockStartPosition (of the current audio block).
    
    if (newSpacialEnvelopeSet)
    {
        // Apply a ramp from the first sample of this audio block
        // with the delay time from the old spacial envelope
        // to the last sample of this audio block with the delay time
        // from the new spacial envelope.
        
        spacialEnvelope.clear();
        spacialEnvelope.addCopiesOf(newSpacialEnvelope);
        
        // For the first sample (from the old spacial envelope).
        int sampleOffsetCausedByDelay = ceil(currentSpacialPosition.getDelay()*sampleRate);
        int audioBlockStartPositionInclDelay = audioBlockStartPosition - sampleOffsetCausedByDelay;
        double audioBlockStartPositionRemainder = double(sampleOffsetCausedByDelay) - currentSpacialPosition.getDelay()*sampleRate; // in [0, 1[. unit: samples.
        
        // For the last sample (from the new spacial envelope).
        int audioBlockEndPositionInclDelay;
        double audioBlockEndPositionRemainder; // will be in [0, 1[. unit: samples.
        
        if (spacialEnvelope.size() == 1)
        {
            nextSpacialPoint = spacialEnvelope[0];
            
            //            delayOnLastSample = nextSpacialPoint->getDistanceDelay();
            double delayOfNewEnvelope = SpacialPosition(*nextSpacialPoint).getDelay()*sampleRate; // measured in samples
            sampleOffsetCausedByDelay = ceil(delayOfNewEnvelope);
            audioBlockEndPositionInclDelay = audioBlockEndPosition - sampleOffsetCausedByDelay;
            audioBlockEndPositionRemainder = 0.0;
            
            constantSpacialPosition = true;
            constantSpacialPositionDelayTimeInSamples = delayOfNewEnvelope;
        }
        else
        {
            constantSpacialPosition = false;
            // We need to figure out the delay time on the last sample
            // (= the first sample of the next audio block)
            nextSpacialPointIndex = 1;
            prepareForNewPosition(audioBlockEndPosition,
                                  &nextSpacialPointIndex,
                                  &previousSpacialPoint,
                                  &nextSpacialPoint,
                                  &currentSpacialPosition);
                // deltaSpacialPosition = The difference between
                // two adjacent samples AFTER this audio block. I.e. the
                // deltaSpacialPosition for the new spacialEnvelope.
            
            // prepareForNewPosition will also set currentSpacialPosition for 
            // the position audioBlockEndPosition i.e. for the next call of
            // getNextAudioBlock.
            sampleOffsetCausedByDelay = ceil(currentSpacialPosition.getDelay()*sampleRate);
            audioBlockEndPositionInclDelay = audioBlockEndPosition - sampleOffsetCausedByDelay;
            audioBlockEndPositionRemainder = double(sampleOffsetCausedByDelay) - currentSpacialPosition.getDelay()*sampleRate; // in [0, 1[. unit: samples.
        }
        
        // Figure out the lowest and highest sample positions needed.
        // ----------------------------------------------------------
        // (such that we can request a corresponding audio block from the
        //  audioSourceGainEnvelope.)
        // These values are absolute ones (according to the corresponding
        // audio file)
        
        // These two soon will specify the block that will be requested.
        int lowestPositionToRequest = INT_MAX;
        int highestPositionToRequest = INT_MIN;
        // If you are interested in a higher resolution (below the sample
        // resolution) of the borders of this block:
        double lowestPositionRemainder;  // This value will be in [0, 1[. unit: samples.
        double highestPositionRemainder; // This value will be in [-1, 0[. unit: samples.
        
        // Remark: The reason why not everything is calculated in seconds:
        // Precision issues which resulted in crackles (samples in between
        // blocks with a value of 0).
        
        if (audioBlockStartPositionInclDelay < lowestPositionToRequest)
        {
            lowestPositionToRequest = audioBlockStartPositionInclDelay;
            lowestPositionRemainder = audioBlockStartPositionRemainder;
        }
        if (audioBlockStartPositionInclDelay > highestPositionToRequest)
        {
            highestPositionToRequest = audioBlockStartPositionInclDelay;
            highestPositionRemainder = audioBlockStartPositionRemainder;
        }
        
        if (audioBlockEndPositionInclDelay < lowestPositionToRequest)
        {
            lowestPositionToRequest = audioBlockEndPositionInclDelay;
            lowestPositionRemainder = sampleOffsetCausedByDelay - currentSpacialPosition.getDelay()*sampleRate;
        }
        if (audioBlockEndPositionInclDelay > highestPositionToRequest)
        {
            highestPositionToRequest = audioBlockEndPositionInclDelay;
            highestPositionRemainder = sampleOffsetCausedByDelay - currentSpacialPosition.getDelay()*sampleRate;
        }
        
        // We have to add 1 to the highestPositionToRequest since the
        // exact value in seconds will be a little higher than 
        // highestPositionToRequest*oneOverSampleRate and we would like
        // to have this position in the audioblock we will request.
        highestPositionToRequest++;
        // The remainder needs to be changed accordingly.
        highestPositionRemainder = highestPositionRemainder - 1.0;
        // This value will now be in [-1, 0[. unit: samples.
      
        
        // Fill the sourceBuffer with samples
        // ----------------------------------
        
        int numberOfSamplesForAudioBlock = highestPositionToRequest - lowestPositionToRequest + 1;
        // Why "+1"?
        // Example: h=2, l=0  =>  numberOfSamples must be 3 (0,1 and 2).
        
        // We also need to take into account some additional samples on both
        // sides to be able to do interpolation.
        numberOfSamplesForAudioBlock = numberOfSamplesForAudioBlock + 2*halfTheInterpolationOrder;
        
        // The sourceInfo.buffer needs to be big enough.
        // This has been ensured in the method setSpacialEnvelope.
        // But if the info.numSamples is much bigger than
        // the samplesPerBlockExpected, we need to allocate more memory.
        // Remark: Memory allocation is something we don't like in the
        // audio thread. But this one here seems necessary.
        if (sourceInfo.buffer->getNumSamples() < numberOfSamplesForAudioBlock)
        {
            DEB("AudioSourceDopplerEffect: Crap, MEMORY ALLOCATION in the "
                "getNextAudioBlock during the transition from an old "
                "to a new spacial envelope!!!"
                "sourceInfo.buffer->getNumSamples() = " 
                + String(sourceInfo.buffer->getNumSamples()))
            
            bool keepExistingContent = false;
            sourceInfo.buffer->setSize(1, 
                                       numberOfSamplesForAudioBlock, 
                                       keepExistingContent);
        }
        
        // Request the audio block from audioSourceGainEnvelope
        audioSourceGainEnvelope.setNextReadPosition(lowestPositionToRequest - halfTheInterpolationOrder);
        sourceInfo.startSample = 0;
        sourceInfo.numSamples = numberOfSamplesForAudioBlock;
        audioSourceGainEnvelope.getNextAudioBlock(sourceInfo);
        
        
        // Do the actual work (fill that info buffer)
        // ==========================================
        // For every sample in the AudioSampleBuffer info, figure out the
        // (delayed) position in the audio file.
        
        float * sampleOfSource = sourceInfo.buffer->getSampleData(0);
        // Since we have requested some additional samples on both borders
        // for the sake of interpolation, we need to set the pointer
        // to the first requested sample.
        sampleOfSource += halfTheInterpolationOrder;
        
        float * sampleOfDestination = info.buffer->getSampleData(0);
        // Maybe there is an offset for the source buffer, so take care
        // of that:
        sampleOfDestination += info.startSample;        
        
        // It can happen that 0 samples are requested.
        if (info.numSamples != 0)
        {
            // Take care of the first sample of the block
            int startSampleInSource = audioBlockStartPositionInclDelay - lowestPositionToRequest;
            sampleOfSource += startSampleInSource;
            double interSampleRemainder = audioBlockStartPositionRemainder; // In [0, 1[. Unit: Samples.
            
            // THE ACTUAL INTERPOLATION
            // \/  \/  \/  \/  \/  \/  \/
            *sampleOfDestination = interpolate(sampleOfSource, interSampleRemainder);
            
            
            // The distance between two sample positions in the source:
            double sampleOffsetBetweenNeighbours = (double(audioBlockEndPositionInclDelay) + audioBlockEndPositionRemainder - (double(audioBlockStartPositionInclDelay) + audioBlockStartPositionRemainder))/double(info.numSamples);
            
            // Take care of the remaining samples.
            for (int i = 1; i != info.numSamples; i++)
            {
                // Go to the next sample in the destination.
                sampleOfDestination++;
                
                // Figure out the fraction of samples we have to move in 
                // the source.
                interSampleRemainder += sampleOffsetBetweenNeighbours;
                int sampleOffset = floor(interSampleRemainder);
                // Be aware that e.g. floor(-1.5) = -2
                interSampleRemainder = interSampleRemainder - double(sampleOffset);
                
                // Go to the next sample in the source.
                sampleOfSource += sampleOffset;
                
                // THE ACTUAL INTERPOLATION
                // \/  \/  \/  \/  \/  \/  \/
                *sampleOfDestination = interpolate(sampleOfSource, interSampleRemainder);
            }
        }
        
        newSpacialEnvelopeSet = false;
    }
    
    // The regular case
    // ================
    else
    {
        // If the spacialEnvelop contains only one point.
		if (constantSpacialPosition)
		{
            // If there is only one point in the spacial envelope,
            // a constant delay is applied.
            // To avaid noise added by interpolation, we sacrify
            // delay time accuracy and adjust the delay time such that
            // the delay time is a multiple of 1/sampleRate.
            // See constantSpacialPositionDelayTimeInSamples.
            audioSourceGainEnvelope.getNextAudioBlock(info);
            return;
        }
        
        // If there are multiple points in the spacialEnvelope.
        else
        {
            // Get enough samples from the audioSourceGainEnvelope to interpolate.
            // ===================================================================

            
            // Store the values of the start position for later use.
            int sampleOffsetCausedByDelay = ceil(currentSpacialPosition.getDelay()*sampleRate);
            int audioBlockStartPositionInclDelay = audioBlockStartPosition - sampleOffsetCausedByDelay;
            double audioBlockStartPositionRemainder = double(sampleOffsetCausedByDelay) - currentSpacialPosition.getDelay()*sampleRate; // in [0, 1[. unit: samples.
            
            // Figure out the lowest and highest sample positions needed.
            // ----------------------------------------------------------
            // (such that we can request a corresponding audio block from the
            //  audioSourceGainEnvelope.)
            // These values are absolute ones (according to the corresponding
            // audio file)
            
            // These two soon will specify the block that will be requested.
            int lowestPositionToRequest = INT_MAX;
            int highestPositionToRequest = INT_MIN;
            // If you are interested in a higher resolution (below the sample
            // resolution) of the borders of this block:
            double lowestPositionRemainder;  // This value will be in [0, 1[. unit: samples.
            double highestPositionRemainder; // This value will be in [-1, 0[. unit: samples.
            
            // Remark: The reason why not everything is calculated in seconds:
            // Precision issues which resulted in crackles (samples in between
            // blocks with a value of 0).
            
            bool envelopePointsInThisBlock = nextSpacialPoint->getPosition() < audioBlockEndPosition ? true : false;
            
            // Figure out the positions in time (incl. delay) of all spacial
            // points contained in this block - if any.
            if (envelopePointsInThisBlock)
            {
                while (nextSpacialPoint->getPosition() < audioBlockEndPosition)
                {
                    prepareForNewPosition(nextSpacialPoint->getPosition(),
                                          &nextSpacialPointIndex,
                                          &previousSpacialPoint,
                                          &nextSpacialPoint,
                                          &currentSpacialPosition);
                        // This will set the nextSpacialPoint to
                        // the previousSpacialPoint!
                        // And the nextSpacialPoint to the next one.
                        // It will also set currentSpacialPosition which
                        // corresponds to the (newly set) previousSpacialPoint.
                    sampleOffsetCausedByDelay = ceil(currentSpacialPosition.getDelay()*sampleRate);
                    int positionOfCurrentSpacialPoint = previousSpacialPoint->getPosition()
                    - sampleOffsetCausedByDelay;
                    if (positionOfCurrentSpacialPoint < lowestPositionToRequest)
                    {
                        lowestPositionToRequest = positionOfCurrentSpacialPoint;
                        lowestPositionRemainder = double(sampleOffsetCausedByDelay) - currentSpacialPosition.getDelay()*sampleRate;
                    }
                    if (positionOfCurrentSpacialPoint > highestPositionToRequest)
                        // Later we will add 1 sample to the value 
                        // highestPositionToRequest to ensure that the requested
                        // block will contain all positions (in time) we need.
                    {
                        highestPositionToRequest = positionOfCurrentSpacialPoint;
                        highestPositionRemainder = double(sampleOffsetCausedByDelay) - currentSpacialPosition.getDelay()*sampleRate;
                    }
                }
            }
   
            // Figure out the position (incl. delay) of audioBlockEndPosition
            // (the first sample of the next audio block):
            prepareForNewPosition(audioBlockEndPosition, 
                                  &nextSpacialPointIndex,
                                  &previousSpacialPoint,
                                  &nextSpacialPoint,
                                  &currentSpacialPosition);
                // To set the currentSpacialPosition
            sampleOffsetCausedByDelay = ceil(currentSpacialPosition.getDelay()*sampleRate);
            int audioBlockEndPositionInclDelay = audioBlockEndPosition - sampleOffsetCausedByDelay;
            double audioBlockEndPositionRemainder = double(sampleOffsetCausedByDelay) - currentSpacialPosition.getDelay()*sampleRate; // in [0, 1[. unit: samples.
            if (audioBlockEndPositionInclDelay < lowestPositionToRequest)
            {
                lowestPositionToRequest = audioBlockEndPositionInclDelay;
                lowestPositionRemainder = sampleOffsetCausedByDelay - currentSpacialPosition.getDelay()*sampleRate;
            }
            if (audioBlockEndPositionInclDelay > highestPositionToRequest)
            {
                highestPositionToRequest = audioBlockEndPositionInclDelay;
                highestPositionRemainder = sampleOffsetCausedByDelay - currentSpacialPosition.getDelay()*sampleRate;
            }
            
            // Also take the audioBlockStartPosition into account
            if (audioBlockStartPositionInclDelay < lowestPositionToRequest)
            {
                lowestPositionToRequest = audioBlockStartPositionInclDelay;
                lowestPositionRemainder = audioBlockStartPositionRemainder;
            }
            if (audioBlockStartPositionInclDelay > highestPositionToRequest)
            {
                highestPositionToRequest = audioBlockStartPositionInclDelay;
                highestPositionRemainder = audioBlockStartPositionRemainder;
            }
            
            // We have to add 1 to the highestPositionToRequest since the
            // exact value in seconds will be a little higher than 
            // highestPositionToRequest*oneOverSampleRate and we would like
            // to have this position in the audioblock we will request.
            highestPositionToRequest++;
            // The remainder needs to be changed accordingly.
            highestPositionRemainder = highestPositionRemainder - 1.0;
                // This value will now be in [-1, 0[. unit: samples.


            // Fill the sourceBuffer with samples
            // ----------------------------------
            
            int numberOfSamplesForAudioBlock = highestPositionToRequest - lowestPositionToRequest + 1;
                // Why "+1"?
                // Example: h=2, l=0  =>  numberOfSamples must be 3 (0,1 and 2).
            
            // We also need to take into account some additional samples on both
            // sides to be able to do interpolation.
            numberOfSamplesForAudioBlock = numberOfSamplesForAudioBlock + 2*halfTheInterpolationOrder;
            
            // The sourceInfo.buffer needs to be big enough.
            // This has been ensured in the method setSpacialEnvelope.
            // But if the info.numSamples is much bigger than
            // the samplesPerBlockExpected, we need to allocate more memory.
            // Remark: Memory allocation is something we don't like in the
            // audio thread. But this one here seems necessary.
            if (sourceInfo.buffer->getNumSamples() < numberOfSamplesForAudioBlock)
            {
                DEB("AudioSourceDopplerEffect: Crap, MEMORY ALLOCATION in the "
                    "getNextAudioBlock!!! "
                    "sourceInfo.buffer->getNumSamples() = " 
                    + String(sourceInfo.buffer->getNumSamples()))
                
                bool keepExistingContent = false;
                sourceInfo.buffer->setSize(1, 
                                           numberOfSamplesForAudioBlock, 
                                           keepExistingContent);
            }
           
            audioSourceGainEnvelope.setNextReadPosition(lowestPositionToRequest - halfTheInterpolationOrder);
            sourceInfo.startSample = 0;
            sourceInfo.numSamples = numberOfSamplesForAudioBlock;
            audioSourceGainEnvelope.getNextAudioBlock(sourceInfo);
            

            
            // Do the actual work (fill that info buffer)
            // ==========================================
            // For every sample in the AudioSampleBuffer info, figure out the
            // (delayed) position in the audio file.
            
            float * sampleOfSource = sourceInfo.buffer->getSampleData(0);
            // Since we have requested some additional samples on both borders
            // for the sake of interpolation, we need to set the pointer
            // to the first requested sample.
            sampleOfSource += halfTheInterpolationOrder;
            
            float * sampleOfDestination = info.buffer->getSampleData(0);
            // Maybe there is an offset for the source buffer, so take care
            // of that:
            sampleOfDestination += info.startSample;
            
            // If there are no envelope points in this audio block, we will
            // interpolate the delay time between the first sample of this
            // block and the first sample of the next block.
            if (!envelopePointsInThisBlock)
            {
                // It can happen that 0 samples are requested.
                if (info.numSamples != 0)
                {
                    // Take care of the first sample of the block
                    int startSampleInSource = audioBlockStartPositionInclDelay - lowestPositionToRequest;
                    sampleOfSource += startSampleInSource;
                    double interSampleRemainder = audioBlockStartPositionRemainder; // In [0, 1[. Unit: Samples.
                    
                    // THE ACTUAL INTERPOLATION
                    // \/  \/  \/  \/  \/  \/  \/
                    *sampleOfDestination = interpolate(sampleOfSource, interSampleRemainder);
                    
                    
                    // The distance between two sample positions in the source:
                    double sampleOffsetBetweenNeighbours = (double(audioBlockEndPositionInclDelay) + audioBlockEndPositionRemainder - (double(audioBlockStartPositionInclDelay) + audioBlockStartPositionRemainder))/double(info.numSamples);
                    
                    // Take care of the remaining samples.
                    for (int i = 1; i != info.numSamples; i++)
                    {
                        // Go to the next sample in the destination.
                        sampleOfDestination++;
                        
                        // Figure out the fraction of samples we have to move in 
                        // the source.
                        interSampleRemainder += sampleOffsetBetweenNeighbours;
                        int sampleOffset = floor(interSampleRemainder);
                        // Be aware that e.g. floor(-1.5) = -2
                        interSampleRemainder = interSampleRemainder - double(sampleOffset);
                        
                        // Go to the next sample in the source.
                        sampleOfSource += sampleOffset;
                        
                        // THE ACTUAL INTERPOLATION
                        // \/  \/  \/  \/  \/  \/  \/
                        *sampleOfDestination = interpolate(sampleOfSource, interSampleRemainder);
                    }
                }
            }
            
            // If there are one or more envelope points in this audio block.
            // (Remark: This block would also cover the case: no envelope point
            // in the audio block. But the previous block is easier to 
            // read/understand and therefore it is kept for the moment.)
            else
            {
                // Set the startSampleInSource to the correct position
                int startSampleInSource = audioBlockStartPositionInclDelay - lowestPositionToRequest;
                sampleOfSource += startSampleInSource;
                double interSampleRemainder = audioBlockStartPositionRemainder; // In [0, 1[. Unit: Samples.
                
                // Prepare for the upcoming while-loop
                int currentPosition = audioBlockStartPosition;
                int currentPositionInclDelay = audioBlockStartPositionInclDelay;
                int currentPositionRemainder = audioBlockStartPositionRemainder;
                
                while (true)
                {    
                    if (nextSpacialPoint->getPosition() >= audioBlockEndPosition )
                    {
                        // Take care of the first sample.
                        // THE ACTUAL INTERPOLATION
                        // \/  \/  \/  \/  \/  \/  \/
                        *sampleOfDestination = interpolate(sampleOfSource, interSampleRemainder);
                        
                        // Take care of the remaining samples.
                        
                        // The distance between two sample positions in the source:
                        int numberOfSamplesBetweenPoints = audioBlockEndPosition - currentPosition;
                        double sampleOffsetBetweenNeighbours = (double(audioBlockEndPositionInclDelay) + audioBlockEndPositionRemainder - (double(currentPositionInclDelay) + currentPositionRemainder))/double(numberOfSamplesBetweenPoints);
                        
                        currentPosition++;
                        // Go to the next sample in the destination.
                        sampleOfDestination++;
                        while (currentPosition != audioBlockEndPosition)
                        {
                            // Figure out the fraction of samples we have to move in 
                            // the source.
                            interSampleRemainder += sampleOffsetBetweenNeighbours;
                            int sampleOffset = floor(interSampleRemainder);
                            // Be aware that e.g. floor(-1.5) = -2
                            interSampleRemainder = interSampleRemainder - double(sampleOffset);
                            
                            // Go to the next sample in the source.
                            sampleOfSource += sampleOffset;
                            
                            // THE ACTUAL INTERPOLATION
                            // \/  \/  \/  \/  \/  \/  \/
                            *sampleOfDestination = interpolate(sampleOfSource, interSampleRemainder);
                            
                            currentPosition++;
                            // Go to the next sample in the destination.
                            sampleOfDestination++;
                        }                        
                        break;
                    }
                    else
                    {
                        // Take care of the first sample.
                        // ------------------------------
                        // THE ACTUAL INTERPOLATION
                        // \/  \/  \/  \/  \/  \/  \/
                        *sampleOfDestination = interpolate(sampleOfSource, interSampleRemainder);
                        
                        // Preparations for the upcoming loop.
                        // -----------------------------------
                        // Figure out
                        // - nextSpacialPointPositionInclDelay and
                        // - nextSpacialPointPositionRemainder
                        SpacialPosition nextSpacialPointPosition(*nextSpacialPoint);
                        sampleOffsetCausedByDelay = ceil(nextSpacialPointPosition.getDelay()*sampleRate);
                        int nextSpacialPointPositionInclDelay = nextSpacialPoint->getPosition() - sampleOffsetCausedByDelay;
                        double nextSpacialPointPositionRemainder = double(sampleOffsetCausedByDelay) - nextSpacialPointPosition.getDelay()*sampleRate;
                        int numberOfSamplesBetweenPoints = nextSpacialPoint->getPosition() - currentPosition;                        
                        // The distance between two sample positions in the source:
                        double sampleOffsetBetweenNeighbours = (double(nextSpacialPointPositionInclDelay) + nextSpacialPointPositionRemainder - (double(currentPositionInclDelay) + currentPositionRemainder))/double(numberOfSamplesBetweenPoints);
                        
                        // Take care of the remaining samples.
                        // -----------------------------------
                        currentPosition++;
                        // Go to the next sample in the destination.
                        sampleOfDestination++;
                        while (currentPosition != nextSpacialPoint->getPosition())
                        {
                            // Figure out the fraction of samples we have to move in 
                            // the source.
                            interSampleRemainder += sampleOffsetBetweenNeighbours;
                            int sampleOffset = floor(interSampleRemainder);
                            // Be aware that e.g. floor(-1.5) = -2
                            interSampleRemainder = interSampleRemainder - double(sampleOffset);
                            
                            // Go to the next sample in the source.
                            sampleOfSource += sampleOffset;
                            
                            // THE ACTUAL INTERPOLATION
                            // \/  \/  \/  \/  \/  \/  \/
                            *sampleOfDestination = interpolate(sampleOfSource, interSampleRemainder);
                            
                            currentPosition++;
                            // Go to the next sample in the destination.
                            sampleOfDestination++;
                        }
                        
                        // Now we are at the position currentPosition = next spacial point.
                        
                        // Get the parameters of the current position (= next spacial point).
                        prepareForNewPosition(nextSpacialPoint->getPosition(),
                                              &nextSpacialPointIndex,
                                              &previousSpacialPoint,
                                              &nextSpacialPoint,
                                              &currentSpacialPosition);
                        // This will set the nextSpacialPoint to
                        // the previousSpacialPoint!
                        // And the nextSpacialPoint to the next one.
                        // It will also set currentSpacialPosition which
                        // corresponds to the (newly set) previousSpacialPoint.
                        sampleOffsetCausedByDelay = ceil(currentSpacialPosition.getDelay()*sampleRate);
                        currentPositionInclDelay = previousSpacialPoint->getPosition() - sampleOffsetCausedByDelay;
                        currentPositionRemainder = double(sampleOffsetCausedByDelay) - currentSpacialPosition.getDelay()*sampleRate;
                    }
                }
            }
        }
    }
}

/** Implements the PositionableAudioSource method. */
int64 AudioSourceDopplerEffect::getNextReadPosition () const
{
	// return audioSourceGainEnvelope.getNextReadPosition();
    return 0;
}

/** Implements the PositionableAudioSource method. */
int64 AudioSourceDopplerEffect::getTotalLength () const
{
	return audioSourceGainEnvelope.getTotalLength();
}

/** Implements the PositionableAudioSource method. */
bool AudioSourceDopplerEffect::isLooping () const
{
	return audioSourceGainEnvelope.isLooping();
}

void AudioSourceDopplerEffect::setSpacialEnvelope (const Array<SpacialEnvelopePoint>& newSpacialEnvelope_)
{
    // DEB("AudioSourceDopplerEffect: setSpacialEnvelope called")
	
	if (newSpacialEnvelope_.size() != 0)
	{
        // STEP 1: Copy the envelope
        // -------------------------
        // Copy the elements over from the Array newSpacialEnvelope_ to the
        // OwnedArray newSpacialEnvelope.
        newSpacialEnvelope.clear();
        for (int i=0; i!=newSpacialEnvelope_.size(); ++i)
        {
            newSpacialEnvelope.add(new SpacialEnvelopePoint(newSpacialEnvelope_[i]));
        }
        
        // STEP 2: Set the buffer size
        // ---------------------------
        // Set the buffer size such that it is big enough for the fastest
        // movements in the new spacial envelope.
        // In STEP 3 we will add an additional point between 2 adjecent points
        // if the distance to the origin is closer than on the other two points.
        // The change in distance (per sample) is bigger on the 2 original
        // points. Thats why this step comes first (to save some calculations).
        
        double highestSlope = 0.0; // seconds / sample
        double lowestSlope = 0.0; // seconds / sample
        // if newSpacialEnvelope.size() == 1, highestSlope = lowestSlope = 0
        if (newSpacialEnvelope.size() > 1)
        {
            for (int k=1; k<newSpacialEnvelope.size(); k++)
            {
                SpacialEnvelopePoint * previousEnvPoint = newSpacialEnvelope[k-1];
                SpacialEnvelopePoint * nextEnvPoint = newSpacialEnvelope[k];
                
                if (previousEnvPoint != nextEnvPoint)
                {
                    // Figure out the highest and lowest change of [distance
                    // to origin] between two adjecent samples on this segment
                    // of the envelope.
                    
                    // The time distance in samples between the two
                    // spacial envelope points
                    double distance = double (nextEnvPoint->getPosition() - previousEnvPoint->getPosition());
                    
                    SpacialPosition previousPoint (*previousEnvPoint);
                    SpacialPosition nextPoint (*nextEnvPoint);
                    SpacialPosition spacialDelta = (nextPoint - previousPoint) * (1./distance);
                    SpacialPosition previousPointPlusOne = previousPoint + spacialDelta;
                    SpacialPosition nextPointMinusOne = nextPoint - spacialDelta;
                    
                    double slope1 = previousPointPlusOne.getDelay() - previousPoint.getDelay(); // seconds / sample
                    double slope2 = nextPoint.getDelay() - nextPointMinusOne.getDelay(); // seconds / sample
                    
                    highestSlope = jmax(jmax(highestSlope, slope1), slope2); // will be >= 0
                    lowestSlope = jmin(jmin(lowestSlope, slope1), slope2); // will be <= 0
                }
            }
        }
        // The highest and lowest time difference between two adjacent
        // (output) samples.
        double regularDifference = oneOverSampleRate;
        // A positive slope means the sound source is moving away and therefore
        // the playback speed is decreasing.
        // lowestTimeDifference <= regularDifference.
        double lowestTimeDifference = regularDifference - highestSlope;
        // A negative slope means the sound source is moving closer and 
        // therefore the playback speed is increasing.
        // highestTimeDifference >= regularDifference.
        double highestTimeDifference = regularDifference - lowestSlope;
        
        double biggestTimeDifference = jmax(highestTimeDifference, 
                                            std::abs(lowestTimeDifference));
        
        double audioBlockStretchFactor = biggestTimeDifference / regularDifference;
        
        // If this estimation is wrong and a bigger audio block is needed in
        // getNextAudioBlock(), memory allocation will be done there again.
        // (hopefully the audio driver behaves nice and it is not neccessary).
        double estimatedMaxToExpectedAudioBlockRatio = 1.5;
        int maxSamplesPerBlockForSource = std::ceil(samplesPerBlockExpected * 
                                                    audioBlockStretchFactor *
                                                    estimatedMaxToExpectedAudioBlockRatio);
        // Allocate memory for the buffer used for the
        // AudioSourceGainEnvelope.getNextAudioBlock() in this 
        // getNextAudioBlock() when the new spacial envelope is engaged.
        // Since this change might happen while getNextAudioBlock() is working
        // with the current spacial envelope, only growth is allowed.
        if (sourceInfo.buffer->getNumSamples() < maxSamplesPerBlockForSource)
        {
            bool keepExistingContent = true;
            sourceInfo.buffer->setSize(1, 
                                       maxSamplesPerBlockForSource, 
                                       keepExistingContent);
        }
        
        // STEP 3
        // ------
        // Figure out the points on the envelope that are locally closest to 
        // the origin. I.e. for two neighbouring envelope points check if there
        // might be a point in between that is closest to the origin.
        // If so, add this point to the envelope.
        // At the end of this procedure, the points closest and farest away
        // from the origin are explicit points of the envelope.
        OwnedArray<SpacialEnvelopePoint> closestPointsInBetween;
        if (newSpacialEnvelope.size()>1)
        {
            for (int i=1; i!=newSpacialEnvelope.size(); ++i)
            {
                SpacialPosition a = SpacialPosition(newSpacialEnvelope[i-1]->getX(),
                                                    newSpacialEnvelope[i-1]->getY(),
                                                    newSpacialEnvelope[i-1]->getZ());
                SpacialPosition b = SpacialPosition(newSpacialEnvelope[i]->getX(),
                                                    newSpacialEnvelope[i]->getY(),
                                                    newSpacialEnvelope[i]->getZ());
                // a and b are positions in space without a 4rd time argument
                // like SpacialEnvelopePoints.
                
                // If a == b they are at the same position in space and
                // all points in between are equally close to the origin.
                if (a != b)
                {
                    // See my notes 120123_doppler_fx_closest_point.tif for
                    // more details of the upcoming calculation.
                    
                    // n is a vector that specifies the direction of the line
                    // defined by a and b.
                    SpacialPosition n = b - a;
                    
                    // With the scalar product, we can specify a plane
                    // that is perpendicular to the line and intersects with
                    // the origin:
                    //  {r in Reals^3 ; <r,n> = 0}
                    //
                    // The line can be parametrised:
                    //  {a + t*n ; t in Reals}
                    //
                    // The intersection of the line and the plane will result
                    // in the point (p) on the line closest to the origin.
                    // So lets intersect!
                    // I.e. put the parametrised line into the equation of
                    // the plane and solve for t...
                    //  <a + tn, n> = 0
                    //  <a,n> + t <n,n> = 0
                    //  t = - <a,n>/<n,n>
                    // ... and put this t into the parametrisation of the line:
                    //  p = a - <a,n>/<n,n> * n
                    
                    // The point closest to the origin
                    SpacialPosition p = a - ((a*n)/(n*n))*n;
                    
                    // Does this point lie in between a and b?
                    // It is sufficient to look at one coordinate.
                    double * smallerX = a.x < b.x ? &a.x : &b.x;
                    double * biggerX =  a.x < b.x ? &b.x : &a.x;
                    if (p.x > *smallerX && p.x < *biggerX)
                    {
                        // If the point lies in between a and b,
                        // add it to the closestPointsInBetween array.
                        // (Otherwise don't)
                        
                        // Figure out the time position for this new point
                        double positionOfA = double (newSpacialEnvelope[i-1]->getPosition());
                        double positionOfB = double (newSpacialEnvelope[i]->getPosition());
                        
                        // (posOfP-posOfA)/(posOfB-posOfA) = (p.x-a.x)/(b.x-a.x)
                        double positionOfP = positionOfA + (p.x - a.x)/(b.x - a.x) * (positionOfB - positionOfA);
                        
                        SpacialEnvelopePoint* newSpacialEnvPoint = 
                            new SpacialEnvelopePoint(p.x, p.y, p.z,
                                                     floor(positionOfP + 0.5));
                            // positionOfP is positive
                            // => floor(positionOfP + 0.5) == round(positionOfP)
                        closestPointsInBetween.add(newSpacialEnvPoint);
                    }
                }
            }
        }
        
        // Add the points from closestPointsInBetween to the newSpacialEnvelope.
        if (closestPointsInBetween.size() > 0)
        {
//            // start debugging
//            for (int i=0; i!=closestPointsInBetween.size(); ++i)
//            {
//                DEB("AudioSourceDopplerEffect: setSpacialEnvelope:")
//                DEB("closestPointsInBetween[" + String(i) + "]->getPosition() ="
//                    + String(closestPointsInBetween[i]->getPosition()))
//            }
//            // end debugging
            
            // Add the newly found points to the newSpacialEnvelope.
            newSpacialEnvelope.addCopiesOf(closestPointsInBetween);
            
            // Sort the newSpacialEnvelope
            newSpacialEnvelope.sort(spacialEnvelopePointPointerComparator);
            
            // start debugging
//            for (int i=0; i!=newSpacialEnvelope.size(); ++i)
//            {
//                DEB("AudioSourceDopplerEffect: setSpacialEnvelope:")
//                DEB("newSpacialEnvelope[" + String(i) + "]->getPosition() ="
//                    + String(newSpacialEnvelope[i]->getPosition()))
//            }
            // end debugging
            
        }
        
        // STEP 4
        // ------
        newSpacialEnvelopeSet = true; // when set, the spacial value 
        // is faded from the old spacial envelope to the new one, in 
        // the interval of one audio block in the upcoming call of
        // getNextAudioBlock(..).
    }
    else
    {
		DEB("AudioSourceDopplerEffect: The newSpacialEnvelope is empty! The spacial envelope hasn't been changed.")
	}
}

inline void AudioSourceDopplerEffect::prepareForNewPosition (int newPosition,
                                   int * nextSpacialPointIndex_,
                                   SpacialEnvelopePoint ** previousSpacialPoint_,
                                   SpacialEnvelopePoint ** nextSpacialPoint_,
                                   SpacialPosition * currentSpacialPosition_)
{    
    // Figure out between which audioEnvelopePoints we are right now
	// and set up
    // - nextSpacialPointIndex_
    // - previousSpacialPoint_
    // - nextSpacialPoint_
	while (spacialEnvelope[nextSpacialPointIndex]->getPosition() <= newPosition)
	{
		(*nextSpacialPointIndex_)++;
	}
	*previousSpacialPoint_ = spacialEnvelope[*nextSpacialPointIndex_ - 1];
	*nextSpacialPoint_ = spacialEnvelope[*nextSpacialPointIndex_];

	// Figure out the *currentSpacialPosition_
    // ---------------------------------------
    
    // The distance in time. In samples.
	double distance = double ((*nextSpacialPoint_)->getPosition() 
                              - (*previousSpacialPoint_)->getPosition());
    // In samples.
	double distanceFromPreviousSpacialPointPosToCurrentPos = double(newPosition 
    - (*previousSpacialPoint_)->getPosition());
    
    double factor = distanceFromPreviousSpacialPointPosToCurrentPos / distance;
    
    currentSpacialPosition_->x = (*previousSpacialPoint_)->getX() + factor * ((*nextSpacialPoint_)->getX() - (*previousSpacialPoint_)->getX());
    currentSpacialPosition_->y = (*previousSpacialPoint_)->getY() + factor * ((*nextSpacialPoint_)->getY() - (*previousSpacialPoint_)->getY());
    currentSpacialPosition_->z = (*previousSpacialPoint_)->getZ() + factor * ((*nextSpacialPoint_)->getZ() - (*previousSpacialPoint_)->getZ());
}

float AudioSourceDopplerEffect::interpolate (float * sampleRightBefore, double remainder)
{
    /*
     y(sampleRightBefore + remainder)
     = y(t_n) 
     = \sum_{k \in \mathbb{Z}} x[k]h(t_n - k)
     = \sum_{k' \in \mathbb{Z}} x[sampleRightBefore + k']h(remainder - k)
     
     \approx \sum_{k' = -halfTheInterpolationOrder}^{halfTheInterpolationOrder}
        x[sampleRightBefore + k']h(remainder - k)
     */
    
    double result = 0.0;
    for (int k = -halfTheInterpolationOrder+1; k <= halfTheInterpolationOrder; k++)
    {
        result += *(sampleRightBefore+k) * h(remainder - k);
    }
    
    // Temp
    // result = *sampleRightBefore;
    return result;
}

double AudioSourceDopplerEffect::h(double t)
{
    // Since h is symmetric (h(t) = h(-t) for all t)
    // only values of h for t >=0 are stored in the array valuesOfH.
    
    int positionInArray = t*double(interpolationStepsPerUnit);
    
    // Calculate
    // positionInArray = abs(positionInArray)
    if (positionInArray < 0)
    {
        positionInArray = -positionInArray;
    }
    // TODO
    // Is there a more elegant way to calculate abs(integer)?
    
    return valuesOfH[positionInArray];
}

bool AudioSourceDopplerEffect::recalculateH(double samplerate)
{
    int numberOfElements = (halfTheInterpolationOrder + 1) * interpolationStepsPerUnit;
    valuesOfH.ensureStorageAllocated(numberOfElements);
    
    // The normalized cutoff frequency
    double fcn = cutoffFrequencyOfInterpolationLPF/samplerate;
    
    double stepSize = 1.0/double(interpolationStepsPerUnit);
    
    // Calculate values of the impulse response of the ideal filter between
    // 0 and halfTheInterpolationOrder in steps of stepSize.
    for (int i = 0; i<halfTheInterpolationOrder; i++)
    {
        
        // Take care of the integer argument: h(i)=...
        if (i == 0)
        {
            // h(0) = 1. Here we have to provide the value of sinc(0)=1.
            // Otherwise we would have a division by zero.
            double value = 1.0;
            valuesOfH.set(0, value);
        }
        else
        {
            double argument = 2.0*pi*fcn*double(i);
            double value = sin(argument)/argument;
            valuesOfH.set(i*interpolationStepsPerUnit, value);
        }
        
        // Take care of the remaining arguments between i and i+1.
        for (int k=1; k<interpolationStepsPerUnit; k++)
        {
            double argument = 2.0*pi*fcn*(double(i) + k*stepSize);
            double value = sin(argument)/argument;
            valuesOfH.set(i*interpolationStepsPerUnit + k, value);
        }
    }
    
    // Apply a raised-cosine window.
    for (int i = 0; i<halfTheInterpolationOrder; i++)
    {
        for (int k=0; k<interpolationStepsPerUnit; k++)
        {
            int positionInArray = i*interpolationStepsPerUnit + k;
            double originalValue = valuesOfH[positionInArray];
            double raisedCosineValue = 0.5 * (1.0 + cos((2*pi*(double(i)+k*stepSize))/(2.*double(halfTheInterpolationOrder) +2)));
            valuesOfH.set(positionInArray, originalValue*raisedCosineValue);
        }
    }
    
    
    return true;
}



// Initialisation (and memory allocation) of the static variables
int AudioSourceDopplerEffect::halfTheInterpolationOrder = 5;
int AudioSourceDopplerEffect::interpolationStepsPerUnit = 128;
double AudioSourceDopplerEffect::cutoffFrequencyOfInterpolationLPF = 20000.0; // Hz
double AudioSourceDopplerEffect::pi = 4.0 * atan(1.0);
Array<double> AudioSourceDopplerEffect::valuesOfH;

// All we would have liked to do is to call AudioSourceDopplerEffect::recalculateH(). 
// Sadly, the C++ compiler doesn't allow this directly. Thats why the dummy 
// variable __recalculateH exists.
bool AudioSourceDopplerEffect::__recalculateH = AudioSourceDopplerEffect::recalculateH(44100);
    // recalculateH will be called again in the constructor with the correct sampleRate
