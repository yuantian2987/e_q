/* /android/src/frameworks/base/libs/audioflinger/AudioShelvingFilter.h
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

#ifndef AUDIO_SHELVING_FILTER_H
#define AUDIO_SHELVING_FILTER_H

#include "AudioBiquadFilter.h"
#include "AudioCoefInterpolator.h"

// A shelving audio filter, with unity skirt gain, and controllable cutoff
// frequency and gain.
// This filter is able to suppress introduce discontinuities and other artifacts
// in the output, even when changing parameters abruptly.
// Parameters can be set to any value - this class will make sure to clip them
// when they are out of supported range.
//
// Implementation notes:
// This class uses an underlying biquad filter whose parameters are determined
// using a linear interpolation from a coefficient table, using a
// AudioCoefInterpolator.
// All is left for this class to do is mapping between high-level parameters to
// fractional indices into the coefficient table.

// Precision for the mFrequency member.
static const int FREQ_PRECISION_BITS = 26;
// Precision for the mGain member.
static const int GAIN_PRECISION_BITS = 10;

// Shelf type
typedef enum _ShelfType_ {
	kLowShelf,
	kHighShelf
}ShelfType;

typedef struct _AudioShelvingFilter_ {

    // Shelf type.
    ShelfType mType;
    // Nyquist, in mHz.
    uint32_t mNiquistFreq;
    // Fractional index into the gain dimension of the coef table in
    // GAIN_PRECISION_BITS precision.
    int32_t mGain;
    // Fractional index into the frequency dimension of the coef table in
    // FREQ_PRECISION_BITS precision.
    uint32_t mFrequency;
    // Nominal value of frequency, as set.
    uint32_t mNominalFrequency;
   // 1/Nyquist[mHz], in 42-bit precision (very small).
    // Used for scaling the frequency.
    uint32_t mFrequencyFactor;

    // A biquad filter, used for the actual processing.
    AudioBiquadFilter mBiquad;
    // A coefficient interpolator, used for mapping the high level parameters to
    // the low-level biquad coefficients. This one is used for the high shelf.
    AudioCoefInterpolator mHiCoefInterp;
    // A coefficient interpolator, used for mapping the high level parameters to
    // the low-level biquad coefficients. This one is used for the low shelf.
    AudioCoefInterpolator mLoCoefInterp;
}AudioShelvingFilter;

void AudioShelvingConfigure(AudioShelvingFilter *mpShelf, int nChannels, int sampleRate);

void AudioShelvingClear(AudioShelvingFilter *mpShelf);

void AudioShelvingReset(AudioShelvingFilter *mpShelf);

void AudioShelvingProcess(AudioShelvingFilter *mpShelf, 
	const audio_sample_t in[], audio_sample_t out[], int frameCount, effect_sound_track indx);

void AudioShelvingEnable(AudioShelvingFilter *mpShelf, bool immediate);

void AudioShelvingDisable(AudioShelvingFilter *mpShelf, bool immediate);

void AudioShelvingSetFrequency(AudioShelvingFilter *mpShelf, uint32_t millihertz);

uint32_t AudioShelvingGetFrequency(AudioShelvingFilter *mpShelf);

void AudioShelvingSetGain(AudioShelvingFilter *mpShelf, int32_t millibel);

int32_t AudioShelvingGetGain(AudioShelvingFilter *mpShelf);

void AudioShelvingCommit(AudioShelvingFilter *mpShelf, bool immediate);


#endif // AUDIO_SHELVING_FILTER_H
