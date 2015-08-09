#ifndef _VOICE_
#define _VOICE_

#include <math.h>
#include <stdint.h>
#include "vultin.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __voice_struct_5 {
   int32_t triang;
   int32_t state_triang;
   int32_t state_saw;
   int32_t state_pulse;
   int32_t rate;
   int32_t pre_wave;
   int32_t pre_pitch;
   int32_t phase;
   int32_t p;
   int32_t output;
   int32_t m;
   int32_t _i25_y1;
   int32_t _i25_x1;
} _voice_struct_5;

int32_t _voice_struct_5_init(_voice_struct_5* st);

typedef struct __voice_struct_8 {
   int32_t dlow;
   int32_t dband;
} _voice_struct_8;

int32_t _voice_struct_8_init(_voice_struct_8* st);

typedef struct __voice_struct_7 {
   int32_t g;
   int32_t _i30_pre_x;
   _voice_struct_8 step;
} _voice_struct_7;

int32_t _voice_struct_7_init(_voice_struct_7* st);

typedef struct __voice_struct_0 {
   int32_t wave;
   int32_t res;
   int32_t pw;
   int32_t pitch;
   int32_t cut;
   _voice_struct_7 _i32;
   _voice_struct_5 _i31;
} _voice_struct_0;

int32_t _voice_struct_0_init(_voice_struct_0* st);

typedef struct __voice_struct_1 {
   int32_t pre_x;
} _voice_struct_1;

int32_t _voice_struct_1_init(_voice_struct_1* st);

typedef struct __voice_struct_2 {
   int32_t y1;
   int32_t x1;
} _voice_struct_2;

int32_t _voice_struct_2_init(_voice_struct_2* st);

typedef struct __voice_struct_4 {
   int32_t tw2;
   int32_t tw1;
   int32_t tw0;
   int32_t dw3;
   int32_t dw2;
   int32_t dw1;
   int32_t dw0;
} _voice_struct_4;

int32_t _voice_struct_4_init(_voice_struct_4* st);

typedef struct __voice_struct_3 {
   int32_t tune;
   int32_t resFixed;
   int32_t dx1;
   int32_t _i12_pre_x;
   int32_t _i11_pre_x;
   _voice_struct_4 filter;
} _voice_struct_3;

int32_t _voice_struct_3_init(_voice_struct_3* st);

typedef struct __voice_struct_6 {
   _voice_struct_7 _i32;
   _voice_struct_5 _i31;
} _voice_struct_6;

int32_t _voice_struct_6_init(_voice_struct_6* st);

typedef struct __voice_struct_0 _voice_struct__voice_;

int32_t _voice_struct__voice__init(_voice_struct__voice_* st);

typedef struct __voice_struct_1 _voice_struct_change;

int32_t _voice_struct_change_init(_voice_struct_change* st);

typedef struct __voice_struct_2 _voice_struct_dcblock;

int32_t _voice_struct_dcblock_init(_voice_struct_dcblock* st);

typedef struct __voice_struct_3 _voice_struct_moog;

int32_t _voice_struct_moog_init(_voice_struct_moog* st);

typedef struct __voice_struct_4 _voice_struct_moog_step;

int32_t _voice_struct_moog_step_init(_voice_struct_moog_step* st);

typedef struct __voice_struct_5 _voice_struct_osc;

int32_t _voice_struct_osc_init(_voice_struct_osc* st);

typedef struct __voice_struct_6 _voice_struct_process;

int32_t _voice_struct_process_init(_voice_struct_process* st);

typedef struct __voice_struct_7 _voice_struct_svf;

int32_t _voice_struct_svf_init(_voice_struct_svf* st);

typedef struct __voice_struct_8 _voice_struct_svf_step;

int32_t _voice_struct_svf_step_init(_voice_struct_svf_step* st);

int32_t _voice__thermal();

int32_t _voice__svf_step(_voice_struct_8* _st_, int32_t input, int32_t g, int32_t q, int32_t sel);

int32_t _voice__svf(_voice_struct_7* _st_, int32_t input, int32_t fc, int32_t q, int32_t sel);

int32_t _voice__samplerate();

int32_t _voice__pulse_train(int32_t m, int32_t phase);

int32_t _voice__process(_voice_struct_6* _st_, int32_t pitch, int32_t pw, int32_t wave, int32_t cut, int32_t res);

int32_t _voice__pitchToRate(int32_t d);

int32_t _voice__osc(_voice_struct_5* _st_, int32_t pitch, int32_t pw, int32_t wave);

int32_t _voice__near_zero(int32_t x);

int32_t _voice__moog_step(_voice_struct_4* _st_, int32_t input, int32_t resFixed, int32_t tune, int32_t output);

int32_t _voice__moog(_voice_struct_3* _st_, int32_t input, int32_t cut, int32_t res);

int32_t _voice__dcblock(_voice_struct_2* _st_, int32_t x0);

int32_t _voice__change(_voice_struct_1* _st_, int32_t x);

int32_t _voice__PI();

int32_t _voice_(_voice_struct_0* _st_);



#ifdef __cplusplus
}
#endif
#endif
