# BigButton_V2.1
yukonnor LMNC big button trigger/sequencer refactored code with MIDI IN, Audit mode (triggering by pressing button) and more to come

INPUT_PULLUP VERSION: please read the code on how to convert to external resistor version of the big button

This version of the Big Button uses internal pull-up resistor on the Arduino, thus having less cost (and less soldering to do).
MIDI IN is added by using <MIDI.h> library, needs a standard circuit to be added (http://4.bp.blogspot.com/-0hrsk6ETAAQ/TyDZjNV1V-I/AAAAAAAAAI8/BQChPc_iGj0/s1600/Arduino+Midi+In.jpg)

A switch allows you to select between MIDI IN mode (notes sent by MIDI are played) and external clock mode (as the original Big Button).

In MIDI IN mode, AUDIT mode is present, in which you can play the samples of the corresponding selected channel by pressing the big button, useful to test the sounds or to add hits during MIDI playback.

Standard MIDI mapping is used:

  36, //C1  -> BASS DRUM
  38, //D1  -> SNARE
  42, //F#1 -> HI-HAT
  49, //C#2 -> CRASH
  51, //D#2 -> RIDE
  56  //G#2 -> COWBELL
  
MIDI channel is set to 11, but you can edit in the code, choosing from 1 to 16.


Pin mapping on the arduino is different:

MIDI Input ... Pin 1 (RX) -> refer to standard MIDI connection with Arduino (6N138 Optoisolator + diode + resistor)

Clock Input ... Pin 2 
Clear Button .. Pin 12 
Delete Button . Pin 10 
Bank Button ... Pin 9 
Big Button .... Pin A4 
Reset Button .. Pin 11 
Fill Button ... Pin 8

Channel Select Switch .. Pin (A0) 
Step Length Knob ....... Pin (A1) 
Shift Knob ............. Pin (A2)

Output 1 ... Pin A3
Output 2 ... Pin 3
Output 3 ... Pin 4
Output 4 ... Pin 5
Output 5 ... Pin 6
Output 6 ... Pin 7

Mode Switch (MIDI/ext CLK) ... Pin A5

Tested and working on an Arduino Leonardo, taking up to 37% of the memory. It should work on others Arduino boards too, but I did not tested it.


More to be added:

·Auto reset when clock is stopped for more than a certain period of time
·Delete button edit, it will be used for other things (I don't know yet for what)




Main idea by Sam Battle (LMNC)
Refactored code by Yukonnor


ENJOY!