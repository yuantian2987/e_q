/*
 * Copyright (C) 2011 The Android Open Source Project
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


#ifndef ANDROID_AUDIO_EFFECT_H
#define ANDROID_AUDIO_EFFECT_H

#include "AudioDef.h"
#include "stddef.h"
typedef struct int32_t **effect_handle_t;


typedef struct effect_uuid_s {
    uint32_t timeLow;
    uint16_t timeMid;
    uint16_t timeHiAndVersion;
    uint16_t clockSeq;
    uint8_t node[6];
} effect_uuid_t;

// Maximum length of character strings in structures defines by this API.
#define EFFECT_STRING_LEN_MAX 64

#define kNumBands  (5)

// NULL UUID definition (matches SL_IID_NULL_)
#define EFFECT_UUID_INITIALIZER { 0xec7178ec, 0xe5e1, 0x4432, 0xa3f4, \
                                  { 0x46, 0x57, 0xe6, 0x79, 0x52, 0x10 } }
static const effect_uuid_t EFFECT_UUID_NULL_ = EFFECT_UUID_INITIALIZER;
static const effect_uuid_t * const EFFECT_UUID_NULL = &EFFECT_UUID_NULL_;
static const char * const EFFECT_UUID_NULL_STR = "ec7178ec-e5e1-4432-a3f4-4657e6795210";


// The effect descriptor contains necessary information to facilitate the enumeration of the effect
// engines present in a library.
typedef struct effect_descriptor_s {
    effect_uuid_t type;     // UUID of to the OpenSL ES interface implemented by this effect
    effect_uuid_t uuid;     // UUID for this particular implementation
    uint32_t apiVersion;    // Version of the effect control API implemented
    uint32_t flags;         // effect engine capabilities/requirements flags (see below)
    uint16_t cpuLoad;       // CPU load indication (see below)
    uint16_t memoryUsage;   // Data Memory usage (see below)
    char    name[EFFECT_STRING_LEN_MAX];   // human readable effect name
    char    implementor[EFFECT_STRING_LEN_MAX];    // human readable effect implementor name
} effect_descriptor_t;

typedef struct _audio_buffer_t_ {
    size_t   frameCount;        // number of frames in buffer
    int16_t*    s16;        // pointer to signed 16 bit data at start of buffer
}audio_buffer_t;

typedef enum _effect_sound_track_ {
    LEFT_SOUND_TRACK = 0x00,
	RIGHT_SOUND_TRACK = 0x01
}effect_sound_track;

//
//--- Standardized command codes for command() function
//
typedef enum _effect_command_e_ {
   EFFECT_CMD_INIT,                 // initialize effect engine
   EFFECT_CMD_SET_CONFIG,           // configure effect engine (see effect_config_t)
   EFFECT_CMD_RESET,                // reset effect engine
   EFFECT_CMD_ENABLE,               // enable effect process
   EFFECT_CMD_DISABLE,              // disable effect process
   EFFECT_CMD_SET_PARAM,            // set parameter immediately (see effect_param_t)
   EFFECT_CMD_SET_PARAM_DEFERRED,   // set parameter deferred
   EFFECT_CMD_SET_PARAM_COMMIT,     // commit previous set parameter deferred
   EFFECT_CMD_GET_PARAM,            // get parameter
   EFFECT_CMD_SET_DEVICE,           // set audio device (see audio.h, audio_devices_t)
   EFFECT_CMD_SET_VOLUME,           // set volume
   EFFECT_CMD_SET_AUDIO_MODE,       // set the audio mode (normal, ring, ...)
   EFFECT_CMD_SET_CONFIG_REVERSE,   // configure effect engine reverse stream(see effect_config_t)
   EFFECT_CMD_SET_INPUT_DEVICE,     // set capture device (see audio.h, audio_devices_t)
   EFFECT_CMD_GET_CONFIG,           // read effect engine configuration
   EFFECT_CMD_GET_CONFIG_REVERSE,   // read configure effect engine reverse stream configuration
   EFFECT_CMD_GET_FEATURE_SUPPORTED_CONFIGS,// get all supported configurations for a feature.
   EFFECT_CMD_GET_FEATURE_CONFIG,   // get current feature configuration
   EFFECT_CMD_SET_FEATURE_CONFIG,   // set current feature configuration
   EFFECT_CMD_SET_AUDIO_SOURCE,     // set the audio source (see audio.h, audio_source_t)
   EFFECT_CMD_OFFLOAD,              // set if effect thread is an offload one,
                                    // send the ioHandle of the effect thread
   EFFECT_CMD_FIRST_PROPRIETARY = 0x10000 // first proprietary command code
}effect_command_e;

typedef enum _EFFECT_EQ_TYPE_ {
    CUSTOMER_TYPE = 0,//customer
	CLASSIC_TYPE,//Classic
	JAZZ_TYPE,//Jazz
	POP_TYPE,//Pop
	ROCK_TYPE//Rock
}EFFECT_EQ_TYPE;

// Values for "accessMode" field of buffer_config_t:
//   overwrite, read only, accumulate (read/modify/write)
enum effect_buffer_access_e {
    EFFECT_BUFFER_ACCESS_WRITE,
    EFFECT_BUFFER_ACCESS_READ,
    EFFECT_BUFFER_ACCESS_ACCUMULATE
};

// feature identifiers for EFFECT_CMD_GET_FEATURE_SUPPORTED_CONFIGS command
enum effect_feature_e {
    EFFECT_FEATURE_AUX_CHANNELS, // supports auxiliary channels (e.g. dual mic noise suppressor)
    EFFECT_FEATURE_CNT
};

typedef enum audio_channel_mask_s {
    AUDIO_CHANNEL_IN_MONO,
    AUDIO_CHANNEL_IN_STEREO,
    AUDIO_CHANNEL_IN_FRONT,
    AUDIO_CHANNEL_IN_BACK,
    AUDIO_CHANNEL_IN_RIGHT
}audio_channel_mask_t;

// EFFECT_FEATURE_AUX_CHANNELS feature configuration descriptor. Describe a combination
// of main and auxiliary channels supported
typedef struct channel_config_s {
    audio_channel_mask_t main_channels; // channel mask for main channels
    audio_channel_mask_t aux_channels;  // channel mask for auxiliary channels
} channel_config_t;

// Values for bit field "mask" in buffer_config_t. If a bit is set, the corresponding field
// in buffer_config_t must be taken into account when executing the EFFECT_CMD_SET_CONFIG command
#define EFFECT_CONFIG_BUFFER    0x0001  // buffer field must be taken into account
#define EFFECT_CONFIG_SMP_RATE  0x0002  // samplingRate field must be taken into account
#define EFFECT_CONFIG_CHANNELS  0x0004  // channels field must be taken into account
#define EFFECT_CONFIG_FORMAT    0x0008  // format field must be taken into account
#define EFFECT_CONFIG_ACC_MODE  0x0010  // accessMode field must be taken into account
#define EFFECT_CONFIG_PROVIDER  0x0020  // bufferProvider field must be taken into account
#define EFFECT_CONFIG_ALL (EFFECT_CONFIG_BUFFER | EFFECT_CONFIG_SMP_RATE | \
                           EFFECT_CONFIG_CHANNELS | EFFECT_CONFIG_FORMAT | \
                           EFFECT_CONFIG_ACC_MODE | EFFECT_CONFIG_PROVIDER)

typedef int32_t (* buffer_function_t)(void *cookie, audio_buffer_t *buffer);

typedef struct buffer_provider_s {
    buffer_function_t getBuffer;       // retrieve next buffer
    buffer_function_t releaseBuffer;   // release used buffer
    void       *cookie;                // for use by client of buffer provider functions
} buffer_provider_t;


typedef struct buffer_config_s {
    audio_buffer_t  buffer;     // buffer for use by process() function if not passed explicitly
    uint32_t   samplingRate;    // sampling rate
    uint32_t   channels;        // channel mask (see audio_channel_mask_t in audio.h)
    buffer_provider_t bufferProvider;   // buffer provider
    uint8_t    format;          // Audio format (see audio_format_t in audio.h)
    uint8_t    accessMode;      // read/write or accumulate in buffer (effect_buffer_access_e)
    uint16_t   mask;            // indicates which of the above fields is valid
} buffer_config_t;

typedef struct effect_config_s {
    buffer_config_t   inputCfg;
    buffer_config_t   outputCfg;
} effect_config_t;

typedef struct effect_param_s {
    int32_t     status;     // Transaction status (unused for command, used for reply)
    uint32_t    psize;      // Parameter size
    uint32_t    vsize;      // Value size
    int32_t     data[20];     // Start of Parameter + Value data
} effect_param_t;

// Effect control interface version 2.0
#define EFFECT_MAKE_API_VERSION(M, m)  (((M)<<16) | ((m) & 0xFFFF))
#define EFFECT_CONTROL_API_VERSION EFFECT_MAKE_API_VERSION(2,0)

// Insert mode
#define EFFECT_FLAG_TYPE_SHIFT          0
#define EFFECT_FLAG_TYPE_SIZE           3
#define EFFECT_FLAG_TYPE_MASK           (((1 << EFFECT_FLAG_TYPE_SIZE) -1) << EFFECT_FLAG_TYPE_SHIFT)
#define EFFECT_FLAG_TYPE_INSERT         (0 << EFFECT_FLAG_TYPE_SHIFT)
#define EFFECT_FLAG_TYPE_AUXILIARY      (1 << EFFECT_FLAG_TYPE_SHIFT)
#define EFFECT_FLAG_TYPE_REPLACE        (2 << EFFECT_FLAG_TYPE_SHIFT)
#define EFFECT_FLAG_TYPE_PRE_PROC       (3 << EFFECT_FLAG_TYPE_SHIFT)
#define EFFECT_FLAG_TYPE_POST_PROC      (4 << EFFECT_FLAG_TYPE_SHIFT)

// Insert preference
#define EFFECT_FLAG_INSERT_SHIFT        (EFFECT_FLAG_TYPE_SHIFT + EFFECT_FLAG_TYPE_SIZE)
#define EFFECT_FLAG_INSERT_SIZE         3
#define EFFECT_FLAG_INSERT_MASK         (((1 << EFFECT_FLAG_INSERT_SIZE) -1) << EFFECT_FLAG_INSERT_SHIFT)
#define EFFECT_FLAG_INSERT_ANY          (0 << EFFECT_FLAG_INSERT_SHIFT)
#define EFFECT_FLAG_INSERT_FIRST        (1 << EFFECT_FLAG_INSERT_SHIFT)
#define EFFECT_FLAG_INSERT_LAST         (2 << EFFECT_FLAG_INSERT_SHIFT)
#define EFFECT_FLAG_INSERT_EXCLUSIVE    (3 << EFFECT_FLAG_INSERT_SHIFT)


#endif  
