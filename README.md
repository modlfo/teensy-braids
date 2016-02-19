teensy-braids
=============

A port of the Mutable Instruments Braids to the Teensy board

This project extracts the Mutable Instruments Braids macro oscillator and uses the analog output of the teensy as audio output.

You can find the code in the main.cpp file.

=============

### Compile

First install the Teensyduino tools. The define the variable ARDUINOPATH. In my case for OSX:

```
$ export ARDUINOPATH=/Applications/Arduino.app/Contents/Java
```

then use make

```
$ make
```
