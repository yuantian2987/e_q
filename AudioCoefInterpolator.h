/* //device/include/server/AudioFlinger/AudioCoefInterpolator.h
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

#ifndef ANDROID_AUDIO_COEF_INTERPOLATOR_H
#define ANDROID_AUDIO_COEF_INTERPOLATOR_H

#include "AudioCommon.h"

// A helper class for linear interpolation of N-D -> M-D coefficient tables.
// This class provides support for out-of-range indexes.
// Details:
// The purpose is efficient approximation of a N-dimensional vector to
// M-dimensional function. The approximation is based on a table of output
// values on a uniform grid of the input values. Values not on the grid are
// linearly interpolated.
// Access to values are done by specifying input values in table index units,
// having an integer and a fractional part, e.g. retrieving a value from index
// 1.4 will result in linear interpolation between index 1 and index 2.

// Maximum allowed number of input dimensions.
///const size_t MAX_IN_DIMS = 8;
#define MAX_IN_DIMS  (8)
// Maximum allowed number of output dimensions.
///const size_t MAX_OUT_DIMS = 8;
#define MAX_OUT_DIMS  (8)

typedef struct _AudioCoefInterpolator_ {

    // Number of input dimensions.
    size_t mNumInDims;
    // Number of input dimensions.
    size_t mInDims[MAX_IN_DIMS];
    // The offset between two consecutive indexes of each dimension. This is in
    // fact a cumulative product of mInDims (done in reverse).
    size_t mInDimOffsets[MAX_IN_DIMS];
    // Number of output dimensions.
    size_t mNumOutDims;
    // The coefficient table.
    const audio_coef_t * mTable;

}AudioCoefInterpolator;

void _AudioCoefInterpolator(AudioCoefInterpolator *mCoefInterp, size_t nInDims,
                                             const size_t inDims[],
                                             size_t nOutDims,
                                             const audio_coef_t * table);

void AudioCoefInterpolator_GetCoef(AudioCoefInterpolator *mCoefInterp, int intCoord[], uint32_t fracCoord[],
                                    audio_coef_t out[]);


#endif // ANDROID_AUDIO_COEF_INTERPOLATOR_H
