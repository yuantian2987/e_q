/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "AudioEqualizer.h"
#include "AudioBiquadFilter.h"
#include "AudioFormatAdapter.h"
#include "user_errno.h"

enum equalizer_state_e {
    EQUALIZER_STATE_UNINITIALIZED,
    EQUALIZER_STATE_INITIALIZED,
    EQUALIZER_STATE_ACTIVE,
};

// Google Graphic Equalizer UUID: e25aa840-543b-11df-98a5-0002a5d5c51b
const effect_descriptor_t gEqualizerDescriptor = {
        {0x0bed4300, 0xddd6, 0x11db, 0x8f34, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // type
        {0xe25aa840, 0x543b, 0x11df, 0x98a5, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_INSERT_LAST),
        0, // TODO
        1,
        "Graphic Equalizer",
        "The Android Open Source Project",
};

/////////////////// BEGIN EQ PRESETS ///////////////////////////////////////////
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define FREQ_0  (50000)
#define FREQ_1  (125000)
#define FREQ_2  (900000)
#define FREQ_3  (3200000)
#define FREQ_4  (6300000)

#define BAND_WIDTH_0  (0)
#define BAND_WIDTH_1  (3600)
#define BAND_WIDTH_2  (3600)
#define BAND_WIDTH_3  (2400)
#define BAND_WIDTH_4  (0)

uint32_t gFreqs[kNumBands] =      { 50000, 125000, 900000, 3200000, 6300000 };
uint32_t gBandwidths[kNumBands] = { 0,     3600,   3600,   2400,    0       };

BAND_CONFIG gBandsNormal[kNumBands] = {
    { 0,  FREQ_0, BAND_WIDTH_0 },
    { 0,  FREQ_1, BAND_WIDTH_1 },
    { 0,  FREQ_2, BAND_WIDTH_2 },
    { 0,  FREQ_3, BAND_WIDTH_3 },
    { 0,  FREQ_4, BAND_WIDTH_4 }
};

BAND_CONFIG gBandsClassic[kNumBands] = {
    { 300,  FREQ_0, BAND_WIDTH_0 },
    { 400,  FREQ_1, BAND_WIDTH_1 },
    { 0,    FREQ_2, BAND_WIDTH_2 },
    { 200,  FREQ_3, BAND_WIDTH_3 },
    { -300, FREQ_4, BAND_WIDTH_4 }
};

BAND_CONFIG gBandsJazz[kNumBands] = {
    { -600, FREQ_0, BAND_WIDTH_0 },
    { 200,  FREQ_1, BAND_WIDTH_1 },
    { 400,  FREQ_2, BAND_WIDTH_2 },
    { -400, FREQ_3, BAND_WIDTH_3 },
    { -600, FREQ_4, BAND_WIDTH_4 }
};

BAND_CONFIG gBandsPop[kNumBands] = {
    { 400,  FREQ_0, BAND_WIDTH_0 },
    { -400, FREQ_1, BAND_WIDTH_1 },
    { 300,  FREQ_2, BAND_WIDTH_2 },
    { -400, FREQ_3, BAND_WIDTH_3 },
    { 600,  FREQ_4, BAND_WIDTH_4 }
};

BAND_CONFIG gBandsRock[kNumBands] = {
    { 700,  FREQ_0, BAND_WIDTH_0 },
    { 400,  FREQ_1, BAND_WIDTH_1 },
    { -400, FREQ_2, BAND_WIDTH_2 },
    { 400,  FREQ_3, BAND_WIDTH_3 },
    { 200,  FREQ_4, BAND_WIDTH_4 }
};

PRESET_CONFIG gEqualizerPresets[5] = {
    { "Normal",  gBandsNormal  },
    { "Classic", gBandsClassic },
    { "Jazz",    gBandsJazz    },
    { "Pop",     gBandsPop     },
    { "Rock",    gBandsRock    }
};

/////////////////// END EQ PRESETS /////////////////////////////////////////////

typedef struct _EqualizerContext_ {
    effect_config_t config;
    AudioFormatAdapter *pAdapter;
    AUDIO_EQUALIZER * pEqualizer;
    uint32_t state;
}EqualizerContext;

EqualizerContext gContext;
AUDIO_EQUALIZER gEqualizer;
AudioFormatAdapter gAdapter;

AUDIO_EQ_CONFIG gConfig;
AUDIO_EQ_CONFIG *pEQcmd = &gConfig;
effect_config_t gEffectCfg;
effect_config_t *pEffectCfg = &gEffectCfg;
effect_param_t cmdData;
bool isSetGain;
bool isSetFreq;
bool isSetBandWidth;

//--- local function prototypes

int Equalizer_init(EqualizerContext *pContext);
int Equalizer_setConfig(EqualizerContext *pContext, effect_config_t *pConfig);
int Equalizer_getParameter(AUDIO_EQUALIZER * pEqualizer, int32_t *pParam, uint32_t *pValueSize, void *pValue);
int Equalizer_setParameter(AUDIO_EQUALIZER * pEqualizer, int32_t *pParam, void *pValue);


//
//--- Effect Library Interface Implementation
//

extern int EffectCreate(const effect_uuid_t *uuid,
                            int32_t sessionId,
                            int32_t ioId,
                            effect_handle_t *pHandle) {
    int ret;
    int i;
	EqualizerContext *pContext = NULL;

    if (pHandle == NULL || uuid == NULL) {
        return -EINVAL;
    }

    if (memcmp(uuid, &gEqualizerDescriptor.uuid, sizeof(effect_uuid_t)) != 0) {
        return -EINVAL;
    }

    pContext = &gContext;
	pContext->pEqualizer = &gEqualizer;
	pContext->pAdapter = &gAdapter;
	
    pContext->state = EQUALIZER_STATE_UNINITIALIZED;
    ret = Equalizer_init(pContext);
    if (ret != 0) {
		pContext->pEqualizer = NULL;
		pContext->pAdapter = NULL;
		pContext = NULL;
        return ret;
    }

    *pHandle = (effect_handle_t)pContext;
    pContext->state = EQUALIZER_STATE_INITIALIZED;

    return 0;
} /* end EffectCreate */

extern int EffectRelease(effect_handle_t handle) {
    EqualizerContext * pContext = (EqualizerContext *)handle;

    if (pContext == NULL) {
        return -EINVAL;
    }

    pContext->state = EQUALIZER_STATE_UNINITIALIZED;
	pContext->pEqualizer = NULL;
	pContext->pAdapter = NULL;
	pContext = NULL;

    return 0;
} /* end EffectRelease */

extern int EffectGetDescriptor(const effect_uuid_t *uuid,
                                   effect_descriptor_t *pDescriptor) {

    if (pDescriptor == NULL || uuid == NULL){
        return -EINVAL;
    }

    if (memcmp(uuid, &gEqualizerDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        *pDescriptor = gEqualizerDescriptor;
        return 0;
    }

    return  -EINVAL;
} /* end EffectGetDescriptor */


//
//--- local functions
//

#define CHECK_ARG(cond) {                     \
    if (!(cond)) {                            \
        return -EINVAL;                       \
    }                                         \
}

//----------------------------------------------------------------------------
// Equalizer_setConfig()
//----------------------------------------------------------------------------
// Purpose: Set input and output audio configuration.
//
// Inputs:
//  pContext:   effect engine context
//  pConfig:    pointer to effect_config_t structure holding input and output
//      configuration parameters
//
// Outputs:
//
//----------------------------------------------------------------------------

int Equalizer_setConfig(EqualizerContext *pContext, effect_config_t *pConfig)
{
	int channelCount;

    CHECK_ARG(pContext != NULL);
    CHECK_ARG(pConfig != NULL);

    CHECK_ARG(pConfig->inputCfg.samplingRate == pConfig->outputCfg.samplingRate);
    CHECK_ARG(pConfig->inputCfg.channels == pConfig->outputCfg.channels);
    CHECK_ARG(pConfig->inputCfg.format == pConfig->outputCfg.format);
    CHECK_ARG((pConfig->inputCfg.channels == AUDIO_CHANNEL_OUT_MONO) ||
              (pConfig->inputCfg.channels == AUDIO_CHANNEL_OUT_STEREO));
    CHECK_ARG(pConfig->outputCfg.accessMode == EFFECT_BUFFER_ACCESS_WRITE
              || pConfig->outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE);
    CHECK_ARG(pConfig->inputCfg.format == AUDIO_FORMAT_PCM_16_BIT);

    if (pConfig->inputCfg.channels == AUDIO_CHANNEL_OUT_MONO) {
        channelCount = 1;
    } else {
        channelCount = 2;
    }
    CHECK_ARG(channelCount <= MAX_CHANNELS);

    pContext->config = *pConfig;

    AudioEqualizerConfigure(pContext->pEqualizer, channelCount,
                          pConfig->inputCfg.samplingRate);

	AudioFormatAdapterConfigure(pContext->pAdapter, pContext->pEqualizer, 
		                channelCount,
                        pConfig->inputCfg.format,
                        pConfig->outputCfg.accessMode);

    return 0;
}   // end Equalizer_setConfig

//----------------------------------------------------------------------------
// Equalizer_getConfig()
//----------------------------------------------------------------------------
// Purpose: Get input and output audio configuration.
//
// Inputs:
//  pContext:   effect engine context
//  pConfig:    pointer to effect_config_t structure holding input and output
//      configuration parameters
//
// Outputs:
//
//----------------------------------------------------------------------------

void Equalizer_getConfig(EqualizerContext *pContext, effect_config_t *pConfig)
{
    *pConfig = pContext->config;
}   // end Equalizer_getConfig


//----------------------------------------------------------------------------
// Equalizer_init()
//----------------------------------------------------------------------------
// Purpose: Initialize engine with default configuration and creates
//     AudioEqualizer instance.
//
// Inputs:
//  pContext:   effect engine context
//
// Outputs:
//
//----------------------------------------------------------------------------

int Equalizer_init(EqualizerContext *pContext)
{
    int status;
	int i = 0;
    int32_t tmpNumBands = kNumBands;
    CHECK_ARG(pContext != NULL);

    pContext->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    pContext->config.inputCfg.channels = AUDIO_CHANNEL_OUT_MONO;///AUDIO_CHANNEL_OUT_STEREO
    pContext->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    pContext->config.inputCfg.samplingRate = 48000;///44100
    pContext->config.inputCfg.bufferProvider.getBuffer = NULL;
    pContext->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.inputCfg.bufferProvider.cookie = NULL;
    pContext->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    pContext->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_WRITE;
    pContext->config.outputCfg.channels = AUDIO_CHANNEL_OUT_MONO;///AUDIO_CHANNEL_OUT_STEREO
    pContext->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    pContext->config.outputCfg.samplingRate = 48000;///44100
    pContext->config.outputCfg.bufferProvider.getBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    pContext->config.outputCfg.bufferProvider.cookie = NULL;
    pContext->config.outputCfg.mask = EFFECT_CONFIG_ALL;
	
    _AudioEqualizer(pContext->pEqualizer, 
		tmpNumBands, 
		1, 
		44100, 
		gEqualizerPresets, 
		ARRAY_SIZE(gEqualizerPresets));

	for (i = 0; i < kNumBands; ++i) {
        AudioEqualizerSetGain(pContext->pEqualizer, i, 0x00);
        AudioEqualizerSetFrequency(pContext->pEqualizer, i, gFreqs[i]);
        AudioEqualizerSetBandwidth(pContext->pEqualizer, i, gBandwidths[i]);
    }
    AudioEqualizerEnable(pContext->pEqualizer, true);
    Equalizer_setConfig(pContext, &pContext->config);

    return 0;
}   // end Equalizer_init


//----------------------------------------------------------------------------
// Equalizer_getParameter()
//----------------------------------------------------------------------------
// Purpose:
// Get a Equalizer parameter
//
// Inputs:
//  pEqualizer       - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to variable to hold retrieved value
//  pValueSize       - pointer to value size: maximum size as input
//
// Outputs:
//  *pValue updated with parameter value
//  *pValueSize updated with actual value size
//
//
// Side Effects:
//
//----------------------------------------------------------------------------

int Equalizer_getParameter(AUDIO_EQUALIZER * pEqualizer, int32_t *pParam, uint32_t *pValueSize, void *pValue)
{
    int status = 0;
	int i = 0;
    int32_t param = *pParam++;
    int32_t param2;
    char *name;

    switch (param) {
    case EQ_PARAM_NUM_BANDS:
    case EQ_PARAM_CUR_PRESET:
    case EQ_PARAM_GET_NUM_OF_PRESETS:
    case EQ_PARAM_BAND_LEVEL:
    case EQ_PARAM_GET_BAND:
        if (*pValueSize < sizeof(int16_t)) {
            return -EINVAL;
        }
        *pValueSize = sizeof(int16_t);
        break;

    case EQ_PARAM_LEVEL_RANGE:
        if (*pValueSize < 2 * sizeof(int16_t)) {
            return -EINVAL;
        }
        *pValueSize = 2 * sizeof(int16_t);
        break;

    case EQ_PARAM_BAND_FREQ_RANGE:
        if (*pValueSize < 2 * sizeof(int32_t)) {
            return -EINVAL;
        }
        *pValueSize = 2 * sizeof(int32_t);
        break;

    case EQ_PARAM_CENTER_FREQ:
        if (*pValueSize < sizeof(int32_t)) {
            return -EINVAL;
        }
        *pValueSize = sizeof(int32_t);
        break;

    case EQ_PARAM_GET_PRESET_NAME:
        break;

    case EQ_PARAM_PROPERTIES:
        if (*pValueSize < (2 + kNumBands) * sizeof(uint16_t)) {
            return -EINVAL;
        }
        *pValueSize = (2 + kNumBands) * sizeof(uint16_t);
        break;

    default:
        return -EINVAL;
    }

    switch (param) {
    case EQ_PARAM_NUM_BANDS:
        *(uint16_t *)pValue = (uint16_t)kNumBands;
        break;

    case EQ_PARAM_LEVEL_RANGE:
        *(int16_t *)pValue = -9600;
        *((int16_t *)pValue + 1) = 4800;
        break;

    case EQ_PARAM_BAND_LEVEL:
        param2 = *pParam;
        if (param2 >= kNumBands) {
            status = -EINVAL;
            break;
        }
		*(int16_t *)pValue = (int16_t)AudioEqualizerGetGain(pEqualizer, param2);
        break;

    case EQ_PARAM_CENTER_FREQ:
        param2 = *pParam;
        if (param2 >= kNumBands) {
            status = -EINVAL;
            break;
        }
		*(int32_t *)pValue = AudioEqualizerGetFrequency(pEqualizer, param2);
        break;

    case EQ_PARAM_BAND_FREQ_RANGE:
        param2 = *pParam;
        if (param2 >= kNumBands) {
            status = -EINVAL;
            break;
        }
		AudioEqualizerGetBandRange(pEqualizer, param2, (uint32_t *)pValue, ((uint32_t *)pValue + 1));
        break;

    case EQ_PARAM_GET_BAND:
        param2 = *pParam;
		*(uint16_t *)pValue = (uint16_t)AudioEqualizerGetMostRelevantBand(pEqualizer, param2);
        break;

    case EQ_PARAM_CUR_PRESET:
		*(uint16_t *)pValue = (uint16_t)AudioEqualizerGetPreset(pEqualizer);
        break;

    case EQ_PARAM_GET_NUM_OF_PRESETS:
		*(uint16_t *)pValue = (uint16_t)AudioEqualizerGetNumPresets(pEqualizer);
        break;

    case EQ_PARAM_GET_PRESET_NAME:
        param2 = *pParam;
        if (param2 >= AudioEqualizerGetNumPresets(pEqualizer)) {
            status = -EINVAL;
            break;
        }
        name = (char *)pValue;
        strncpy(name, AudioEqualizerGetPresetName(pEqualizer, param2), *pValueSize - 1);
        name[*pValueSize - 1] = 0;
        *pValueSize = strlen(name) + 1;
        break;

    case EQ_PARAM_PROPERTIES: {
        int16_t *p = (int16_t *)pValue;
        p[0] = (int16_t)AudioEqualizerGetPreset(pEqualizer);
        p[1] = (int16_t)kNumBands;
        for (i = 0; i < kNumBands; i++) {
            p[2 + i] = (int16_t)AudioEqualizerGetGain(pEqualizer, i);
        }
    } break;

    default:
        status = -EINVAL;
        break;
    }

    return status;
} // end Equalizer_getParameter


//----------------------------------------------------------------------------
// Equalizer_setParameter()
//----------------------------------------------------------------------------
// Purpose:
// Set a Equalizer parameter
//
// Inputs:
//  pEqualizer       - handle to instance data
//  pParam           - pointer to parameter
//  pValue           - pointer to value
//
// Outputs:
//
//
// Side Effects:
//
//----------------------------------------------------------------------------

int Equalizer_setParameter (AUDIO_EQUALIZER * pEqualizer, int32_t *pParam, void *pValue)
{
	int i = 0;
    int status = 0;
    int32_t preset;
    int32_t band;
    int32_t level;
    int32_t param = *pParam++;
    
	switch (param) {
    case EQ_PARAM_CUR_PRESET:
        preset = (int32_t)(*(int32_t *)pValue);

        if (preset < 0 || preset >= AudioEqualizerGetNumPresets(pEqualizer)) {
            status = -EINVAL;
            break;
        }
		AudioEqualizerSetPreset(pEqualizer, preset);
		AudioEqualizerCommit(pEqualizer, true);
        break;
    case EQ_PARAM_BAND_LEVEL:
        band =  *pParam;
        level = *(int32_t *)pValue;
        if (band >= kNumBands) {
            status = -EINVAL;
            break;
        }
		AudioEqualizerSetGain(pEqualizer, band, level);
		AudioEqualizerCommit(pEqualizer, true);
        break;
    case EQ_PARAM_PROPERTIES: {
        int32_t *p = (int32_t *)pValue;
        if ((int)p[0] > AudioEqualizerGetNumPresets(pEqualizer)) {
            status = -EINVAL;
            break;
        }
        if (p[0] >= 1) {///changed by wangwp, 0 stand for customer mode
			AudioEqualizerSetPreset(pEqualizer, p[0]);
        } else {
            if ((int)p[1] != kNumBands) {
                status = -EINVAL;
                break;
            }
			if(isSetGain) {
                for (i = 0; i < kNumBands; i++) {
				    AudioEqualizerSetGain(pEqualizer, i, p[2 + i]);
                }				
			}			
			if(isSetFreq) {
                for (i = 0; i < kNumBands; i++) {
				    AudioEqualizerSetFrequency(pEqualizer, i, p[7 + i]);
                }
			}
			if(isSetBandWidth) {
                for (i = 0; i < kNumBands; i++) {
				    AudioEqualizerSetBandwidth(pEqualizer, i, p[12 + i]);
                }
			}
        }
        AudioEqualizerCommit(pEqualizer, true);
    } break;
    default:
        status = -EINVAL;
        break;
    }

    return status;
} // end Equalizer_setParameter

//
//--- Effect Control Interface Implementation
//

extern int Equalizer_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer, effect_sound_track indx)
{
    EqualizerContext * pContext = (EqualizerContext *) self;

    if (pContext == NULL) {
        return -EINVAL;
    }
    if (inBuffer == NULL || inBuffer->s16 == NULL ||
        outBuffer == NULL || outBuffer->s16 == NULL ||
        inBuffer->frameCount != outBuffer->frameCount) {
        return -EINVAL;
    }

    if (pContext->state == EQUALIZER_STATE_UNINITIALIZED) {
        return -EINVAL;
    }
    if (pContext->state == EQUALIZER_STATE_INITIALIZED) {
        return -ENODATA;
        ///return -61;///from errno.h
    }

	AudioFormatAdapterProcess(pContext->pAdapter, inBuffer->s16, outBuffer->s16, outBuffer->frameCount, indx);

    return 0;
}   // end Equalizer_process

