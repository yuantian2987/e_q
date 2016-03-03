/* //device/include/server/AudioFlinger/AudioPeakingFilter.cpp
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

#include "AudioPeakingFilter.h"
#include "AudioCommon.h"
#include "EffectsMath.h"

///#include <new>
#include <assert.h>
///#include <cutils/compiler.h>

// Format of the coefficient table:
// kCoefTable[freq][gain][bw][coef]
// freq - peak frequency, in octaves below Nyquist,from -9 to -1.
// gain - gain, in millibel, starting at -9600, jumps of 1024, to 4736 millibel.
// bw   - bandwidth, starting at 1 cent, jumps of 1024, to 3073 cents.
// coef - 0: b0
//        1: b1
//        2: b2
//        3: -a1
//        4: -a2
static const size_t kInDims[3] = {9, 15, 4};
static const audio_coef_t kCoefTable[9*15*4*5] = {
#include "AudioPeakingFilterCoef.inl"
};

void _AudioPeakingFilter(AudioPeakingFilter *mpPeakingFilter, int nChannels, int sampleRate) {
    _AudioBiquadFilter(&(mpPeakingFilter->mBiquad), nChannels, sampleRate);
	_AudioCoefInterpolator(&(mpPeakingFilter->mCoefInterp) , 3, kInDims, 5, (const audio_coef_t*) kCoefTable);
	AudioPeakingConfigure(mpPeakingFilter, nChannels, sampleRate);///
	AudioPeakingReset(mpPeakingFilter); 
}

void AudioPeakingConfigure(AudioPeakingFilter *mpPeakingFilter, int nChannels, int sampleRate) {
    mpPeakingFilter->mNiquistFreq = sampleRate * 500;
    mpPeakingFilter->mFrequencyFactor = ((1ull) << 42) / mpPeakingFilter->mNiquistFreq;
	AudioBiquadConfigure(&(mpPeakingFilter->mBiquad), nChannels, sampleRate);
	AudioPeakingSetFrequency(mpPeakingFilter, mpPeakingFilter->mNominalFrequency);///
	AudioPeakingCommit(mpPeakingFilter, true);///
}

void AudioPeakingClear(AudioPeakingFilter *mpPeakingFilter) { 
	AudioBiquadClear(&(mpPeakingFilter->mBiquad));
}

void AudioPeakingReset(AudioPeakingFilter *mpPeakingFilter) {
	AudioPeakingSetGain(mpPeakingFilter, 0);///
	AudioPeakingSetFrequency(mpPeakingFilter, 0);///
	AudioPeakingSetBandwidth(mpPeakingFilter, 2400);///
	AudioPeakingCommit(mpPeakingFilter, true);///
}

void AudioPeakingProcess(AudioPeakingFilter *mpPeakingFilter, 
	const audio_sample_t in[], audio_sample_t out[], int frameCount, effect_sound_track indx) { 

	AudioBiquadProcess(&(mpPeakingFilter->mBiquad), in, out, frameCount, indx);
}

void AudioPeakingEnable(AudioPeakingFilter *mpPeakingFilter, bool immediate) { 
	AudioBiquadEnable(&(mpPeakingFilter->mBiquad), immediate);
}

void AudioPeakingDisable(AudioPeakingFilter *mpPeakingFilter, bool immediate) { 
	AudioBiquadDisable(&(mpPeakingFilter->mBiquad), immediate);
}

void AudioPeakingSetFrequency(AudioPeakingFilter *mpPeakingFilter, uint32_t millihertz) {
    uint32_t normFreq = 0;
	mpPeakingFilter->mNominalFrequency = millihertz;
    if (CC_UNLIKELY(millihertz > mpPeakingFilter->mNiquistFreq / 2)) {
        millihertz = mpPeakingFilter->mNiquistFreq / 2;
    }
    normFreq = (uint32_t)(
            ((uint64_t)(millihertz) * mpPeakingFilter->mFrequencyFactor) >> 10);
    if (CC_LIKELY(normFreq > (1 << 23))) {
        mpPeakingFilter->mFrequency = (Effects_log2(normFreq) - ((32-9) << 15)) << (FREQ_PRECISION_BITS - 15);
    } else {
        mpPeakingFilter->mFrequency = 0;
    }
}

uint32_t AudioPeakingGetFrequency(AudioPeakingFilter *mpPeakingFilter) { 
	return mpPeakingFilter->mNominalFrequency; 
}

void AudioPeakingSetGain(AudioPeakingFilter *mpPeakingFilter, int32_t millibel) {
    mpPeakingFilter->mGain = millibel + 9600;
}

int32_t AudioPeakingGetGain(AudioPeakingFilter *mpPeakingFilter) { 
	return mpPeakingFilter->mGain - 9600; 
}

void AudioPeakingSetBandwidth(AudioPeakingFilter *mpPeakingFilter, uint32_t cents) {
    mpPeakingFilter->mBandwidth = cents - 1;
}

void AudioPeakingCommit(AudioPeakingFilter *mpPeakingFilter, bool immediate) {
    audio_coef_t coefs[5];
    int intCoord[3] = {
        mpPeakingFilter->mFrequency >> FREQ_PRECISION_BITS,
        mpPeakingFilter->mGain >> GAIN_PRECISION_BITS,
        mpPeakingFilter->mBandwidth >> BANDWIDTH_PRECISION_BITS
    };
    uint32_t fracCoord[3] = {
        mpPeakingFilter->mFrequency << (32 - FREQ_PRECISION_BITS),
        (uint32_t)(mpPeakingFilter->mGain) << (32 - GAIN_PRECISION_BITS),
        mpPeakingFilter->mBandwidth << (32 - BANDWIDTH_PRECISION_BITS)
    };
	AudioCoefInterpolator_GetCoef(&(mpPeakingFilter->mCoefInterp), intCoord, fracCoord, coefs);
	AudioBiquadSetCoefs(&(mpPeakingFilter->mBiquad), coefs, immediate);
}

void AudioPeakingGetBandRange(AudioPeakingFilter *mpPeakingFilter, uint32_t *pLow, uint32_t *pHigh) {
    // Half bandwidth, in octaves, 15-bit precision
    int32_t halfBW = (((mpPeakingFilter->mBandwidth + 1) / 2) << 15) / 1200;

    *pLow = (uint32_t)(((uint64_t)(mpPeakingFilter->mNominalFrequency) * Effects_exp2(-halfBW + (16 << 15))) >> 16);
    if (CC_UNLIKELY(halfBW >= (16 << 15))) {
        *pHigh = mpPeakingFilter->mNiquistFreq;
    } else {
        *pHigh = (uint32_t)(((uint64_t)(mpPeakingFilter->mNominalFrequency) * Effects_exp2(halfBW + (16 << 15))) >> 16);
        if (CC_UNLIKELY(*pHigh > mpPeakingFilter->mNiquistFreq)) {
            *pHigh = mpPeakingFilter->mNiquistFreq;
        }
    }
}

uint32_t AudioPeakingGetBandwidth(AudioPeakingFilter *mpPeakingFilter) { 
	return mpPeakingFilter->mBandwidth + 1; 
}

