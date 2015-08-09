#ifndef _SVF_
#define _SVF_

#include <math.h>
#include <stdint.h>
#include "vultin.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __state_variable_struct_0 {
   int32_t pre_x;
} _state_variable_struct_0;

int32_t _state_variable_struct_0_init(_state_variable_struct_0* st);

typedef struct __state_variable_struct_2 {
   int32_t dlow;
   int32_t dband;
} _state_variable_struct_2;

int32_t _state_variable_struct_2_init(_state_variable_struct_2* st);

typedef struct __state_variable_struct_1 {
   int32_t g;
   int32_t _i4_pre_x;
   _state_variable_struct_2 step;
} _state_variable_struct_1;

int32_t _state_variable_struct_1_init(_state_variable_struct_1* st);

typedef struct __state_variable_struct_0 _state_variable_struct_change;

int32_t _state_variable_struct_change_init(_state_variable_struct_change* st);

typedef struct __state_variable_struct_1 _state_variable_struct_svf;

int32_t _state_variable_struct_svf_init(_state_variable_struct_svf* st);

typedef struct __state_variable_struct_2 _state_variable_struct_svf_step;

int32_t _state_variable_struct_svf_step_init(_state_variable_struct_svf_step* st);

int32_t _state_variable__svf_step(_state_variable_struct_2* _st_, int32_t input, int32_t g, int32_t q, int32_t sel);

int32_t _state_variable__svf(_state_variable_struct_1* _st_, int32_t input, int32_t fc, int32_t q, int32_t sel);

int32_t _state_variable__change(_state_variable_struct_0* _st_, int32_t x);

int32_t _state_variable_();



#ifdef __cplusplus
}
#endif
#endif
