#ifndef _VULTIN_
#define _VULTIN_
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

static const int32_t fix_pi  = 205887;

// Type conversion
static inline float  fix_to_float(int32_t a) {
    return (float)a / 0x00010000;
}
static inline int32_t fix_from_float(float a){
    float temp = a * 0x00010000;
    return (int32_t)temp;
}
static inline int32_t fix_from_int(int a) {
    return a * 0x00010000;
}

// Basic operations for fixed point numbers
static inline int32_t fix_add(int32_t x, int32_t y){
    return x+y;
}

static inline int32_t fix_sub(int32_t x, int32_t y){
    return x-y;
}

static inline int32_t fix_mul(int32_t x, int32_t y){
    int64_t res = (int64_t) x * y;
    return res >> 16;
}

static inline int32_t fix_minus(int32_t x){
    return -x;
}

int32_t fix_div(int32_t a, int32_t b);

static inline int32_t fix_abs(int32_t x){
    return x<0?(-x):x;
}

static inline int32_t fix_min(int32_t a,int32_t b){
    return a<b?a:b;
}

static inline int32_t fix_max(int32_t a,int32_t b){
    return a>b?a:b;
}

static inline int32_t fix_clip(int32_t v,int32_t minv, int32_t maxv){
    return v>maxv?maxv:(v<minv?minv:v);
}

static inline int32_t fix_floor(int32_t x){
    return (x & 0xFFFF0000UL);
}

static inline int32_t fix_not(int32_t x){
    return ~x;
}

int32_t fix_exp(int32_t inValue);

int32_t fix_sin(int32_t inAngle);

#ifdef __cplusplus
}
#endif

#endif
