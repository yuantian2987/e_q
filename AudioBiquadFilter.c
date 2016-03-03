/* //device/servers/AudioFlinger/AudioBiquadFilter.cpp
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

#include <string.h>
#include <assert.h>
#include "AudioBiquadFilter.h"

const audio_coef_t IDENTITY_COEFS[NUM_COEFS] = { AUDIO_COEF_ONE, 0, 0, 0, 0 };

static void setState(AudioBiquadFilter *mBiquad, state_t state);
static bool updateCoefs(AudioBiquadFilter *mBiquad, const audio_coef_t coefs[NUM_COEFS], int frameCount);
static void process_normal_mono(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx);
static void process_normal_multi(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx);

static void process_bypass(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx) {
    // The common case is in-place processing, because this is what the EQ does.
    if (CC_UNLIKELY(in != out)) {
        memcpy(out, in, frameCount * mBiquad->mNumChannels * sizeof(audio_sample_t));
    }
}

static void process_transition_bypass_mono(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx)  {

  if (updateCoefs(mBiquad, IDENTITY_COEFS, frameCount)) {
      setState(mBiquad, STATE_NORMAL);
  }
  process_normal_mono(mBiquad, in, out, frameCount, indx);
}

static void process_transition_bypass_multi(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx)  {
	
    if (updateCoefs(mBiquad, IDENTITY_COEFS, frameCount)) {
        setState(mBiquad, STATE_NORMAL);
    }
    process_normal_multi(mBiquad, in, out, frameCount, indx);
}

static void process_transition_normal_mono(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx) {
	
    if (updateCoefs(mBiquad, mBiquad->mTargetCoefs, frameCount)) {
        setState(mBiquad, STATE_NORMAL);
    }
    process_normal_mono(mBiquad, in, out, frameCount, indx);
}

static void process_transition_normal_multi(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx) {
	
    if (updateCoefs(mBiquad, mBiquad->mTargetCoefs, frameCount)) {
        setState(mBiquad, STATE_NORMAL);
    }
    process_normal_multi(mBiquad, in, out, frameCount, indx);
}

static void setState(AudioBiquadFilter *mBiquad, state_t state) {
    switch (state) {
    case STATE_BYPASS:
      mCurProcessFunc = &process_bypass;
      break;
    case STATE_TRANSITION_TO_BYPASS:
      if (mBiquad->mNumChannels == 1) {
        mCurProcessFunc = &process_transition_bypass_mono;
      } else {
        mCurProcessFunc = &process_transition_bypass_multi;
      }
      mBiquad->mCoefDirtyBits = (1 << NUM_COEFS) - 1;
      break;
    case STATE_TRANSITION_TO_NORMAL:
      if (mBiquad->mNumChannels == 1) {
        mCurProcessFunc = &process_transition_normal_mono;
      } else {
        mCurProcessFunc = &process_transition_normal_multi;
      }
      mBiquad->mCoefDirtyBits = (1 << NUM_COEFS) - 1;
      break;
    case STATE_NORMAL:
      if (mBiquad->mNumChannels == 1) {
        mCurProcessFunc = &process_normal_mono;
      } else {
        mCurProcessFunc = &process_normal_multi;
      }
      break;
    }
    mBiquad->mState = state;
}

static bool updateCoefs(AudioBiquadFilter *mBiquad, const audio_coef_t coefs[NUM_COEFS], int frameCount) {
    int i = 0;
	int64_t maxDelta = mBiquad->mMaxDelta * frameCount;
    for (i = 0; i < NUM_COEFS; ++i) {
        if (mBiquad->mCoefDirtyBits & (1<<i)) {
            audio_coef_t diff = coefs[i] - mBiquad->mCoefs[i];
            if (diff > maxDelta) {
                mBiquad->mCoefs[i] += maxDelta;
            } else if (diff < -maxDelta) {
                mBiquad->mCoefs[i] -= maxDelta;
            } else {
                mBiquad->mCoefs[i] = coefs[i];
                mBiquad->mCoefDirtyBits ^= (1<<i);
            }
        }
    }
    return mBiquad->mCoefDirtyBits == 0;
}

static void process_normal_mono(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx) {

	size_t nFrames = frameCount;
    audio_sample_t x1 = mBiquad->mDelays[indx][0];
    audio_sample_t x2 = mBiquad->mDelays[indx][1];
    audio_sample_t y1 = mBiquad->mDelays[indx][2];
    audio_sample_t y2 = mBiquad->mDelays[indx][3];
    const audio_coef_t b0 = mBiquad->mCoefs[0];//left shift 24
    const audio_coef_t b1 = mBiquad->mCoefs[1];
    const audio_coef_t b2 = mBiquad->mCoefs[2];
    const audio_coef_t a1 = mBiquad->mCoefs[3];
    const audio_coef_t a2 = mBiquad->mCoefs[4];
	audio_coef_sample_acc_t acc;
	audio_sample_t y0;
    while (nFrames-- > 0) {
        audio_sample_t x0 = *(in++);//left shift 9 than the original in data
        acc = mul_coef_sample(b0, x0);
        acc = mac_coef_sample(b1, x1, acc);
        acc = mac_coef_sample(b2, x2, acc);
        acc = mac_coef_sample(a1, y1, acc);
        acc = mac_coef_sample(a2, y2, acc);//acc is 18(9+9) left shift 
        y0 = coef_sample_acc_to_sample(acc);//right shift 24, 33-24 = 9, now the y0 is 9 left shift than original in data
        y2 = y1;
        y1 = y0;
        x2 = x1;
        x1 = x0;
        (*out++) = y0;
    }
    mBiquad->mDelays[indx][0] = x1;//left shift 9
    mBiquad->mDelays[indx][1] = x2;//left shift 9
    mBiquad->mDelays[indx][2] = y1;//left shift 9
    mBiquad->mDelays[indx][3] = y2;//left shift 9
}


static void process_normal_multi(AudioBiquadFilter *mBiquad, 
	const audio_sample_t * in, audio_sample_t * out, int frameCount, effect_sound_track indx) {
	
	int ch = 0;
    const audio_coef_t b0 = mBiquad->mCoefs[0];
    const audio_coef_t b1 = mBiquad->mCoefs[1];
    const audio_coef_t b2 = mBiquad->mCoefs[2];
    const audio_coef_t a1 = mBiquad->mCoefs[3];
    const audio_coef_t a2 = mBiquad->mCoefs[4];
	audio_sample_t y0;
    for (ch = 0; ch < mBiquad->mNumChannels; ++ch) {
        size_t nFrames = frameCount;
        audio_sample_t x1 = mBiquad->mDelays[ch][0];
        audio_sample_t x2 = mBiquad->mDelays[ch][1];
        audio_sample_t y1 = mBiquad->mDelays[ch][2];
        audio_sample_t y2 = mBiquad->mDelays[ch][3];
        while (nFrames-- > 0) {
            audio_sample_t x0 = *in;//left shift 9 than the original in data
            audio_coef_sample_acc_t acc;
            acc = mul_coef_sample(b0, x0);
            acc = mac_coef_sample(b1, x1, acc);
            acc = mac_coef_sample(b2, x2, acc);
            acc = mac_coef_sample(a1, y1, acc);
            acc = mac_coef_sample(a2, y2, acc);//acc is 18(9+9) left shift 
            y0 = coef_sample_acc_to_sample(acc);//right shift 24, 42-24 = 18, now the y0 is 18(9+9) left shift than original in data
            y2 = y1;
            y1 = y0;
            x2 = x1;
            x1 = x0;
            *out = y0;
            in += mBiquad->mNumChannels;
            out += mBiquad->mNumChannels;
        }
        mBiquad->mDelays[ch][0] = x1;
        mBiquad->mDelays[ch][1] = x2;
        mBiquad->mDelays[ch][2] = y1;
        mBiquad->mDelays[ch][3] = y2;
        ///in -= frameCount * mBiquad->mNumChannels - 1;
        ///out -= frameCount * mBiquad->mNumChannels - 1;
    }
}

void _AudioBiquadFilter(AudioBiquadFilter *mBiquad, int nChannels, int sampleRate) {
	AudioBiquadConfigure(mBiquad, nChannels, sampleRate);///
    AudioBiquadReset(mBiquad);///
}

void AudioBiquadConfigure(AudioBiquadFilter *mBiquad, int nChannels, int sampleRate) {
    assert(nChannels > 0 && nChannels <= MAX_CHANNELS);
    assert(sampleRate > 0);
    mBiquad->mNumChannels  = nChannels;
    mBiquad->mMaxDelta = (int64_t)(MAX_DELTA_PER_SEC) * AUDIO_COEF_ONE / sampleRate;
	AudioBiquadClear(mBiquad);///
}

void AudioBiquadReset(AudioBiquadFilter *mBiquad) {
    memcpy(mBiquad->mCoefs, IDENTITY_COEFS, sizeof(mBiquad->mCoefs));
    mBiquad->mCoefDirtyBits = 0;
    setState(mBiquad, STATE_BYPASS);
}

void AudioBiquadClear(AudioBiquadFilter *mBiquad) {
    memset(mBiquad->mDelays, 0, sizeof(mBiquad->mDelays));
}

void AudioBiquadSetCoefs(AudioBiquadFilter *mBiquad, const audio_coef_t *coefs, bool immediate) {
    memcpy(mBiquad->mTargetCoefs, coefs, sizeof(mBiquad->mTargetCoefs));
    if (mBiquad->mState & STATE_ENABLED_MASK) {
        if (CC_UNLIKELY(immediate)) {
            memcpy(mBiquad->mCoefs, coefs, sizeof(mBiquad->mCoefs));
            setState(mBiquad, STATE_NORMAL);
        } else {
            setState(mBiquad, STATE_TRANSITION_TO_NORMAL);
        }
    }
}

void AudioBiquadProcess(AudioBiquadFilter *mBiquad, 
	const audio_sample_t *pIn, audio_sample_t *pOut, int frameCount, effect_sound_track indx) {
    mCurProcessFunc(mBiquad, pIn, pOut, frameCount, indx);///??
}

void AudioBiquadEnable(AudioBiquadFilter *mBiquad, bool immediate) {
    if (CC_UNLIKELY(immediate)) {
        memcpy(mBiquad->mCoefs, mBiquad->mTargetCoefs, sizeof(mBiquad->mCoefs));
        setState(mBiquad, STATE_NORMAL);
    } else {
        setState(mBiquad, STATE_TRANSITION_TO_NORMAL);
    }
}

void AudioBiquadDisable(AudioBiquadFilter *mBiquad, bool immediate) {
    if (CC_UNLIKELY(immediate)) {
        memcpy(mBiquad->mCoefs, IDENTITY_COEFS, sizeof(mBiquad->mCoefs));
        setState(mBiquad, STATE_BYPASS);
    } else {
        setState(mBiquad, STATE_TRANSITION_TO_BYPASS);
    }
}



