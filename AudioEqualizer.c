/*
 * Copyright 2009, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AudioEqualizer"

#include <assert.h>
#include <stdlib.h>
#include "AudioEqualizer.h"
#include "AudioPeakingFilter.h"
#include "AudioShelvingFilter.h"
#include "EffectsMath.h"

void AudioEqualizerReset(AUDIO_EQUALIZER * pEqualizer);
void AudioEqualizerCommit(AUDIO_EQUALIZER *pEqualizer, bool immediate);

void _AudioEqualizer(AUDIO_EQUALIZER * pEqualizer, 
			int32_t bandsNum, 
			int nChannels, 
			int sampleRate, 
			const PRESET_CONFIG * presets, 
			int32_t mNumPresets) {
			
    int32_t i = 0;
	pEqualizer->mSampleRate = sampleRate;
	pEqualizer->mNumPeaking = bandsNum - 2;
	pEqualizer->mpPresets = presets;
	pEqualizer->mNumPresets = mNumPresets; 
	_AudioShelvingFilter(&(pEqualizer->mpLowShelf), kLowShelf, nChannels, sampleRate);
	for(i=0; i<pEqualizer->mNumPeaking; i++) {
	    _AudioPeakingFilter(&(pEqualizer->mpPeakingFilters[i]), nChannels, sampleRate);
	}
	_AudioShelvingFilter(&(pEqualizer->mpHighShelf), kHighShelf, nChannels, sampleRate);
	AudioEqualizerReset(pEqualizer);
}

void AudioEqualizerConfigure(AUDIO_EQUALIZER * pEqualizer, int nChannels, int sampleRate) {
	int i = 0;
    AudioShelvingConfigure(&(pEqualizer->mpLowShelf), nChannels, sampleRate);///low
    for (i = 0; i < pEqualizer->mNumPeaking; ++i) {
        AudioPeakingConfigure(&(pEqualizer->mpPeakingFilters[i]), nChannels, sampleRate);///peaking
    }
    AudioShelvingConfigure(&(pEqualizer->mpHighShelf), nChannels, sampleRate);///high
}

void AudioEqualizerClear(AUDIO_EQUALIZER * pEqualizer) {
    int i = 0;
	AudioShelvingClear(&(pEqualizer->mpLowShelf));///low
    for (i = 0; i < pEqualizer->mNumPeaking; ++i) {
        AudioPeakingClear(&(pEqualizer->mpPeakingFilters[i]));///peaking
    }
    AudioShelvingClear(&(pEqualizer->mpHighShelf));///high
}

void AudioEqualizerFree(AUDIO_EQUALIZER * pEqualizer) {
    if (pEqualizer != NULL) {
        ///free(pEqualizer);
		pEqualizer = NULL;
    }
}

void AudioEqualizerReset(AUDIO_EQUALIZER *pEqualizer) {
	int i = 0;
    const int32_t bottom = Effects_log2(kMinFreq);
    const int32_t top = Effects_log2(pEqualizer->mSampleRate * 500);
    const int32_t jump = (top - bottom) / (pEqualizer->mNumPeaking + 2);
    int32_t centerFreq = bottom + jump/2;

    AudioShelvingReset(&(pEqualizer->mpLowShelf));
    AudioShelvingSetFrequency(&(pEqualizer->mpLowShelf), Effects_exp2(centerFreq));///low
    centerFreq += jump;
    for (i = 0; i < pEqualizer->mNumPeaking; ++i) {
        AudioPeakingReset(&(pEqualizer->mpPeakingFilters[i]));
        AudioPeakingSetFrequency(&(pEqualizer->mpPeakingFilters[i]), Effects_exp2(centerFreq));///peaking
        centerFreq += jump;
    }
    AudioShelvingReset(&(pEqualizer->mpHighShelf));
    AudioShelvingSetFrequency(&(pEqualizer->mpHighShelf), Effects_exp2(centerFreq));///high
	AudioEqualizerCommit(pEqualizer, true);///
    pEqualizer->mCurPreset = PRESET_CUSTOM;
}

void AudioEqualizerSetGain(AUDIO_EQUALIZER * pEqualizer, int band, int32_t millibel) {
    assert(band >= 0 && band < pEqualizer->mNumPeaking + 2);
    if (band == 0) {
        AudioShelvingSetGain(&(pEqualizer->mpLowShelf), millibel);///low
    } else if (band == pEqualizer->mNumPeaking + 1) {
        AudioShelvingSetGain(&(pEqualizer->mpHighShelf), millibel);///high
    } else {
        AudioPeakingSetGain(&(pEqualizer->mpPeakingFilters[band - 1]), millibel);///peaking
    }
    pEqualizer->mCurPreset = PRESET_CUSTOM;
}

void AudioEqualizerSetFrequency(AUDIO_EQUALIZER * pEqualizer, int band, uint32_t millihertz) {
    assert(band >= 0 && band < pEqualizer->mNumPeaking + 2);
    if (band == 0) {
        AudioShelvingSetFrequency(&(pEqualizer->mpLowShelf), millihertz);///low
    } else if (band == pEqualizer->mNumPeaking + 1) {
        AudioShelvingSetFrequency(&(pEqualizer->mpHighShelf), millihertz);///high
    } else {
        AudioPeakingSetFrequency(&(pEqualizer->mpPeakingFilters[band - 1]), millihertz);///peaking
    }
    pEqualizer->mCurPreset = PRESET_CUSTOM;
}

void AudioEqualizerSetBandwidth(AUDIO_EQUALIZER * pEqualizer, int band, uint32_t cents) {
    assert(band >= 0 && band < pEqualizer->mNumPeaking + 2);
    if (band > 0 && band < pEqualizer->mNumPeaking + 1) {
        AudioPeakingSetBandwidth(&(pEqualizer->mpPeakingFilters[band - 1]), cents);///peaking
        pEqualizer->mCurPreset = PRESET_CUSTOM;
    }
}

int32_t AudioEqualizerGetGain(AUDIO_EQUALIZER * pEqualizer, int band) {
    assert(band >= 0 && band < pEqualizer->mNumPeaking + 2);
    if (band == 0) {
        return AudioShelvingGetGain(&(pEqualizer->mpLowShelf));///low
    } else if (band == pEqualizer->mNumPeaking + 1) {
        return AudioShelvingGetGain(&(pEqualizer->mpHighShelf));///high
    } else {
        return AudioPeakingGetGain(&(pEqualizer->mpPeakingFilters[band - 1]));///peaking
    }
}

uint32_t AudioEqualizerGetFrequency(AUDIO_EQUALIZER * pEqualizer, int band) {
    assert(band >= 0 && band < pEqualizer->mNumPeaking + 2);
    if (band == 0) {
        return AudioShelvingGetFrequency(&(pEqualizer->mpLowShelf));///low
    } else if (band == pEqualizer->mNumPeaking + 1) {
        return AudioShelvingGetFrequency(&(pEqualizer->mpHighShelf));///high
    } else {
        return AudioPeakingGetFrequency(&(pEqualizer->mpPeakingFilters[band - 1]));///peaking
    }
}

uint32_t AudioEqualizerGetBandwidth(AUDIO_EQUALIZER * pEqualizer, int band) {
    assert(band >= 0 && band < pEqualizer->mNumPeaking + 2);
    if (band == 0 || band == pEqualizer->mNumPeaking + 1) {
        return 0;
    } else {
        return AudioPeakingGetBandwidth(&(pEqualizer->mpPeakingFilters[band - 1]));///peaking
    }
}

void AudioEqualizerGetBandRange(AUDIO_EQUALIZER * pEqualizer, int band, uint32_t *pLow,
                                  uint32_t *pHigh) {
    assert(band >= 0 && band < pEqualizer->mNumPeaking + 2);
    if (band == 0) {
        *pLow = 0;
        *pHigh = AudioShelvingGetFrequency(&(pEqualizer->mpLowShelf));///low
    } else if (band == pEqualizer->mNumPeaking + 1) {
        *pLow = AudioShelvingGetFrequency(&(pEqualizer->mpHighShelf));///high
        *pHigh = pEqualizer->mSampleRate * 500;
    } else {
        AudioPeakingGetBandRange(&(pEqualizer->mpPeakingFilters[band - 1]), pLow, pHigh);///peaking
    }
}

const char * AudioEqualizerGetPresetName(AUDIO_EQUALIZER * pEqualizer, int preset) {
    assert(preset < pEqualizer->mNumPresets && preset >= PRESET_CUSTOM);
    if (preset == PRESET_CUSTOM) {
        return "Custom";
    } else {
        return pEqualizer->mpPresets[preset].name;
    }
}

int AudioEqualizerGetNumPresets(AUDIO_EQUALIZER * pEqualizer) {
    return pEqualizer->mNumPresets;
}

int AudioEqualizerGetPreset(AUDIO_EQUALIZER * pEqualizer) {
    return pEqualizer->mCurPreset;
}

void AudioEqualizerSetPreset(AUDIO_EQUALIZER * pEqualizer, int preset) {
	int band = 0;
	PRESET_CONFIG presetCfg;
	BAND_CONFIG bandCfg;
    assert(preset < pEqualizer->mNumPresets && preset >= 0);
    presetCfg = pEqualizer->mpPresets[preset];
    for (band = 0; band < (pEqualizer->mNumPeaking + 2); ++band) {
        bandCfg = presetCfg.bandConfigs[band];
		AudioEqualizerSetGain(pEqualizer, band, bandCfg.gain);///
		AudioEqualizerSetFrequency(pEqualizer, band, bandCfg.freq);///
		AudioEqualizerSetBandwidth(pEqualizer, band, bandCfg.bandwidth);///
    }
    pEqualizer->mCurPreset = preset;
}

void AudioEqualizerCommit(AUDIO_EQUALIZER *pEqualizer, bool immediate) {
	int i = 0;
    AudioShelvingCommit(&(pEqualizer->mpLowShelf), immediate);///low
    for (i = 0; i < pEqualizer->mNumPeaking; ++i) {
        AudioPeakingCommit(&(pEqualizer->mpPeakingFilters[i]), immediate);///peaking
    }
    AudioShelvingCommit(&(pEqualizer->mpHighShelf), immediate);///high
}

void AudioEqualizerProcess(AUDIO_EQUALIZER * pEqualizer, 
	const audio_sample_t * pIn, audio_sample_t * pOut, int frameCount, effect_sound_track indx) {

	int i = 0;
    AudioShelvingProcess(&(pEqualizer->mpLowShelf), pIn, pOut, frameCount, indx);///low
    for (i = 0; i < pEqualizer->mNumPeaking; ++i) {
        AudioPeakingProcess(&(pEqualizer->mpPeakingFilters[i]), pIn, pOut, frameCount, indx);///peaking
    }
    AudioShelvingProcess(&(pEqualizer->mpHighShelf), pIn, pOut, frameCount, indx);///high
}

void AudioEqualizerEnable(AUDIO_EQUALIZER * pEqualizer, bool immediate) {
	int i = 0;
    AudioShelvingEnable(&(pEqualizer->mpLowShelf), immediate);///low
    for (i = 0; i < pEqualizer->mNumPeaking; ++i) {
        AudioPeakingEnable(&(pEqualizer->mpPeakingFilters[i]), immediate);///peaking
    }
    AudioShelvingEnable(&(pEqualizer->mpHighShelf), immediate);///high
}

void AudioEqualizerDisable(AUDIO_EQUALIZER * pEqualizer, bool immediate) {
	int i = 0;
    AudioShelvingDisable(&(pEqualizer->mpLowShelf), immediate);///low
    for (i = 0; i < pEqualizer->mNumPeaking; ++i) {
        AudioPeakingDisable(&(pEqualizer->mpPeakingFilters[i]), immediate);///peaking
    }
    AudioShelvingDisable(&(pEqualizer->mpHighShelf), immediate);///high
}

int AudioEqualizerGetMostRelevantBand(AUDIO_EQUALIZER * pEqualizer, uint32_t targetFreq) {
    // First, find the two bands that the target frequency is between.
	uint32_t low, high, freq;
	int band, i;

    low = AudioShelvingGetFrequency(&(pEqualizer->mpLowShelf));///low
    if (targetFreq <= low) {
        return 0;
    }
    high = AudioShelvingGetFrequency(&(pEqualizer->mpHighShelf));///high
    if (targetFreq >= high) {
        return pEqualizer->mNumPeaking + 1;
    }
    band = pEqualizer->mNumPeaking;
    for (i = 0; i < pEqualizer->mNumPeaking; ++i) {
        freq = AudioPeakingGetFrequency(&(pEqualizer->mpPeakingFilters[i]));///peaking
        if (freq >= targetFreq) {
            high = freq;
            band = i;
            break;
        }
        low = freq;
    }
    // Now, low is right below the target and high is right above. See which one
    // is closer on a log scale.
    low = Effects_log2(low);
    high = Effects_log2(high);
    targetFreq = Effects_log2(targetFreq);
    if (high - targetFreq < targetFreq - low) {
        return band + 1;
    } else {
        return band;
    }
}

