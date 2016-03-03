/* //device/include/server/AudioFlinger/AudioPeakingFilter.h
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

#ifndef ANDROID_AUDIO_PEAKING_FILTER_H
#define ANDROID_AUDIO_PEAKING_FILTER_H

#include "AudioBiquadFilter.h"
#include "AudioCoefInterpolator.h"

// A peaking audio filter, with unity skirt gain, and controllable peak
// frequency, gain and bandwidth.
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
///static const int FREQ_PRECISION_BITS = 26;
#define FREQ_PRECISION_BITS  (26)
// Precision for the mGain member.
///static const int GAIN_PRECISION_BITS = 10;
#define GAIN_PRECISION_BITS  (10)
// Precision for the mBandwidth member.
///static const int BANDWIDTH_PRECISION_BITS = 10;
#define BANDWIDTH_PRECISION_BITS  (10)


typedef struct _AudioPeakingFilter_ {

    // Nyquist, in mHz.
    uint32_t mNiquistFreq;
    // Fractional index into the gain dimension of the coef table in
    // GAIN_PRECISION_BITS precision.
    int32_t mGain;
    // Fractional index into the bandwidth dimension of the coef table in
    // BANDWIDTH_PRECISION_BITS precision.
    uint32_t mBandwidth;
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
    // the low-level biquad coefficients.
    AudioCoefInterpolator mCoefInterp;
}AudioPeakingFilter;

void AudioPeakingConfigure(AudioPeakingFilter *mpPeakingFilter, int nChannels, int sampleRate);

void AudioPeakingReset(AudioPeakingFilter *mpPeakingFilter);

void AudioPeakingProcess(AudioPeakingFilter *mpPeakingFilter, 
	const audio_sample_t in[], audio_sample_t out[], int frameCount, effect_sound_track indx); 

void AudioPeakingEnable(AudioPeakingFilter *mpPeakingFilter, bool immediate);

void AudioPeakingDisable(AudioPeakingFilter *mpPeakingFilter, bool immediate);

void AudioPeakingSetFrequency(AudioPeakingFilter *mpPeakingFilter, uint32_t millihertz);

uint32_t AudioPeakingGetFrequency(AudioPeakingFilter *mpPeakingFilter);

void AudioPeakingSetGain(AudioPeakingFilter *mpPeakingFilter, int32_t millibel);

int32_t AudioPeakingGetGain(AudioPeakingFilter *mpPeakingFilter); 

void AudioPeakingSetBandwidth(AudioPeakingFilter *mpPeakingFilter, uint32_t cents);

void AudioPeakingCommit(AudioPeakingFilter *mpPeakingFilter, bool immediate);

void AudioPeakingGetBandRange(AudioPeakingFilter *mpPeakingFilter, uint32_t *pLow, uint32_t *pHigh);

#endif // ANDROID_AUDIO_PEAKING_FILTER_H

