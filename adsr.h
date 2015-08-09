#ifndef _ADSR_
#define _ADSR_

#include <math.h>
#include <stdint.h>
#include "vultin.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __adsr_struct_0 {
   int32_t value;
   int32_t sustainLevel;
   int32_t state;
   int32_t releaseRate;
   int32_t gate;
   int32_t decayRate;
   int32_t attackRate;
   int32_t _i1_pre_x;
   int32_t _i0_count;
} _adsr_struct_0;

int32_t _adsr_struct_0_init(_adsr_struct_0* st);

typedef struct __adsr_struct_1 {
   int32_t pre_x;
} _adsr_struct_1;

int32_t _adsr_struct_1_init(_adsr_struct_1* st);

typedef struct __adsr_struct_2 {
   int32_t count;
} _adsr_struct_2;

int32_t _adsr_struct_2_init(_adsr_struct_2* st);

typedef struct __adsr_struct_0 _adsr_struct_adsr;

int32_t _adsr_struct_adsr_init(_adsr_struct_adsr* st);

typedef struct __adsr_struct_1 _adsr_struct_change;

int32_t _adsr_struct_change_init(_adsr_struct_change* st);

typedef struct __adsr_struct_2 _adsr_struct_each;

int32_t _adsr_struct_each_init(_adsr_struct_each* st);

int32_t _adsr__each(_adsr_struct_2* _st_, int32_t n);

int32_t _adsr__change(_adsr_struct_1* _st_, int32_t x);

int32_t _adsr__adsr(_adsr_struct_0* _st_, int32_t input, int32_t attack, int32_t decay, int32_t sustain, int32_t release);

int32_t _adsr_();



#ifdef __cplusplus
}
#endif
#endif
