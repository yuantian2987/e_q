/* /android/src/frameworks/base/libs/audioflinger/AudioShelvingFilter.cpp
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

#include "AudioShelvingFilter.h"
#include "AudioCommon.h"
#include "EffectsMath.h"

///#include <new>
#include <assert.h>
///#include <cutils/compiler.h>

// Format of the coefficient tables:
// kCoefTable[freq][gain][coef]
// freq  - cutoff frequency, in octaves below Nyquist,from -10 to -6 in low
//         shelf, -2 to 0 in high shelf.
// gain  - gain, in millibel, starting at -9600, jumps of 1024, to 4736 millibel.
// coef - 0: b0
//        1: b1
//        2: b2
//        3: -a1
//        4: -a2
static const size_t kHiInDims[2] = {3, 15};
static const audio_coef_t kHiCoefTable[3*15*5] = {
#include "AudioHighShelfFilterCoef.inl"
};
static const size_t kLoInDims[2] = {5, 15};
static const audio_coef_t kLoCoefTable[5*15*5] = {
#include "AudioLowShelfFilterCoef.inl"
};

void _AudioShelvingFilter(AudioShelvingFilter *mpShelf, ShelfType type, int nChannels, int sampleRate) {
    mpShelf->mType = type;      
    _AudioBiquadFilter(&(mpShelf->mBiquad), nChannels, sampleRate);
	if(type == kLowShelf){
	    _AudioCoefInterpolator(&(mpShelf->mLoCoefInterp), 2, kLoInDims, 5, (const audio_coef_t*)kLoCoefTable);
	}
	else{
		_AudioCoefInterpolator(&(mpShelf->mHiCoefInterp), 2, kHiInDims, 5, (const audio_coef_t*)kHiCoefTable);
	}
	AudioShelvingConfigure(mpShelf, nChannels, sampleRate);///
}

void AudioShelvingConfigure(AudioShelvingFilter *mpShelf, int nChannels, int sampleRate) {
    mpShelf->mNiquistFreq = sampleRate * 500;
    mpShelf->mFrequencyFactor = ((1ull) << 42) / mpShelf->mNiquistFreq;
	AudioBiquadConfigure(&(mpShelf->mBiquad), nChannels, sampleRate);
	AudioShelvingSetFrequency(mpShelf, mpShelf->mNominalFrequency);
	AudioShelvingCommit(mpShelf, true);
}

void AudioShelvingClear(AudioShelvingFilter *mpShelf) { 
	AudioBiquadClear(&(mpShelf->mBiquad));
}

void AudioShelvingReset(AudioShelvingFilter *mpShelf) {
	AudioShelvingSetGain(mpShelf, 0);///
	mpShelf->mType == kLowShelf ? 0 : mpShelf->mNiquistFreq;
	AudioShelvingSetFrequency(mpShelf, mpShelf->mType);/// 
	AudioShelvingCommit(mpShelf, true);///
}

void AudioShelvingProcess(AudioShelvingFilter *mpShelf, 
	const audio_sample_t in[], audio_sample_t out[], int frameCount, effect_sound_track indx) { 

	AudioBiquadProcess(&(mpShelf->mBiquad), in, out, frameCount, indx);
}

void AudioShelvingEnable(AudioShelvingFilter *mpShelf, bool immediate) { 
	AudioBiquadEnable(&(mpShelf->mBiquad), immediate);
}
void AudioShelvingDisable(AudioShelvingFilter *mpShelf, bool immediate) { 
	AudioBiquadDisable(&(mpShelf->mBiquad), immediate);
}

void AudioShelvingSetFrequency(AudioShelvingFilter *mpShelf, uint32_t millihertz) {
    uint32_t normFreq = 0;
	uint32_t log2minFreq = 0;
	mpShelf->mNominalFrequency = millihertz;
    if (CC_UNLIKELY(millihertz > mpShelf->mNiquistFreq / 2)) {
        millihertz = mpShelf->mNiquistFreq / 2;
    }
    normFreq = (uint32_t)(
            ((uint64_t)(millihertz) * mpShelf->mFrequencyFactor) >> 10);
    log2minFreq = (mpShelf->mType == kLowShelf ? (32-10) : (32-2));
    if (CC_LIKELY(normFreq > (1U << log2minFreq))) {
        mpShelf->mFrequency = (Effects_log2(normFreq) - (log2minFreq << 15)) << (FREQ_PRECISION_BITS - 15);
    } else {
        mpShelf->mFrequency = 0;
    }
}

uint32_t AudioShelvingGetFrequency(AudioShelvingFilter *mpShelf) { 
	return mpShelf->mNominalFrequency; 
}

void AudioShelvingSetGain(AudioShelvingFilter *mpShelf, int32_t millibel) {
    mpShelf->mGain = millibel + 9600;
}

int32_t AudioShelvingGetGain(AudioShelvingFilter *mpShelf) { 
	return mpShelf->mGain - 9600; 
}

void AudioShelvingCommit(AudioShelvingFilter *mpShelf, bool immediate) {
    audio_coef_t coefs[5];
    int intCoord[2] = {
        mpShelf->mFrequency >> FREQ_PRECISION_BITS,///right shift 26
        mpShelf->mGain >> GAIN_PRECISION_BITS ///right shift 10
    };
    uint32_t fracCoord[2] = {
        mpShelf->mFrequency << (32 - FREQ_PRECISION_BITS), ///left shift 6
        (uint32_t)(mpShelf->mGain) << (32 - GAIN_PRECISION_BITS) ///left shift 22
    };
    if (mpShelf->mType == kHighShelf) {
        AudioCoefInterpolator_GetCoef(&(mpShelf->mHiCoefInterp), intCoord, fracCoord, coefs);
    } else {
        AudioCoefInterpolator_GetCoef(&(mpShelf->mLoCoefInterp), intCoord, fracCoord, coefs);
    }
	AudioBiquadSetCoefs(&(mpShelf->mBiquad), coefs, immediate);
}

