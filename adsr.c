#include "adsr.h"

int32_t _adsr_struct_0_init(_adsr_struct_0* st){
   st->value = 0 /* 0. */ ;
   st->sustainLevel = 0 /* 0. */ ;
   st->state = 0 /* 0. */ ;
   st->releaseRate = 0 /* 0. */ ;
   st->gate = 0 /* 0. */ ;
   st->decayRate = 0 /* 0. */ ;
   st->attackRate = 0 /* 0. */ ;
   st->_i1_pre_x = 0 /* 0. */ ;
   st->_i0_count = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _adsr_struct_1_init(_adsr_struct_1* st){
   st->pre_x = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _adsr_struct_2_init(_adsr_struct_2* st){
   st->count = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _adsr_struct_adsr_init(_adsr_struct_adsr* st){
   _adsr_struct_0_init(st);
   return 0 /* 0 */ ;
}

int32_t _adsr_struct_change_init(_adsr_struct_change* st){
   _adsr_struct_1_init(st);
   return 0 /* 0 */ ;
}

int32_t _adsr_struct_each_init(_adsr_struct_each* st){
   _adsr_struct_2_init(st);
   return 0 /* 0 */ ;
}

int32_t _adsr__each(_adsr_struct_2* _st_, int32_t n){
   int32_t ret;
   ret = _st_->count == 0 /* 0 */ ;
   _st_->count = ((fix_add(_st_->count, 65536 /* 1 */ )) % n);
   return ret;
}

int32_t _adsr__change(_adsr_struct_1* _st_, int32_t x){
   int32_t v;
   v = (_st_->pre_x != x);
   _st_->pre_x = x;
   return v;
}

int32_t _adsr__adsr(_adsr_struct_0* _st_, int32_t input, int32_t attack, int32_t decay, int32_t sustain, int32_t release){
   int32_t up;
   int32_t rate;
   int32_t edge;
   int32_t down;
   int32_t current;
   int32_t _tmp14;
   int32_t _tmp13;
   int32_t _tmp12;
   int32_t _tmp11;
   int32_t SUSTAIN;
   int32_t RELEASE;
   int32_t IDLE;
   int32_t DECAY;
   int32_t ATTACK;
   IDLE = 0 /* 0 */ ;
   ATTACK = 65536 /* 1 */ ;
   DECAY = 131072 /* 2 */ ;
   SUSTAIN = 196608 /* 3 */ ;
   RELEASE = 262144 /* 4 */ ;
   int32_t _i0_ret;
   int32_t _i0_n;
   _i0_n = 2097152 /* 32 */ ;
   _i0_ret = _st_->_i0_count == 0 /* 0 */ ;
   _st_->_i0_count = ((fix_add(_st_->_i0_count, 65536 /* 1 */ )) % _i0_n);
   _tmp13 = _i0_ret;
   if(_tmp13){
      if(attack == 0 /* 0 */ ){
         _st_->attackRate = 65536 /* 1 */ ;
      }
      else {
         _st_->attackRate = (fix_div(6553600 /* 100 */ , (fix_mul(attack, 28901376 /* 441 */ ))));
      }
      if(decay == 0 /* 0 */ ){
         _st_->decayRate = 65536 /* 1 */ ;
      }
      else {
         _st_->decayRate = (fix_div((fix_mul(6553600 /* 100 */ , (fix_sub(65536 /* 1 */ , sustain)))), (fix_mul(decay, 28901376 /* 441 */ ))));
      }
      if((release == 0 /* 0 */  || sustain == 0 /* 0 */ )){
         _st_->releaseRate = 65536 /* 1 */ ;
      }
      else {
         _st_->releaseRate = (fix_div((fix_mul(6553600 /* 100 */ , sustain)), (fix_mul(release, 28901376 /* 441 */ ))));
      }
   }
   _st_->sustainLevel = sustain;
   current = (input > 32768 /* 0.5 */ );
   edge = 0 /* 0 */ ;
   rate = 0 /* 0 */ ;
   int32_t _i1_x;
   int32_t _i1_v;
   _i1_x = current;
   _i1_v = (_st_->_i1_pre_x != _i1_x);
   _st_->_i1_pre_x = _i1_x;
   _tmp12 = _i1_v;
   if(_tmp12){
      if(current){
         _st_->gate = 65536 /* 1 */ ;
      }
      else {
         _st_->gate = 0 /* 0 */ ;
      }
      edge = 65536 /* 1 */ ;
   }
   up = (edge && _st_->gate);
   _tmp14 = fix_not(_st_->gate);
   down = (edge && _tmp14);
   _tmp11 = _st_->state == IDLE;
   if(_tmp11){
      if(up){
         _st_->state = ATTACK;
      }
   }
   else {
      int32_t _tmp10;
      _tmp10 = _st_->state == ATTACK;
      if(_tmp10){
         int32_t _tmp4;
         _tmp4 = (_st_->value >= 65536 /* 1 */ );
         if(_tmp4){
            _st_->state = DECAY;
         }
         if(down){
            _st_->state = RELEASE;
         }
         rate = _st_->attackRate;
      }
      else {
         int32_t _tmp9;
         _tmp9 = _st_->state == DECAY;
         if(_tmp9){
            int32_t _tmp5;
            _tmp5 = (_st_->value <= _st_->sustainLevel);
            if(_tmp5){
               _st_->state = SUSTAIN;
            }
            if(down){
               _st_->state = RELEASE;
            }
            rate = (fix_minus(_st_->decayRate));
         }
         else {
            int32_t _tmp8;
            _tmp8 = _st_->state == SUSTAIN;
            if(_tmp8){
               if(down){
                  _st_->state = RELEASE;
               }
               rate = 0 /* 0 */ ;
               _st_->value = _st_->sustainLevel;
            }
            else {
               int32_t _tmp7;
               _tmp7 = _st_->state == RELEASE;
               if(_tmp7){
                  int32_t _tmp6;
                  _tmp6 = (_st_->value <= 0 /* 0 */ );
                  if(_tmp6){
                     _st_->state = IDLE;
                  }
                  if(up){
                     _st_->state = ATTACK;
                  }
                  rate = (fix_minus(_st_->releaseRate));
               }
            }
         }
      }
   }
   _st_->value = (fix_add(rate, _st_->value));
   _st_->value = fix_clip(_st_->value, 0 /* 0 */ , 65536 /* 1 */ );
   return _st_->value;
}

int32_t _adsr_(){
}



