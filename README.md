teensy-braids
=============

A port of the Mutable Instruments Braids to the Teensy board

This project extracts the Mutable Instruments Braids macro oscillator and uses the analog output (pin A14) of the teensy as audio output.

Once flashed, the Teensy receives MIDI-USB messages. The oscillator produces sound continuously and it changes the pitch based on the note-on message. The oscillator type, color and timbre are controlled with the messages CC32, CC33 and CC34 respectively.

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
this will automatically open the Teensy loader program.