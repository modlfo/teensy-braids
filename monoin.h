#ifndef _MONOIN_
#define _MONOIN_

#include <math.h>
#include <stdint.h>
#include "vultin.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __monoin_struct_0 {
   int32_t pre;
   int32_t n4;
   int32_t n3;
   int32_t n2;
   int32_t n1;
   int32_t count;
} _monoin_struct_0;

int32_t _monoin_struct_0_init(_monoin_struct_0* st);

typedef struct __monoin_struct_0 _monoin_struct_isGateOn;

int32_t _monoin_struct_isGateOn_init(_monoin_struct_isGateOn* st);

typedef struct __monoin_struct_0 _monoin_struct_noteOff;

int32_t _monoin_struct_noteOff_init(_monoin_struct_noteOff* st);

typedef struct __monoin_struct_0 _monoin_struct_noteOn;

int32_t _monoin_struct_noteOn_init(_monoin_struct_noteOn* st);

int32_t _monoin__noteOn(_monoin_struct_0* _st_, int32_t n);

int32_t _monoin__noteOff(_monoin_struct_0* _st_, int32_t n);

int32_t _monoin__isGateOn(_monoin_struct_0* _st_);

int32_t _monoin_();



#ifdef __cplusplus
}
#endif
#endif
