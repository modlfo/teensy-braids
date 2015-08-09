#!/bin/bash
~/Development/vult/vultc.native -ccode -real fixed ~/Development/vult/test/blit.vult -o blit
~/Development/vult/vultc.native -ccode -real fixed ~/Development/vult/test/state_variable.vult -o svf
~/Development/vult/vultc.native -ccode -real fixed ~/Development/vult/test/voice.vult -o voice
~/Development/vult/vultc.native -ccode -real fixed ~/Development/vult/test/monoin.vult -o monoin
~/Development/vult/vultc.native -ccode -real fixed ~/Development/vult/test/adsr.vult -o adsr
cp ~/Development/vult/runtime/vultin.h .
cp ~/Development/vult/runtime/vultin.c .