extern int Equalizer_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
        void *pCmdData, uint32_t *replySize, void *pReplyData) {

    EqualizerContext * pContext = (EqualizerContext *) self;
    int retsize;
	AUDIO_EQUALIZER * pEqualizer = NULL;
	effect_param_t *p = NULL;
	int voffset;

    if (pContext == NULL || pContext->state == EQUALIZER_STATE_UNINITIALIZED) {
        return -EINVAL;
    }

    pEqualizer = pContext->pEqualizer;

    switch (cmdCode) {
    case EFFECT_CMD_INIT:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            return -EINVAL;
        }
        *(int *) pReplyData = Equalizer_init(pContext);
        break;
    case EFFECT_CMD_SET_CONFIG:
        if (pCmdData == NULL || cmdSize != sizeof(effect_config_t)
                || pReplyData == NULL || *replySize != sizeof(int)) {
            return -EINVAL;
        }
        *(int *) pReplyData = Equalizer_setConfig(pContext,
                (effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_GET_CONFIG:
        if (pReplyData == NULL || *replySize != sizeof(effect_config_t)) {
            return -EINVAL;
        }
        Equalizer_getConfig(pContext, (effect_config_t *) pCmdData);
        break;
    case EFFECT_CMD_RESET:
        Equalizer_setConfig(pContext, &pContext->config);
        break;
    case EFFECT_CMD_GET_PARAM: {
        if (pCmdData == NULL || cmdSize < (sizeof(effect_param_t) + sizeof(int32_t)) ||
            pReplyData == NULL || *replySize < (sizeof(effect_param_t) + sizeof(int32_t))) {
            return -EINVAL;
        }
        p = (effect_param_t *)pCmdData;
        memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);
        p = (effect_param_t *)pReplyData;
        voffset = 2;//((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
        p->status = Equalizer_getParameter(pEqualizer, (int32_t *)p->data, &p->vsize,
                p->data + voffset);
        *replySize = sizeof(effect_param_t) + voffset + p->vsize;

        } break;
    case EFFECT_CMD_SET_PARAM: {
        if (pCmdData == NULL || cmdSize < (sizeof(effect_param_t) + sizeof(int32_t)) ||
            pReplyData == NULL || *replySize != sizeof(int32_t)) {
            return -EINVAL;
        }
        p = (effect_param_t *) pCmdData;
        *(int *)pReplyData = Equalizer_setParameter(pEqualizer, (int32_t *)p->data,
                p->data + p->psize);
        } break;
    case EFFECT_CMD_ENABLE:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            return -EINVAL;
        }
        if (pContext->state != EQUALIZER_STATE_INITIALIZED) {
            return -ENOSYS;
        }
        pContext->state = EQUALIZER_STATE_ACTIVE;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_DISABLE:
        if (pReplyData == NULL || *replySize != sizeof(int)) {
            return -EINVAL;
        }
        if (pContext->state != EQUALIZER_STATE_ACTIVE) {
            return -ENOSYS;
        }
        pContext->state = EQUALIZER_STATE_INITIALIZED;
        *(int *)pReplyData = 0;
        break;
    case EFFECT_CMD_SET_DEVICE:
    case EFFECT_CMD_SET_VOLUME:
    case EFFECT_CMD_SET_AUDIO_MODE:
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

extern int Equalizer_getDescriptor(effect_handle_t   self, effect_descriptor_t *pDescriptor) {

	EqualizerContext * pContext = (EqualizerContext *) self;

    if (pContext == NULL || pDescriptor == NULL) {
        return -EINVAL;
    }

    *pDescriptor = gEqualizerDescriptor;

    return 0;
}


extern void EffectSetCmdEnable(effect_handle_t pEQHandle, bool flag)
{    
	int ret = 0;
	uint32_t replySize;
	uint32_t replyData;

	replySize = sizeof(int);
	replyData = 0;
	if(flag) {
	    pEQcmd->cmdCode = EFFECT_CMD_ENABLE;
	}
	else {
			pEQcmd->cmdCode = EFFECT_CMD_DISABLE;
	}
	pEQcmd->cmdSize = 0;
	pEQcmd->pCmdData = NULL;
	pEQcmd->pReplySize = &replySize;
	pEQcmd->pReplyData = (void *)&replyData;

	ret = Equalizer_command(pEQHandle, pEQcmd->cmdCode, pEQcmd->cmdSize, pEQcmd->pCmdData, pEQcmd->pReplySize, pEQcmd->pReplyData);
	if(ret)
	    printf("Equalizer_command enable: ret = 0x%x \n", ret);
}

extern void EffectSetCmdCurPreset(effect_handle_t pEQHandle, EFFECT_EQ_TYPE indx)
{
	int ret = 0;
	uint32_t replySize;
	uint32_t replyData;
	cmdData.status = 0;
	cmdData.psize = 1;///cmdData.data[1] - cmdData.data[0]
	cmdData.vsize = 0;
	cmdData.data[0] = EQ_PARAM_CUR_PRESET;
	cmdData.data[1] = indx;//preset 0->normal,1->Classic, 2->Jazz, 3->Pop, 4->Rock
	replySize = sizeof(int);
	replyData = 0;
	pEQcmd->cmdCode = EFFECT_CMD_SET_PARAM;
	pEQcmd->cmdSize = sizeof(effect_param_t) + sizeof(int);
	pEQcmd->pCmdData = (void *)&cmdData;
	pEQcmd->pReplySize = &replySize;
	pEQcmd->pReplyData = (void *)&replyData;

	ret = Equalizer_command(pEQHandle, pEQcmd->cmdCode, pEQcmd->cmdSize, pEQcmd->pCmdData, pEQcmd->pReplySize, pEQcmd->pReplyData);
	if(ret)
	    printf("Equalizer_command ret = 0x%x \n", ret);
	if(0 == cmdData.data[4])
		printf("Set Customer type \n");
	else if(1 == cmdData.data[4])
		printf("Set Classic type \n");
	else if(2 == cmdData.data[4])
		printf("Set Jazz type \n");
	else if(3 == cmdData.data[4])
		printf("Set Pop type \n");
	else if(4 == cmdData.data[4])
		printf("Set Rock type \n");
}

extern void EffectSetCmdBandLevel(effect_handle_t pEQHandle, int32_t bandIndx, int32_t gainValue)
{    
	int ret = 0;
	uint32_t replySize;
	uint32_t replyData;
	cmdData.status = 0;
	cmdData.psize = 2;///cmdData.data[2] - cmdData.data[0]
	cmdData.vsize = 0;
	cmdData.data[0] = EQ_PARAM_BAND_LEVEL;
	cmdData.data[1] = bandIndx;//num of band value: 0~4
	cmdData.data[2] = gainValue;//the band of cmdData.data[4]'s level value
	
	replySize = sizeof(int);
	replyData = 0;
	pEQcmd->cmdCode = EFFECT_CMD_SET_PARAM;
	pEQcmd->cmdSize = sizeof(effect_param_t) + sizeof(int);
	pEQcmd->pCmdData = (void *)&cmdData;
	pEQcmd->pReplySize = &replySize;
	pEQcmd->pReplyData = (void *)&replyData;

	ret = Equalizer_command(pEQHandle, pEQcmd->cmdCode, pEQcmd->cmdSize, pEQcmd->pCmdData, pEQcmd->pReplySize, pEQcmd->pReplyData);
	if(ret)
	    printf("Equalizer_command band level: ret = 0x%x \n", ret);
}

extern void EffectSetCmdProperties(effect_handle_t pEQHandle, 
	        EFFECT_EQ_TYPE indx, int32_t *pGainData, int32_t *pFreqData, int32_t *pBandwidthData) {    
	int i = 0;
	int ret = 0;
	uint32_t replySize;
	uint32_t replyData;
	
	isSetGain = false;
	isSetFreq = false;
	isSetBandWidth = false;
	cmdData.status = 0;
	cmdData.psize = 1;///cmdData.data[1] - cmdData.data[0]
	cmdData.vsize = 0;
	cmdData.data[0] = EQ_PARAM_PROPERTIES;
	cmdData.data[1] = indx;///preset index, user want to change which preset, set default value 0, Customer. p[0]
	cmdData.data[2] = kNumBands;///the band of cmdData.data[4]'s level value. p[1]
	if(NULL != pGainData) {
		cmdData.data[3] = 0;///the band 0's gain value. p[2]
		cmdData.data[4] = 0;///the band 1's gain value. p[3]
		cmdData.data[5] = 0;///the band 2's gain value. p[4]
		cmdData.data[6] = 0;///the band 3's gain value. p[5]
		cmdData.data[7] = 0;///the band 4's gain value. p[6]
	    for(i=0; i<kNumBands; i++) {
		    cmdData.data[3 + i] = pGainData[i];
	    }
		isSetGain = true;
	}
	if(NULL != pFreqData) {
		cmdData.data[8] = 0;///the band 0's freq value. p[7]
		cmdData.data[9] = 0;///the band 1's freq value. p[8]
		cmdData.data[10] = 0;///the band 2's freq value. p[9]
		cmdData.data[11] = 0;///the band 3's freq value. p[10]
		cmdData.data[12] = 0;///the band 4's freq value. p[11]
	    for(i=0; i<kNumBands; i++) {
		    cmdData.data[8 + i] = pFreqData[i];
	    }	
		isSetFreq = true;
	}
	if(NULL != pFreqData) {
		cmdData.data[13] = 0;///the band 0's bandwidth value. p[12]
		cmdData.data[14] = 0;///the band 1's bandwidth value. p[13]
		cmdData.data[15] = 0;///the band 2's bandwidth value. p[14]
		cmdData.data[16] = 0;///the band 3's bandwidth value. p[15]
		cmdData.data[17] = 0;///the band 4's bandwidth value. p[16]		
        for(i=0; i<kNumBands; i++) {
		    cmdData.data[13 + i] = pBandwidthData[i];
	    }
		isSetBandWidth = true;
	}
	replySize = sizeof(int);
	replyData = 0;
	pEQcmd->cmdCode = EFFECT_CMD_SET_PARAM;
	pEQcmd->cmdSize = sizeof(effect_param_t) + sizeof(int);
	pEQcmd->pCmdData = (void *)&cmdData;
	pEQcmd->pReplySize = &replySize;
	pEQcmd->pReplyData = (void *)&replyData;

	ret = Equalizer_command(pEQHandle, pEQcmd->cmdCode, pEQcmd->cmdSize, pEQcmd->pCmdData, pEQcmd->pReplySize, pEQcmd->pReplyData);
	if(ret)
	    printf("Equalizer_command band level: ret = 0x%x \n", ret);
}

extern void EffectSetCmdConfig(effect_handle_t pEQHandle)
{
	int ret = 0;
	uint32_t replySize;
	uint32_t replyData;

	replySize = sizeof(int);
	replyData = 0;

	pEffectCfg->inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
	pEffectCfg->inputCfg.channels = AUDIO_CHANNEL_OUT_MONO;///AUDIO_CHANNEL_OUT_STEREO
	pEffectCfg->inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
	pEffectCfg->inputCfg.samplingRate = 48000;///44100
	pEffectCfg->inputCfg.bufferProvider.getBuffer = NULL;
	pEffectCfg->inputCfg.bufferProvider.releaseBuffer = NULL;
	pEffectCfg->inputCfg.bufferProvider.cookie = NULL;
	pEffectCfg->inputCfg.mask = EFFECT_CONFIG_ALL;
	
	pEffectCfg->outputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
	pEffectCfg->outputCfg.channels = AUDIO_CHANNEL_OUT_MONO;///AUDIO_CHANNEL_OUT_STEREO
	pEffectCfg->outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
	pEffectCfg->outputCfg.samplingRate = 48000;///44100
	pEffectCfg->outputCfg.bufferProvider.getBuffer = NULL;
	pEffectCfg->outputCfg.bufferProvider.releaseBuffer = NULL;
	pEffectCfg->outputCfg.bufferProvider.cookie = NULL;
	pEffectCfg->outputCfg.mask = EFFECT_CONFIG_ALL;

	pEQcmd->cmdCode = EFFECT_CMD_SET_CONFIG;
	pEQcmd->cmdSize = sizeof(effect_config_t);
	pEQcmd->pCmdData = (void *)pEffectCfg;
	pEQcmd->pReplySize = &replySize;
	pEQcmd->pReplyData = (void *)&replyData;
	
	ret = Equalizer_command(pEQHandle, pEQcmd->cmdCode, pEQcmd->cmdSize, pEQcmd->pCmdData, pEQcmd->pReplySize, pEQcmd->pReplyData);
	if(ret)
	    printf("Equalizer_command set config: ret = 0x%x \n", ret);
}


AUDIO_EQ_CONFIG ggConfig;
AUDIO_EQ_CONFIG *pEQGetcmd = &ggConfig;
effect_param_t cmdGetData;
uint32_t gReplySize;
uint32_t gReplyData;

extern void EffectGetParam(effect_handle_t pEQHandle, int32_t indx) {    
	int i = 0;
	int ret = 0;

	cmdGetData.status = 0;
	cmdGetData.psize = 0;///
	cmdGetData.vsize = 2 * sizeof(int32_t);
	cmdGetData.data[0] = EQ_PARAM_BAND_FREQ_RANGE;
	cmdGetData.data[1] = indx;///0~4 band num
	cmdGetData.data[2] = 0;///the low band range
	cmdGetData.data[3] = 0;///the high band range
	gReplySize = sizeof(effect_param_t) + sizeof(int);
	gReplyData = 0;
	pEQGetcmd->cmdCode = EFFECT_CMD_GET_PARAM;
	pEQGetcmd->cmdSize = sizeof(effect_param_t) + sizeof(int);
	pEQGetcmd->pCmdData = (void *)&cmdGetData;
	pEQGetcmd->pReplySize = &gReplySize;
	pEQGetcmd->pReplyData = (void *)&gReplyData;

	printf("%d->low range is: %d, high range is: %d \n", cmdGetData.data[1], cmdGetData.data[2], cmdGetData.data[3]);
	ret = Equalizer_command(pEQHandle, pEQGetcmd->cmdCode, pEQGetcmd->cmdSize, pEQGetcmd->pCmdData, pEQGetcmd->pReplySize, pEQGetcmd->pReplyData);
	if(ret)
	    printf("Equalizer_command band level: ret = 0x%x \n", ret);
	printf("%d->low range is: %d, high range is: %d \n", cmdGetData.data[1], cmdGetData.data[2], cmdGetData.data[3]);
}

