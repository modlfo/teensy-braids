#include "blit.h"

int32_t _blit_struct_0_init(_blit_struct_0* st){
   st->y1 = 0 /* 0. */ ;
   st->x1 = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _blit_struct_1_init(_blit_struct_1* st){
   st->triang = 0 /* 0. */ ;
   st->state_triang = 0 /* 0. */ ;
   st->state_saw = 0 /* 0. */ ;
   st->state_pulse = 0 /* 0. */ ;
   st->rate = 0 /* 0. */ ;
   st->pre_wave = 0 /* 0 */ ;
   st->pre_pitch = 0 /* 0. */ ;
   st->phase = 0 /* 0. */ ;
   st->p = 0 /* 0. */ ;
   st->output = 0 /* 0. */ ;
   st->m = 0 /* 0 */ ;
   st->_i11_y1 = 0 /* 0. */ ;
   st->_i11_x1 = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _blit_struct_dcblock_init(_blit_struct_dcblock* st){
   _blit_struct_0_init(st);
   return 0 /* 0 */ ;
}

int32_t _blit_struct_osc_init(_blit_struct_osc* st){
   _blit_struct_1_init(st);
   return 0 /* 0 */ ;
}

int32_t _blit__pulse_train(int32_t m, int32_t phase){
   int32_t tmp1;
   int32_t pi_phase;
   int32_t denominator1;
   int32_t _tmp22;
   pi_phase = (fix_mul(phase, 205887 /* 3.14159265359 */ ));
   denominator1 = fix_sin(pi_phase);
   tmp1 = 0 /* 0 */ ;
   int32_t _i3_x;
   int32_t _i3__tmp25;
   int32_t _i3__tmp14;
   _i3_x = denominator1;
   _i3__tmp25 = fix_abs(_i3_x);
   _i3__tmp14 = (_i3__tmp25 < 1310 /* 0.02 */ );
   _tmp22 = _i3__tmp14;
   if(_tmp22){
      tmp1 = 65536 /* 1 */ ;
   }
   else {
      tmp1 = fix_sin((fix_mul(m, pi_phase)));
      tmp1 = (fix_div(tmp1, (fix_mul(m, denominator1))));
   }
   return tmp1;
}

int32_t _blit__pitchToRate(int32_t d){
   int32_t _tmp24;
   int32_t _tmp13;
   _tmp24 = fix_exp((fix_mul(3785 /* 0.0577623 */ , d)));
   _tmp13 = (fix_mul(16 /* 0.00025549375 */ , _tmp24));
   return _tmp13;
}

int32_t _blit__osc(_blit_struct_1* _st_, int32_t pitch, int32_t pw, int32_t wave){
   int32_t tmp2;
   int32_t tmp1;
   int32_t shift05;
   int32_t shift;
   int32_t fixed_pitch;
   int32_t _tmp23;
   int32_t _tmp21;
   int32_t _tmp20;
   int32_t _tmp19;
   int32_t _tmp18;
   int32_t _tmp17;
   int32_t _tmp16;
   int32_t _tmp12;
   fixed_pitch = 0 /* 0 */ ;
   _tmp23 = fix_abs(_st_->output);
   _tmp21 = ((wave != _st_->pre_wave) && (_tmp23 < 3276 /* 0.05 */ ));
   if(_tmp21){
      _st_->pre_wave = wave;
   }
   _tmp20 = (_st_->pre_wave == 0 /* 0 */  || _st_->pre_wave == 65536 /* 1 */ );
   if(_tmp20){
      fixed_pitch = pitch;
   }
   else {
      fixed_pitch = (fix_add(pitch, 786432 /* 12 */ ));
   }
   _tmp19 = (_st_->pre_pitch != fixed_pitch);
   if(_tmp19){
      int32_t maxHarmonics;
      _st_->pre_pitch = fixed_pitch;
      int32_t _i6_d;
      int32_t _i6__tmp24;
      int32_t _i6__tmp13;
      _i6_d = pitch;
      _i6__tmp24 = fix_exp((fix_mul(3785 /* 0.0577623 */ , _i6_d)));
      _i6__tmp13 = (fix_mul(16 /* 0.00025549375 */ , _i6__tmp24));
      _st_->rate = _i6__tmp13;
      _st_->p = (fix_div(65536 /* 1 */ , _st_->rate));
      maxHarmonics = fix_floor((fix_mul(_st_->p, 32768 /* 0.5 */ )));
      _st_->m = (fix_add((fix_mul(131072 /* 2 */ , maxHarmonics)), 65536 /* 1 */ ));
   }
   shift05 = (fix_add(32768 /* 0.5 */ , (fix_mul(pw, 32112 /* 0.49 */ ))));
   shift = (fix_add(_st_->phase, shift05));
   _tmp18 = (shift > 65536 /* 1 */ );
   if(_tmp18){
      shift = (fix_sub(shift, 65536 /* 1 */ ));
   }
   tmp1 = _blit__pulse_train(_st_->m, _st_->phase);
   tmp2 = _blit__pulse_train(_st_->m, shift);
   _st_->phase = (fix_add(_st_->phase, _st_->rate));
   _tmp17 = (_st_->phase > 65536 /* 1 */ );
   if(_tmp17){
      _st_->phase = (fix_sub(_st_->phase, 65536 /* 1 */ ));
   }
   _st_->state_pulse = fix_clip((fix_sub((fix_add((fix_mul(_st_->state_pulse, 65503 /* 0.9995 */ )), tmp1)), tmp2)), -65536 /* -1 */ , 65536 /* 1 */ );
   _st_->state_saw = fix_clip((fix_add((fix_mul(_st_->state_saw, 65503 /* 0.9995 */ )), (fix_mul((fix_div((fix_sub((fix_add(tmp1, tmp2)), (fix_div(131072 /* 2 */ , _st_->p)))), shift05)), 32768 /* 0.5 */ )))), -65536 /* -1 */ , 65536 /* 1 */ );
   _st_->state_triang = fix_clip((fix_add((fix_mul(_st_->state_triang, 65503 /* 0.9995 */ )), (fix_div((fix_mul(131072 /* 2 */ , _st_->state_pulse)), _st_->p)))), -65536 /* -1 */ , 65536 /* 1 */ );
   _tmp16 = (_st_->pre_wave < 21845 /* 0.333333333333 */ );
   if(_tmp16){
      _st_->output = _st_->state_pulse;
   }
   else {
      int32_t _tmp15;
      _tmp15 = (_st_->pre_wave < 43690 /* 0.666666666667 */ );
      if(_tmp15){
         _st_->output = _st_->state_saw;
      }
      else {
         _st_->output = (fix_mul(131072 /* 2 */ , _st_->state_triang));
      }
   }
   int32_t _i11_y0;
   int32_t _i11_x0;
   _i11_x0 = _st_->output;
   _i11_y0 = (fix_add((fix_sub(_i11_x0, _st_->_i11_x1)), (fix_mul(_st_->_i11_y1, 65208 /* 0.995 */ ))));
   _st_->_i11_x1 = _i11_x0;
   _st_->_i11_y1 = _i11_y0;
   _st_->output = _i11_y0;
   _tmp12 = (fix_mul(_st_->output, 16384 /* 0.25 */ ));
   return _tmp12;
}

int32_t _blit__near_zero(int32_t x){
   int32_t _tmp25;
   int32_t _tmp14;
   _tmp25 = fix_abs(x);
   _tmp14 = (_tmp25 < 1310 /* 0.02 */ );
   return _tmp14;
}

int32_t _blit__dcblock(_blit_struct_0* _st_, int32_t x0){
   int32_t y0;
   y0 = (fix_add((fix_sub(x0, _st_->x1)), (fix_mul(_st_->y1, 65208 /* 0.995 */ ))));
   _st_->x1 = x0;
   _st_->y1 = y0;
   return y0;
}

int32_t _blit_(){
}



