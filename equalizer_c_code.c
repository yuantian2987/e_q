// equalizer_c_code.cpp : 定义控制台应用程序的入口点。
//

#include "audio_effect.h"
#include "stdio.h"
#define APP_INPUT_BUFF_SIZE  (0x4000)
#define APP_OUTPUT_BUFF_SIZE (APP_INPUT_BUFF_SIZE*4)
#define OUT_SAMPLE_RATE     (48000)
#define IN_SAMPLE_RATE      (48000)
#define IN_SAMPLE_RATE_441      (44100)
#define IN_SAMPLE_RATE_96      (96000)
//UINT16 av_ifc_buff_in[APP_INPUT_BUFF_SIZE];//0x4000 0x2000
//UINT32 av_ifc_buff_out[APP_OUTPUT_BUFF_SIZE];

UINT16 audio_process_buff_left[APP_OUTPUT_BUFF_SIZE];
UINT16 audio_process_buff_right[APP_OUTPUT_BUFF_SIZE];

INT16 app_in_buff_left[APP_INPUT_BUFF_SIZE];//0x1000
INT16 app_in_buff_right[APP_INPUT_BUFF_SIZE];//0x1000


/* EffectCreate and Equalizer_process preparatory work */
effect_uuid_t gUUID = {0xe25aa840, 0x543b, 0x11df, 0x98a5, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}};
effect_uuid_t *pUUID = &gUUID;
effect_handle_t pEQHandle;

audio_buffer_t inBuffer;
audio_buffer_t outBuffer;


extern int EffectCreate(const effect_uuid_t *uuid, int32_t sessionId, int32_t ioId, effect_handle_t *pHandle);

extern int Equalizer_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer, effect_sound_track indx);

extern void EffectSetCmdEnable(effect_handle_t pEQHandle, bool flag);

extern void EffectSetCmdCurPreset(effect_handle_t pEQHandle, EFFECT_EQ_TYPE indx);

extern void EffectSetCmdBandLevel(effect_handle_t pEQHandle, int32_t bandIndx, int32_t gainValue);

extern void EffectSetCmdProperties(effect_handle_t pEQHandle,
                                   EFFECT_EQ_TYPE indx, int32_t *pGainData, int32_t *pFreqData, int32_t *pBandwidthData);

extern void EffectSetCmdConfig(effect_handle_t pEQHandle);

extern void EffectGetParam(effect_handle_t pEQHandle, int32_t indx);



int main(int argc, char* argv[])
{
    int ret = 0;
    int i = 0;
    effect_sound_track indx = LEFT_SOUND_TRACK;
    EFFECT_EQ_TYPE typeIndx = CUSTOMER_TYPE;
    int32_t bandIndx = 0;
    int32_t gainValue = 0;
    int32_t gGainData[kNumBands] = {0, 0, 0, 0, 0};
    int32_t gFreqData[kNumBands] = {50000, 125000, 900000, 3200000, 6300000};
    int32_t gbandwidthData[kNumBands] = {0, 3600, 3600, 2400, 0};
    FILE *fp_in,*fp_out,*fp_test;
    int32_t n;

    fp_in  = fopen("48k_16bit.bin","rb");//48_1K_16bit.bin 44_1_1K_16bit.bin 96_1k_16bit.bin
    if (fp_in == NULL)
    {
        printf("aaaaa\n");
        return -1;
    }
    fp_out  = fopen("48k_16bit_out.bin","wb");//48_1K_16bit_out.bin 44_1K_16bit_out.bin 96_1k_16bit_out.bin
    if (fp_out == NULL)
    {
        printf("ddddd\n");
        return -2;
    }
    printf("main start\n");

    ret = EffectCreate(pUUID, 0, 0, &pEQHandle);///EffectCreate
    printf("EffectCreate ret = 0x%x \n", ret);

    printf("set_EQ_CMD enable \n");
    EffectSetCmdEnable(pEQHandle, true);///enable cmd
    printf("set_EQ_CMD set cur preset \n");
    EffectSetCmdCurPreset(pEQHandle, typeIndx);///set param cmd
    ///printf("set_EQ_CMD set gain \n");
    ///EffectSetCmdBandLevel(pEQHandle, bandIndx, gainValue);///set param cmd
    //printf("set_EQ_CMD set properties \n");
    ///EffectSetCmdProperties(pEQHandle, CUSTOMER_TYPE, gGainData, gFreqData, gbandwidthData);

    ///printf("set_EQ_CONFIG start set config \n");
    ///EffectSetCmdConfig(pEQHandle);///set config

    printf("Equalizer_process start\n");

    inBuffer.s16 = app_in_buff_left;
    outBuffer.s16 = audio_process_buff_left;
    while(1)
    {
        n = fread(&app_in_buff_left,2,APP_INPUT_BUFF_SIZE,fp_in);
        printf("read n = %d\n",n);
        if (n == 0)
        {
            printf("finished \n");
            break;
        }
        inBuffer.frameCount = outBuffer.frameCount = n;
        ret = Equalizer_process(pEQHandle, &inBuffer, &outBuffer, indx);///Equalizer_process
        printf("Equalizer_process ret = 0x%x \n", ret);
        fwrite(audio_process_buff_left,2,n,fp_out);

    }
    fclose(fp_in);
    fclose(fp_out);
    printf("get_EQ_CMD EffectGetParam \n");
    //EffectGetParam(pEQHandle, 2);
    printf("main end\n");
    //while(1);
    return 0;
}

