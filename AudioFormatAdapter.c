#include "AudioFormatAdapter.h"
#include <string.h>
#include <assert.h>

static void ConvertInput(AudioFormatAdapter *pFormatAdapter, const int16_t *pIn, uint32_t numSamples);
static void ConvertOutput(AudioFormatAdapter *pFormatAdapter, int16_t *pOut, uint32_t numSamples);

void AudioFormatAdapterConfigure(AudioFormatAdapter *pFormatAdapter, AUDIO_EQUALIZER * pEqualizer, 
				int nChannels, uint8_t pcmFormat, uint32_t behavior) {
	pFormatAdapter->mpProcessor = pEqualizer;
	pFormatAdapter->mNumChannels = nChannels;
	pFormatAdapter->mPcmFormat = pcmFormat;
	pFormatAdapter->mBehavior = behavior;
	pFormatAdapter->mMaxSamplesPerCall = BUFFER_SIZE / nChannels;
}

void AudioFormatAdapterFree(AudioFormatAdapter *pFormatAdapter) {
    if (pFormatAdapter != NULL) {
        ///free(pFormatAdapter);
		pFormatAdapter = NULL;
    }
}

void AudioFormatAdapterProcess(AudioFormatAdapter *pFormatAdapter, 
	const int16_t * pIn, int16_t * pOut, uint32_t numSamples, effect_sound_track indx) {

	while (numSamples > 0) {
        uint32_t numSamplesIter = min(numSamples, pFormatAdapter->mMaxSamplesPerCall);/// 2048 , 2048
        uint32_t nSamplesChannels = numSamplesIter * pFormatAdapter->mNumChannels;///mNumChannels = 1
        if (pFormatAdapter->mPcmFormat == AUDIO_FORMAT_PCM_16_BIT) {
            ConvertInput(pFormatAdapter, pIn, nSamplesChannels);///left shift 9
            AudioEqualizerProcess(pFormatAdapter->mpProcessor, pFormatAdapter->mBuffer, pFormatAdapter->mBuffer, numSamplesIter, indx);
            ConvertOutput(pFormatAdapter, pOut, nSamplesChannels);///right shift 9
        }
        numSamples -= numSamplesIter;
    }
}

static void ConvertInput(AudioFormatAdapter *pFormatAdapter, const int16_t *pIn, uint32_t numSamples) {
	if (pFormatAdapter->mPcmFormat == AUDIO_FORMAT_PCM_16_BIT) {
		const int16_t * pIn16 = pIn;
		audio_sample_t * pOut = pFormatAdapter->mBuffer;
		while (numSamples-- > 0) {
			*(pOut++) = s15_to_audio_sample_t(*(pIn16++));///left shift 9 bit for *(pIn16++)
		}
	} else {///AUDIO_FORMAT_PCM_8_24_BIT
		assert(false);
	}
}

static void ConvertOutput(AudioFormatAdapter *pFormatAdapter, int16_t *pOut, uint32_t numSamples) {
	if (pFormatAdapter->mPcmFormat == AUDIO_FORMAT_PCM_16_BIT) {
		const audio_sample_t * pIn = pFormatAdapter->mBuffer;
		int16_t * pOut16 = pOut;
		if (pFormatAdapter->mBehavior == EFFECT_BUFFER_ACCESS_WRITE) {
			while (numSamples-- > 0) {
				*(pOut16++) = audio_sample_t_to_s15_clip(*(pIn++));///right shift 9 bit for *(pIn16++)
			}
		} else if (pFormatAdapter->mBehavior == EFFECT_BUFFER_ACCESS_ACCUMULATE) {
			while (numSamples-- > 0) {
				*(pOut16++) += audio_sample_t_to_s15_clip(*(pIn++));///right shift 9 bit for *(pIn16++)
			}
		} else {
			assert(false);
		}
	} else {
		assert(false);
	}
}


