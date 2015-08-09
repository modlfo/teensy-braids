#include "voice.h"

int32_t _voice_struct_5_init(_voice_struct_5* st){
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
   st->_i25_y1 = 0 /* 0. */ ;
   st->_i25_x1 = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _voice_struct_8_init(_voice_struct_8* st){
   st->dlow = 0 /* 0. */ ;
   st->dband = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _voice_struct_7_init(_voice_struct_7* st){
   st->g = 0 /* 0. */ ;
   st->_i30_pre_x = 0 /* 0. */ ;
   _voice_struct_8_init((&st->step));
   return 0 /* 0 */ ;
}

int32_t _voice_struct_0_init(_voice_struct_0* st){
   st->wave = 0 /* 0 */ ;
   st->res = 0 /* 0. */ ;
   st->pw = 0 /* 0. */ ;
   st->pitch = 0 /* 0. */ ;
   st->cut = 0 /* 0. */ ;
   _voice_struct_7_init((&st->_i32));
   _voice_struct_5_init((&st->_i31));
   return 0 /* 0 */ ;
}

int32_t _voice_struct_1_init(_voice_struct_1* st){
   st->pre_x = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _voice_struct_2_init(_voice_struct_2* st){
   st->y1 = 0 /* 0. */ ;
   st->x1 = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _voice_struct_4_init(_voice_struct_4* st){
   st->tw2 = 0 /* 0. */ ;
   st->tw1 = 0 /* 0. */ ;
   st->tw0 = 0 /* 0. */ ;
   st->dw3 = 0 /* 0. */ ;
   st->dw2 = 0 /* 0. */ ;
   st->dw1 = 0 /* 0. */ ;
   st->dw0 = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _voice_struct_3_init(_voice_struct_3* st){
   st->tune = 0 /* 0. */ ;
   st->resFixed = 0 /* 0. */ ;
   st->dx1 = 0 /* 0. */ ;
   st->_i12_pre_x = 0 /* 0. */ ;
   st->_i11_pre_x = 0 /* 0. */ ;
   _voice_struct_4_init((&st->filter));
   return 0 /* 0 */ ;
}

int32_t _voice_struct_6_init(_voice_struct_6* st){
   _voice_struct_7_init((&st->_i32));
   _voice_struct_5_init((&st->_i31));
   return 0 /* 0 */ ;
}

int32_t _voice_struct__voice__init(_voice_struct__voice_* st){
   _voice_struct_0_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice_struct_change_init(_voice_struct_change* st){
   _voice_struct_1_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice_struct_dcblock_init(_voice_struct_dcblock* st){
   _voice_struct_2_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice_struct_moog_init(_voice_struct_moog* st){
   _voice_struct_3_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice_struct_moog_step_init(_voice_struct_moog_step* st){
   _voice_struct_4_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice_struct_osc_init(_voice_struct_osc* st){
   _voice_struct_5_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice_struct_process_init(_voice_struct_process* st){
   _voice_struct_6_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice_struct_svf_init(_voice_struct_svf* st){
   _voice_struct_7_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice_struct_svf_step_init(_voice_struct_svf_step* st){
   _voice_struct_8_init(st);
   return 0 /* 0 */ ;
}

int32_t _voice__thermal(){
   return 53687 /* 0.819199996645 */ ;
}

int32_t _voice__svf_step(_voice_struct_8* _st_, int32_t input, int32_t g, int32_t q, int32_t sel){
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

int32_t _voice__svf(_voice_struct_7* _st_, int32_t input, int32_t fc, int32_t q, int32_t sel){
   int32_t x2;
   int32_t x1;
   int32_t fix_q;
   int32_t _tmp37;
   int32_t _tmp33;
   fc = fix_clip(fc, 0 /* 0 */ , 65536 /* 1 */ );
   q = fix_clip(q, 0 /* 0 */ , 65536 /* 1 */ );
   fix_q = (fix_mul(131072 /* 2 */ , (fix_sub(65536 /* 1 */ , q))));
   int32_t _i30_x;
   int32_t _i30_v;
   _i30_x = fc;
   _i30_v = (_st_->_i30_pre_x != _i30_x);
   _st_->_i30_pre_x = _i30_x;
   _tmp37 = _i30_v;
   if(_tmp37){
      _st_->g = (fix_mul(fc, 32768 /* 0.5 */ ));
   }
   x1 = _voice__svf_step((&_st_->step), input, _st_->g, fix_q, sel);
   x2 = _voice__svf_step((&_st_->step), input, _st_->g, fix_q, sel);
   _tmp33 = (fix_mul((fix_add(x1, x2)), 32768 /* 0.5 */ ));
   return _tmp33;
}

int32_t _voice__samplerate(){
   return 2890137600 /* 44100 */ ;
}

int32_t _voice__pulse_train(int32_t m, int32_t phase){
   int32_t tmp1;
   int32_t pi_phase;
   int32_t denominator1;
   int32_t _tmp49;
   int32_t _tmp46;
   _tmp49 = 205887 /* 3.14159265359 */ ;
   pi_phase = (fix_mul(phase, _tmp49));
   denominator1 = fix_sin(pi_phase);
   tmp1 = 0 /* 0 */ ;
   int32_t _i17_x;
   int32_t _i17__tmp66;
   int32_t _i17__tmp36;
   _i17_x = denominator1;
   _i17__tmp66 = fix_abs(_i17_x);
   _i17__tmp36 = (_i17__tmp66 < 0 /* 1e-12 */ );
   _tmp46 = _i17__tmp36;
   if(_tmp46){
      tmp1 = 65536 /* 1 */ ;
   }
   else {
      tmp1 = fix_sin((fix_mul(m, pi_phase)));
      tmp1 = (fix_div(tmp1, (fix_mul(m, denominator1))));
   }
   return tmp1;
}

int32_t _voice__process(_voice_struct_6* _st_, int32_t pitch, int32_t pw, int32_t wave, int32_t cut, int32_t res){
   int32_t osc;
   int32_t filter;
   osc = _voice__osc((&_st_->_i31), pitch, pw, wave);
   filter = _voice__svf((&_st_->_i32), osc, cut, res, 0 /* 0 */ );
   return filter;
}

int32_t _voice__pitchToRate(int32_t d){
   int32_t _tmp51;
   int32_t _tmp50;
   int32_t _tmp34;
   _tmp50 = fix_exp((fix_mul(3785 /* 0.0577623 */ , d)));
   _tmp51 = 2890137600 /* 44100 */ ;
   _tmp34 = (fix_div((fix_mul(535809 /* 8.1758 */ , _tmp50)), _tmp51));
   return _tmp34;
}

int32_t _voice__osc(_voice_struct_5* _st_, int32_t pitch, int32_t pw, int32_t wave){
   int32_t tmp2;
   int32_t tmp1;
   int32_t shift05;
   int32_t shift;
   int32_t fixed_pitch;
   int32_t _tmp48;
   int32_t _tmp45;
   int32_t _tmp44;
   int32_t _tmp43;
   int32_t _tmp42;
   int32_t _tmp41;
   int32_t _tmp40;
   fixed_pitch = 0 /* 0 */ ;
   _tmp48 = fix_abs(_st_->output);
   _tmp45 = ((wave != _st_->pre_wave) && (_tmp48 < 3276 /* 0.05 */ ));
   if(_tmp45){
      _st_->pre_wave = wave;
   }
   _tmp44 = (_st_->pre_wave == 0 /* 0 */  || _st_->pre_wave == 65536 /* 1 */ );
   if(_tmp44){
      fixed_pitch = pitch;
   }
   else {
      fixed_pitch = (fix_add(pitch, 786432 /* 12 */ ));
   }
   _tmp43 = (_st_->pre_pitch != fixed_pitch);
   if(_tmp43){
      int32_t maxHarmonics;
      _st_->pre_pitch = fixed_pitch;
      int32_t _i20_d;
      int32_t _i20__tmp51;
      int32_t _i20__tmp50;
      int32_t _i20__tmp34;
      _i20_d = pitch;
      _i20__tmp50 = fix_exp((fix_mul(3785 /* 0.0577623 */ , _i20_d)));
      _i20__tmp51 = 2890137600 /* 44100 */ ;
      _i20__tmp34 = (fix_div((fix_mul(535809 /* 8.1758 */ , _i20__tmp50)), _i20__tmp51));
      _st_->rate = _i20__tmp34;
      _st_->p = (fix_div(65536 /* 1 */ , _st_->rate));
      maxHarmonics = fix_floor((fix_mul(_st_->p, 16384 /* 0.25 */ )));
      _st_->m = (fix_add((fix_mul(131072 /* 2 */ , maxHarmonics)), 65536 /* 1 */ ));
   }
   shift05 = (fix_add(32768 /* 0.5 */ , (fix_mul(pw, 32112 /* 0.49 */ ))));
   shift = (fix_add(_st_->phase, shift05));
   _tmp42 = (shift > 65536 /* 1 */ );
   if(_tmp42){
      shift = (fix_sub(shift, 65536 /* 1 */ ));
   }
   tmp1 = _voice__pulse_train(_st_->m, _st_->phase);
   tmp2 = _voice__pulse_train(_st_->m, shift);
   _st_->phase = (fix_add(_st_->phase, _st_->rate));
   _tmp41 = (_st_->phase > 65536 /* 1 */ );
   if(_tmp41){
      _st_->phase = (fix_sub(_st_->phase, 65536 /* 1 */ ));
   }
   _st_->state_pulse = fix_clip((fix_sub((fix_add((fix_mul(_st_->state_pulse, 65503 /* 0.9995 */ )), tmp1)), tmp2)), -65536 /* -1 */ , 65536 /* 1 */ );
   _st_->state_saw = fix_clip((fix_add((fix_mul(_st_->state_saw, 65503 /* 0.9995 */ )), (fix_mul((fix_div((fix_sub((fix_add(tmp1, tmp2)), (fix_div(131072 /* 2 */ , _st_->p)))), shift05)), 32768 /* 0.5 */ )))), -65536 /* -1 */ , 65536 /* 1 */ );
   _st_->state_triang = fix_clip((fix_add((fix_mul(_st_->state_triang, 65503 /* 0.9995 */ )), (fix_div((fix_mul(131072 /* 2 */ , _st_->state_pulse)), _st_->p)))), -65536 /* -1 */ , 65536 /* 1 */ );
   _tmp40 = _st_->pre_wave == 0 /* 0 */ ;
   if(_tmp40){
      _st_->output = _st_->state_pulse;
   }
   else {
      int32_t _tmp39;
      _tmp39 = _st_->pre_wave == 65536 /* 1 */ ;
      if(_tmp39){
         _st_->output = _st_->state_saw;
      }
      else {
         int32_t _tmp38;
         _tmp38 = _st_->pre_wave == 131072 /* 2 */ ;
         if(_tmp38){
            _st_->output = (fix_mul(131072 /* 2 */ , _st_->state_triang));
         }
      }
   }
   int32_t _i25_y0;
   int32_t _i25_x0;
   _i25_x0 = _st_->output;
   _i25_y0 = (fix_add((fix_sub(_i25_x0, _st_->_i25_x1)), (fix_mul(_st_->_i25_y1, 65208 /* 0.995 */ ))));
   _st_->_i25_x1 = _i25_x0;
   _st_->_i25_y1 = _i25_y0;
   _st_->output = _i25_y0;
   return _st_->output;
}

int32_t _voice__near_zero(int32_t x){
   int32_t _tmp66;
   int32_t _tmp36;
   _tmp66 = fix_abs(x);
   _tmp36 = (_tmp66 < 0 /* 1e-12 */ );
   return _tmp36;
}

int32_t _voice__moog_step(_voice_struct_4* _st_, int32_t input, int32_t resFixed, int32_t tune, int32_t output){
   int32_t w3;
   int32_t w2;
   int32_t w1;
   int32_t w0;
   int32_t i0;
   int32_t _tmp65;
   int32_t _tmp64;
   int32_t _tmp63;
   int32_t _tmp62;
   int32_t _tmp61;
   int32_t _tmp60;
   int32_t _tmp59;
   i0 = (fix_sub(input, (fix_mul(resFixed, output))));
   _tmp64 = 53687 /* 0.819199996645 */ ;
   _tmp65 = tanh((fix_mul(i0, _tmp64)));
   w0 = (fix_add(_st_->dw0, (fix_mul(tune, (fix_sub(_tmp65, _st_->tw0))))));
   _tmp63 = 53687 /* 0.819199996645 */ ;
   _st_->tw0 = tanh((fix_mul(w0, _tmp63)));
   w1 = (fix_sub((fix_add(_st_->dw1, (fix_mul(tune, _st_->tw0)))), _st_->tw1));
   _tmp62 = 53687 /* 0.819199996645 */ ;
   _st_->tw1 = tanh((fix_mul(w1, _tmp62)));
   w2 = (fix_sub((fix_add(_st_->dw2, (fix_mul(tune, _st_->tw1)))), _st_->tw2));
   _tmp61 = 53687 /* 0.819199996645 */ ;
   _st_->tw2 = tanh((fix_mul(w2, _tmp61)));
   _tmp59 = 53687 /* 0.819199996645 */ ;
   _tmp60 = tanh((fix_mul(_st_->dw3, _tmp59)));
   w3 = (fix_sub((fix_add(_st_->dw3, (fix_mul(tune, _st_->tw2)))), _tmp60));
   _st_->dw0 = w0;
   _st_->dw1 = w1;
   _st_->dw2 = w2;
   _st_->dw3 = w3;
   return w3;
}

int32_t _voice__moog(_voice_struct_3* _st_, int32_t input, int32_t cut, int32_t res){
   int32_t x1;
   int32_t x0;
   int32_t _tmp58;
   int32_t _tmp57;
   int32_t _tmp47;
   int32_t _tmp35;
   int32_t _i11_x;
   int32_t _i11_v;
   _i11_x = cut;
   _i11_v = (_st_->_i11_pre_x != _i11_x);
   _st_->_i11_pre_x = _i11_x;
   _tmp57 = _i11_v;
   int32_t _i12_x;
   int32_t _i12_v;
   _i12_x = res;
   _i12_v = (_st_->_i12_pre_x != _i12_x);
   _st_->_i12_pre_x = _i12_x;
   _tmp58 = _i12_v;
   _tmp47 = (_tmp57 || _tmp58);
   if(_tmp47){
      int32_t x_2;
      int32_t x3;
      int32_t x2;
      int32_t fcr;
      int32_t fc;
      int32_t acr;
      int32_t _tmp56;
      int32_t _tmp55;
      int32_t _tmp54;
      int32_t _tmp53;
      int32_t _tmp52;
      res = fix_clip(res, 0 /* 0 */ , 65536 /* 1 */ );
      _tmp56 = 2890137600 /* 44100 */ ;
      cut = fix_clip(cut, 65536 /* 1 */ , _tmp56);
      _tmp55 = 2890137600 /* 44100 */ ;
      fc = (fix_div(cut, _tmp55));
      x_2 = (fix_mul(fc, 32768 /* 0.5 */ ));
      x2 = (fix_mul(fc, fc));
      x3 = (fix_mul(fc, x2));
      fcr = (fix_add((fix_sub((fix_add((fix_mul(122748 /* 1.873 */ , x3)), (fix_mul(32473 /* 0.4955 */ , x2)))), (fix_mul(42532 /* 0.649 */ , fc)))), 65457 /* 0.9988 */ ));
      acr = (fix_add((fix_add((fix_mul(-257975 /* -3.9364 */ , x2)), (fix_mul(120645 /* 1.8409 */ , fc)))), 65326 /* 0.9968 */ ));
      _tmp52 = 205887 /* 3.14159265359 */ ;
      _tmp53 = fix_exp((fix_minus((fix_mul((fix_mul((fix_mul(131072 /* 2 */ , _tmp52)), x_2)), fcr)))));
      _tmp54 = 53687 /* 0.819199996645 */ ;
      _st_->tune = (fix_div((fix_sub(65536 /* 1. */ , _tmp53)), _tmp54));
      _st_->resFixed = (fix_mul((fix_mul(262144 /* 4 */ , res)), acr));
   }
   x0 = _voice__moog_step((&_st_->filter), input, _st_->resFixed, _st_->tune, _st_->dx1);
   x1 = _voice__moog_step((&_st_->filter), input, _st_->resFixed, _st_->tune, x0);
   _st_->dx1 = x1;
   _tmp35 = (fix_mul((fix_add(x0, x1)), 32768 /* 0.5 */ ));
   return _tmp35;
}

int32_t _voice__dcblock(_voice_struct_2* _st_, int32_t x0){
   int32_t y0;
   y0 = (fix_add((fix_sub(x0, _st_->x1)), (fix_mul(_st_->y1, 65208 /* 0.995 */ ))));
   _st_->x1 = x0;
   _st_->y1 = y0;
   return y0;
}

int32_t _voice__change(_voice_struct_1* _st_, int32_t x){
   int32_t v;
   v = (_st_->pre_x != x);
   _st_->pre_x = x;
   return v;
}

int32_t _voice__PI(){
   return 205887 /* 3.14159265359 */ ;
}

int32_t _voice_(_voice_struct_0* _st_){
   int32_t n;
   _st_->pitch = 26214400 /* 400 */ ;
   _st_->pw = 19660 /* 0.3 */ ;
   _st_->wave = 0 /* 0 */ ;
   _st_->cut = 0 /* 0 */ ;
   _st_->res = 0 /* 0 */ ;
   n = 0 /* 0 */ ;
   while(n < 2890137600 /* 44100 */ ){
      int32_t kk;
      int32_t x_wave;
      int32_t x_res;
      int32_t x_pw;
      int32_t x_pitch;
      int32_t x_osc;
      int32_t x_filter;
      int32_t x_cut;
      x_pitch = _st_->pitch;
      x_pw = _st_->pw;
      x_wave = _st_->wave;
      x_cut = _st_->cut;
      x_res = _st_->res;
      x_osc = _voice__osc((&_st_->_i31), x_pitch, x_pw, x_wave);
      x_filter = _voice__svf((&_st_->_i32), x_osc, x_cut, x_res, 0 /* 0 */ );
      kk = x_filter;
      n = (fix_add(n, 65536 /* 1 */ ));
   }

   return 0 /* 0 */ ;
}



