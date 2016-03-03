/* //device/servers/AudioFlinger/AudioCoefInterpolator.cpp
 **
 ** Copyright 2008, The Android Open Source Project
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
#include "AudioCoefInterpolator.h"

audio_coef_t interp(audio_coef_t lo, audio_coef_t hi, uint32_t frac);
static void getCoefRecurse(AudioCoefInterpolator *mCoefInterp, size_t index,
                                           const uint32_t fracCoord[],
                                           audio_coef_t out[], size_t dim);

void _AudioCoefInterpolator(AudioCoefInterpolator *mCoefInterp, size_t nInDims,
                                             const size_t inDims[],
                                             size_t nOutDims,
                                             const audio_coef_t * table) {
	size_t dim = 0;
    mCoefInterp->mNumInDims = nInDims;
    memcpy(mCoefInterp->mInDims, inDims, nInDims * sizeof(size_t));
    mCoefInterp->mNumOutDims = nOutDims;
    mCoefInterp->mTable = table;
    // Initialize offsets array
    dim = nInDims - 1;
    mCoefInterp->mInDimOffsets[nInDims - 1] = nOutDims;
    while (dim-- > 0) {
        mCoefInterp->mInDimOffsets[dim] = mCoefInterp->mInDimOffsets[dim + 1] * mCoefInterp->mInDims[dim + 1];
    }
}

void AudioCoefInterpolator_GetCoef(AudioCoefInterpolator *mCoefInterp, int intCoord[], uint32_t fracCoord[],
                                    audio_coef_t out[]) {
    size_t index = 0;
    size_t dim = mCoefInterp->mNumInDims;
    while (dim-- > 0) {
        if (CC_UNLIKELY(intCoord[dim] < 0)) {
            fracCoord[dim] = 0;
        } else if (CC_UNLIKELY(intCoord[dim] >= (int)mCoefInterp->mInDims[dim] - 1)) {
            fracCoord[dim] = 0;
            index += mCoefInterp->mInDimOffsets[dim] * (mCoefInterp->mInDims[dim] - 1);
        } else {
            index += mCoefInterp->mInDimOffsets[dim] * intCoord[dim];
        }
    }
    getCoefRecurse(mCoefInterp, index, fracCoord, out, 0);
}

static void getCoefRecurse(AudioCoefInterpolator *mCoefInterp, size_t index,
                                           const uint32_t fracCoord[],
                                           audio_coef_t out[], size_t dim) {
	size_t d = 0;
    if (dim == mCoefInterp->mNumInDims) {
        memcpy(out, mCoefInterp->mTable + index, mCoefInterp->mNumOutDims * sizeof(audio_coef_t));
    } else {
        getCoefRecurse(mCoefInterp, index, fracCoord, out, dim + 1);
        if (CC_LIKELY(fracCoord != 0)) {
           audio_coef_t tempCoef[MAX_OUT_DIMS];
           getCoefRecurse(mCoefInterp, index + mCoefInterp->mInDimOffsets[dim], fracCoord, tempCoef, dim + 1);
            d = mCoefInterp->mNumOutDims;
            while (d-- > 0) {
                out[d] = interp(out[d], tempCoef[d], fracCoord[dim]);
            }
        }
    }
}

audio_coef_t interp(audio_coef_t lo, audio_coef_t hi, uint32_t frac) {
    int64_t delta = (int64_t)(hi-lo) * frac;
    return lo + (audio_coef_t)(delta >> 32);
}

