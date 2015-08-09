#include "svf.h"

int32_t _state_variable_struct_0_init(_state_variable_struct_0* st){
   st->pre_x = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _state_variable_struct_2_init(_state_variable_struct_2* st){
   st->dlow = 0 /* 0. */ ;
   st->dband = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _state_variable_struct_1_init(_state_variable_struct_1* st){
   st->g = 0 /* 0. */ ;
   st->_i4_pre_x = 0 /* 0. */ ;
   _state_variable_struct_2_init((&st->step));
   return 0 /* 0 */ ;
}

int32_t _state_variable_struct_change_init(_state_variable_struct_change* st){
   _state_variable_struct_0_init(st);
   return 0 /* 0 */ ;
}

int32_t _state_variable_struct_svf_init(_state_variable_struct_svf* st){
   _state_variable_struct_1_init(st);
   return 0 /* 0 */ ;
}

int32_t _state_variable_struct_svf_step_init(_state_variable_struct_svf_step* st){
   _state_variable_struct_2_init(st);
   return 0 /* 0 */ ;
}

int32_t _state_variable__svf_step(_state_variable_struct_2* _st_, int32_t input, int32_t g, int32_t q, int32_t sel){
   int32_t output;
   int32_t notch;
   int32_t low;
   int32_t high;
   int32_t band;
   low = (fix_add(_st_->dlow, (fix_mul(g, _st_->dband))));
   high = (fix_sub((fix_sub(input, low)), (fix_mul(q, _st_->dband))));
   band = (fix_add((fix_mul(g, high)), _st_->dband));
   notch = (fix_add(high, low));
   _st_->dband = fix_clip(band, -65536 /* -1 */ , 65536 /* 1 */ );
   _st_->dlow = fix_clip(low, -65536 /* -1 */ , 65536 /* 1 */ );
   if(sel == 0 /* 0 */ ){
      output = low;
   }
   else {
      output = (sel == 65536 /* 1 */ ?high:(sel == 131072 /* 2 */ ?band:notch));
   }
   return output;
}

int32_t _state_variable__svf(_state_variable_struct_1* _st_, int32_t input, int32_t fc, int32_t q, int32_t sel){
   int32_t x2;
   int32_t x1;
   int32_t fix_q;
   int32_t _tmp6;
   int32_t _tmp5;
   fc = fix_clip(fc, 0 /* 0 */ , 65536 /* 1 */ );
   q = fix_clip(q, 0 /* 0 */ , 65536 /* 1 */ );
   fix_q = (fix_mul(131072 /* 2 */ , (fix_sub(65536 /* 1 */ , q))));
   int32_t _i4_x;
   int32_t _i4_v;
   _i4_x = fc;
   _i4_v = (_st_->_i4_pre_x != _i4_x);
   _st_->_i4_pre_x = _i4_x;
   _tmp6 = _i4_v;
   if(_tmp6){
      _st_->g = (fix_mul(fc, 32768 /* 0.5 */ ));
   }
   x1 = _state_variable__svf_step((&_st_->step), input, _st_->g, fix_q, sel);
   x2 = _state_variable__svf_step((&_st_->step), input, _st_->g, fix_q, sel);
   _tmp5 = (fix_mul((fix_add(x1, x2)), 32768 /* 0.5 */ ));
   return _tmp5;
}

int32_t _state_variable__change(_state_variable_struct_0* _st_, int32_t x){
   int32_t v;
   v = (_st_->pre_x != x);
   _st_->pre_x = x;
   return v;
}

int32_t _state_variable_(){
}



