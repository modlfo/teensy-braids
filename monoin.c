#include "monoin.h"

int32_t _monoin_struct_0_init(_monoin_struct_0* st){
   st->pre = 0 /* 0. */ ;
   st->n4 = 0 /* 0. */ ;
   st->n3 = 0 /* 0. */ ;
   st->n2 = 0 /* 0. */ ;
   st->n1 = 0 /* 0. */ ;
   st->count = 0 /* 0. */ ;
   return 0 /* 0 */ ;
}

int32_t _monoin_struct_isGateOn_init(_monoin_struct_isGateOn* st){
   _monoin_struct_0_init(st);
   return 0 /* 0 */ ;
}

int32_t _monoin_struct_noteOff_init(_monoin_struct_noteOff* st){
   _monoin_struct_0_init(st);
   return 0 /* 0 */ ;
}

int32_t _monoin_struct_noteOn_init(_monoin_struct_noteOn* st){
   _monoin_struct_0_init(st);
   return 0 /* 0 */ ;
}

int32_t _monoin__noteOn(_monoin_struct_0* _st_, int32_t n){
   int32_t _tmp15;
   int32_t _tmp11;
   _tmp15 = _st_->count == 0 /* 0 */ ;
   if(_tmp15){
      _st_->n1 = n;
      _st_->pre = n;
   }
   else {
      int32_t _tmp14;
      _tmp14 = _st_->count == 65536 /* 1 */ ;
      if(_tmp14){
         _st_->n2 = n;
         _st_->pre = n;
      }
      else {
         int32_t _tmp13;
         _tmp13 = _st_->count == 131072 /* 2 */ ;
         if(_tmp13){
            _st_->n3 = n;
            _st_->pre = n;
         }
         else {
            int32_t _tmp12;
            _tmp12 = _st_->count == 196608 /* 3 */ ;
            if(_tmp12){
               _st_->n4 = n;
               _st_->pre = n;
            }
         }
      }
   }
   _tmp11 = (_st_->count <= 262144 /* 4 */ );
   if(_tmp11){
      _st_->count = (fix_add(_st_->count, 65536 /* 1 */ ));
   }
   return _st_->pre;
}

int32_t _monoin__noteOff(_monoin_struct_0* _st_, int32_t n){
   int32_t found;
   int32_t _tmp6;
   int32_t _tmp5;
   int32_t _tmp4;
   int32_t _tmp3;
   int32_t _tmp2;
   int32_t _tmp10;
   int32_t _tmp1;
   found = 0 /* 0 */ ;
   _tmp10 = n == _st_->n1;
   if(_tmp10){
      int32_t _tpl18;
      int32_t _tpl17;
      int32_t _tpl16;
      _tpl16 = _st_->n2;
      _tpl17 = _st_->n3;
      _tpl18 = _st_->n4;
      _st_->n1 = _tpl16;
      _st_->n2 = _tpl17;
      _st_->n3 = _tpl18;
      found = 65536 /* 1 */ ;
   }
   else {
      int32_t _tmp9;
      _tmp9 = n == _st_->n2;
      if(_tmp9){
         int32_t _tpl20;
         int32_t _tpl19;
         _tpl19 = _st_->n3;
         _tpl20 = _st_->n4;
         _st_->n2 = _tpl19;
         _st_->n3 = _tpl20;
         found = 65536 /* 1 */ ;
      }
      else {
         int32_t _tmp8;
         _tmp8 = n == _st_->n3;
         if(_tmp8){
            _st_->n3 = _st_->n4;
            found = 65536 /* 1 */ ;
         }
         else {
            int32_t _tmp7;
            _tmp7 = n == _st_->n4;
            if(_tmp7){
               found = 65536 /* 1 */ ;
            }
         }
      }
   }
   _tmp6 = (found && (_st_->count > 0 /* 0 */ ));
   if(_tmp6){
      _st_->count = (fix_sub(_st_->count, 65536 /* 1 */ ));
   }
   _tmp5 = _st_->count == 0 /* 0 */ ;
   if(_tmp5){
      _st_->pre = 0 /* 0 */ ;
   }
   _tmp4 = _st_->count == 65536 /* 1 */ ;
   if(_tmp4){
      _st_->pre = _st_->n1;
   }
   _tmp3 = _st_->count == 131072 /* 2 */ ;
   if(_tmp3){
      _st_->pre = _st_->n2;
   }
   _tmp2 = _st_->count == 196608 /* 3 */ ;
   if(_tmp2){
      _st_->pre = _st_->n3;
   }
   _tmp1 = _st_->count == 262144 /* 4 */ ;
   if(_tmp1){
      _st_->pre = _st_->n4;
   }
   return _st_->pre;
}

int32_t _monoin__isGateOn(_monoin_struct_0* _st_){
   int32_t _tmp0;
   if((_st_->count > 0 /* 0 */ )){
      _tmp0 = 65536 /* 1 */ ;
   }
   else {
      _tmp0 = 0 /* 0 */ ;
   }
   return _tmp0;
}

int32_t _monoin_(){
}



