/* //device/include/server/AudioFlinger/AudioBiquadFilter.h
**
** Copyright 2007, The Android Open Source Project
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

#ifndef ANDROID_AUDIO_BIQUAD_FILTER_H
#define ANDROID_AUDIO_BIQUAD_FILTER_H

#include "AudioCommon.h"

// A biquad filter.
// Implements the recursion y[n]=a0*y[n-1]+a1*y[n-2]+b0*x[n]+b1*x[n-1]+b2*x[n-2]
// (the a0 and a1 coefficients have an opposite sign to the common convention)
// The filter works on fixed sized blocks of data (frameCount multi-channel
// samples, as defined during construction). An arbitrary number of interlaced
// channels is supported.
// Filter can operate in an enabled (active) or disabled (bypassed) states.
// A mechanism for suppression of artifacts caused by abrupt coefficient changes
// is implemented: normally, when the enable(), disable() and setCoefs() methods
// are called without the immediate flag set, the filter smoothly transitions
// from its current state to the desired state.

// Filter state.
typedef enum _state_t_ {
	// Bypass.
	STATE_BYPASS = 0x01,
	// In the process of smooth transition to bypass state.
	STATE_TRANSITION_TO_BYPASS = 0x02,
	// In the process of smooth transition to normal (enabled) state.
	STATE_TRANSITION_TO_NORMAL = 0x04,
	// In normal (enabled) state.
	STATE_NORMAL = 0x05,
	// A bit-mask for determining whether the filter is enabled or disabled
	// in the eyes of the client.
	STATE_ENABLED_MASK = 0x04
}state_t;

// Max number of channels (can be changed here, and everything should work).
///const int MAX_CHANNELS = 2;
#define MAX_CHANNELS  (2)
// Number of coefficients.
///const int NUM_COEFS = 5;
#define NUM_COEFS  (5)
// The maximum rate of coefficient change, measured in coefficient units per
// second.
///const audio_coef_t MAX_DELTA_PER_SEC = 2000;
#define MAX_DELTA_PER_SEC  (2000)


typedef struct _AudioBiquadFilter_ {

    // Coefficients of identity transformation.
    audio_coef_t IDENTITY_COEFS[NUM_COEFS];

    // Number of channels.
    int mNumChannels;
    // Current state.
    state_t mState;
    // Maximum coefficient delta per sample.
    audio_coef_t mMaxDelta;

    // A bit-mask designating for which coefficients the current value is not
    // necessarily identical to the target value (since we're in transition
    // state).
    uint32_t mCoefDirtyBits;
    // The current coefficients.
    audio_coef_t mCoefs[NUM_COEFS];
    // The target coefficients. Will not be identical to mCoefs if we are in a
    // transition state.
    audio_coef_t mTargetCoefs[NUM_COEFS];

    // The delay lines.
    audio_sample_t mDelays[MAX_CHANNELS][4];

}AudioBiquadFilter;

// A prototype of the actual processing function. Has the same semantics as
// the process() method.
typedef void (*process_func)(AudioBiquadFilter *mBiquad, 
        const audio_sample_t *pIn, audio_sample_t *pOut, int frameCount, effect_sound_track indx);

process_func mCurProcessFunc;

void _AudioBiquadFilter(AudioBiquadFilter *mBiquad, int nChannels, int sampleRate);

void AudioBiquadConfigure(AudioBiquadFilter *mBiquad, int nChannels, int sampleRate);

void AudioBiquadReset(AudioBiquadFilter *mBiquad);

void AudioBiquadClear(AudioBiquadFilter *mBiquad);

void AudioBiquadSetCoefs(AudioBiquadFilter *mBiquad, const audio_coef_t *coefs, bool immediate);

void AudioBiquadProcess(AudioBiquadFilter *mBiquad, 
	const audio_sample_t *pIn, audio_sample_t *pOut, int frameCount, effect_sound_track indx);

void AudioBiquadEnable(AudioBiquadFilter *mBiquad, bool immediate);

void AudioBiquadDisable(AudioBiquadFilter *mBiquad, bool immediate);







#endif // ANDROID_AUDIO_BIQUAD_FILTER_H
