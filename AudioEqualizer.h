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

#ifndef AUDIOEQUALIZER_H_
#define AUDIOEQUALIZER_H_

#include "AudioCommon.h"
#include "AudioShelvingFilter.h"
#include "AudioPeakingFilter.h"

// A parametric audio equalizer. Supports an arbitrary number of bands and
// presets.
// The EQ is composed of a low-shelf, zero or more peaking filters and a high
// shelf, where each band has frequency and gain controls, and the peaking
// filters have an additional bandwidth control.

// Bottom frequency, in mHz.
///static const int kMinFreq = 20000;
#define kMinFreq (20000)
// This value is used when requesting current preset, and EQ is not using a
// preset.
///static const int PRESET_CUSTOM = -1;
#define PRESET_CUSTOM  (-1)

typedef struct _BAND_CONFIG_ {
	// Gain in millibel.
	int32_t gain;
	// Frequency in millihertz.
	uint32_t freq;
	// Bandwidth in cents (ignored on shelving filters).
	uint32_t bandwidth;
}BAND_CONFIG;

// Preset configuration.
typedef struct _PRESET_CONFIG_ {
	// Human-readable name.
	const char * name;
	// An array of size nBands where each element is a configuration for the
	// corresponding band.
	const BAND_CONFIG * bandConfigs;
}PRESET_CONFIG;

typedef struct  _AUDIO_EQUALIZER_{
    // Configuration of a single band.
	BAND_CONFIG BandConfig;
	PRESET_CONFIG PresetConfig;

    // Sample rate, in Hz.
    int mSampleRate;
    // Number of peaking filters. Total number of bands is +2.
    int mNumPeaking;
    // Preset configurations.
    const PRESET_CONFIG * mpPresets;
    // Number of elements in mpPresets;
    int mNumPresets;
    // Current preset.
    int mCurPreset;

    // The low-shelving filter.
    AudioShelvingFilter mpLowShelf;
    // The high-shelving filter.
    AudioShelvingFilter mpHighShelf;
    // An array of size mNumPeaking of peaking filters.
    AudioPeakingFilter mpPeakingFilters[3];

}AUDIO_EQUALIZER;

void AudioEqualizerProcess(AUDIO_EQUALIZER * pEqualizer, 
	const audio_sample_t * pIn, audio_sample_t * pOut, int frameCount, effect_sound_track indx);

#endif // AUDIOEQUALIZER_H_

