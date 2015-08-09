#ifndef _BLIT_
#define _BLIT_

#include <math.h>
#include <stdint.h>
#include "vultin.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __blit_struct_0 {
   int32_t y1;
   int32_t x1;
} _blit_struct_0;

int32_t _blit_struct_0_init(_blit_struct_0* st);

typedef struct __blit_struct_1 {
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
   int32_t _i11_y1;
   int32_t _i11_x1;
} _blit_struct_1;

int32_t _blit_struct_1_init(_blit_struct_1* st);

typedef struct __blit_struct_0 _blit_struct_dcblock;

int32_t _blit_struct_dcblock_init(_blit_struct_dcblock* st);

typedef struct __blit_struct_1 _blit_struct_osc;

int32_t _blit_struct_osc_init(_blit_struct_osc* st);

int32_t _blit__pulse_train(int32_t m, int32_t phase);

int32_t _blit__pitchToRate(int32_t d);

int32_t _blit__osc(_blit_struct_1* _st_, int32_t pitch, int32_t pw, int32_t wave);

int32_t _blit__near_zero(int32_t x);

int32_t _blit__dcblock(_blit_struct_0* _st_, int32_t x0);

int32_t _blit_();



#ifdef __cplusplus
}
#endif
#endif
