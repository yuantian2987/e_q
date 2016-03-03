/* added by wangwp */

#ifndef ANDROID_AUDIO_DEF_H
#define ANDROID_AUDIO_DEF_H


//#define CC_LIKELY(exp)   (__builtin_expect( !!(exp), 1 ))
//#define CC_UNLIKELY(exp) (__builtin_expect( !!(exp), 0 ))
#define CC_LIKELY(exp)   exp
#define CC_UNLIKELY(exp) exp
#define true (1)
#define false (0)

//typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
//typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef int bool;
//typedef int size_t;
typedef short INT16;
typedef unsigned short UINT16;
typedef enum _audio_format_pcm_ {
    AUDIO_FORMAT_PCM_8_24_BIT,
    AUDIO_FORMAT_PCM_16_BIT
}audio_format_pcm;

typedef enum _audio_ch_out_ {
    AUDIO_CHANNEL_OUT_MONO,
    AUDIO_CHANNEL_OUT_STEREO
}audio_ch_out;

typedef enum _eq_param_ {
    EQ_PARAM_NUM_BANDS,
    EQ_PARAM_CUR_PRESET,
    EQ_PARAM_GET_NUM_OF_PRESETS,
    EQ_PARAM_BAND_LEVEL,
    EQ_PARAM_GET_BAND,
    EQ_PARAM_LEVEL_RANGE,
    EQ_PARAM_BAND_FREQ_RANGE,
    EQ_PARAM_CENTER_FREQ,
    EQ_PARAM_GET_PRESET_NAME,
    EQ_PARAM_PROPERTIES
}eq_param;

typedef struct _AUDIO_EQ_CONFIG_ {
    uint32_t cmdCode;
	uint32_t cmdSize;
	void* pCmdData;
	uint32_t* pReplySize;
	void* pReplyData;
}AUDIO_EQ_CONFIG; 

#endif

