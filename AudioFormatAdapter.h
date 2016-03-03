/* /android/src/frameworks/base/media/libeffects/AudioFormatAdapter.h
**
** Copyright 2009, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef AUDIOFORMATADAPTER_H_
#define AUDIOFORMATADAPTER_H_

#include "audio_effect.h"
#include "AudioEqualizer.h"

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define BUFFER_SIZE (15834698/2)

typedef struct _AudioFormatAdapter_ {
    // The underlying processor.
    AUDIO_EQUALIZER *mpProcessor;
    // The number of input/output channels.
    int mNumChannels;
    // The desired PCM format.
    uint8_t mPcmFormat;
    // The desired buffer behavior.
    uint32_t mBehavior;
    // An intermediate buffer for processing.
    audio_sample_t mBuffer[BUFFER_SIZE];
    // The buffer size, divided by the number of channels - represents the
    // maximum number of multi-channel samples that can be stored in the
    // intermediate buffer.
    size_t mMaxSamplesPerCall;
}AudioFormatAdapter;

void AudioFormatAdapterConfigure(AudioFormatAdapter *pFormatAdapter, AUDIO_EQUALIZER * pEqualizer, 
				int nChannels, uint8_t pcmFormat, uint32_t behavior);

void AudioFormatAdapterFree(AudioFormatAdapter *pFormatAdapter);


void AudioFormatAdapterProcess(AudioFormatAdapter *pFormatAdapter, 
	const int16_t * pIn, int16_t * pOut, uint32_t numSamples, effect_sound_track indx);

#endif // AUDIOFORMATADAPTER_H_

