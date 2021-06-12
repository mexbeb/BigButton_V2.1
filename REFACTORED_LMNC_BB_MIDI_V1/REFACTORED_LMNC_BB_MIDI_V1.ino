//FROM AN ORIGINAL IDEA OF SAM BATTLE (LOOK MUM NO COMPUTER)
//
//REFACTORED CODE BY YUKONNOR
//
//ADDED [MIDI IN/AUDIT/SEQUENCE RESET WHEN CLOCK STOPS (to be implemented)] FUNCTIONS BY MEXBEB
//
//
//THE BIG BUTTON DRUM SEQUENCER/TRIGGER
//
// MIDI Input ... Pin 0 (RX) -> refer to standard MIDI connection with Arduino (6N138 Optoisolator + diode + resistor)

// Clock Input ... Pin 2 (available for "Interrupts")
// Clear Button .. Pin 12 (clears the entire loop sequence for the current channel and bank)
// Delete Button . Pin 10 (deletes the current step for the current channel and bank)
// Bank Button ... Pin 9 (each channel has 2 banks where alternative patterns can be recored and stored. this button toggles the bank for the current channeL)
// Big Button .... Pin A4 (this is the record button!)
// Reset Button .. Pin 11 (resets the current channel and back to step 1)
// Fill Button ... Pin 8 (while held, it will continuously play the channel your on)

// Channel Select Switch .. Pin (A0) (selects the current channel, 1-6, to modify)
// Step Length Knob ....... Pin (A1) (sets the amount of steps for all channels, 1-32)
// Shift Knob ............. Pin (A2) (set the amount of steps to shift the output of the current channel)

// Output 1 ... Pin A3
// Output 2 ... Pin 3
// Output 3 ... Pin 4
// Output 4 ... Pin 5
// Output 5 ... Pin 6
// Output 6 ... Pin 7

//Mode Switch (MIDI/ext CLK) ... Pin A5


#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
//USBMIDI_NAMESPACE::usbMidiTransport usbMIDI2(11);
//MIDI_NAMESPACE::MidiInterface<USBMIDI_NAMESPACE::usbMidiTransport> MIDI2((USBMIDI_NAMESPACE::usbMidiTransport&)usbMIDI2);

const byte    debug = 0;           // if 1, run the Serial Monitor and debug program

unsigned long time = 0;
const byte    debounce = 200;
const byte    clockstop = 1000; //setting 1 second as a stop timer for the clock
unsigned long highCLKlast;         //stores last time CLK was updated

// CLOCK IN
// Declared as volatile as it is shared between the ISR and the main program.
const int    clkInPin = 2;
volatile int clkInState;

// BIG BUTTON aka Record Button
const byte recordButtonPin = A4;
byte       recordButtonState = 0;

// DELETE BUTTON
const byte deleteButtonPin = 10;
byte       deletebuttonState = 0;

// RESET BUTTON
const byte resetButtonPin = 11;
byte       resetButtonState = 0;

// FILL BUTTON
const byte fillButtonPin = 8;
byte       fillButtonState = 0;
byte       fillState[6] = {0, 0, 0, 0, 0, 0}; // store whether the fill is on or off for each channel

// CLEAR BUTTON
const byte clearButtonPin = 12;
byte       clearButtonState = 0;

// BANK BUTTON
const byte bankButtonPin = 9;
byte       bankButtonState = 0;
byte       bankState[6] = {0, 0, 0, 0, 0, 0}; // stores whether the bank is off or on (0 or 6) for each channel (which then selects the pattern sequence).

// CLOCK STUFF
byte       currentStep  = 0;    // sets the shared global 'current step', used in the 'delete' function. Previously 'looper'
byte       currentStep1 = 0;
byte       currentStep2 = 0;
byte       currentStep3 = 0;
byte       currentStep4 = 0;
byte       currentStep5 = 0;
byte       currentStep6 = 0;

// SHIFT STUFF
const byte shiftPin = A2;
int        shiftPotRead = 0;
byte       shiftValue = 0;       // Value of how many beats to shift (0-8) - prev "KnobValue"
byte       lastShiftValue = 0;
byte       shiftValue1 = 0;
byte       shiftValue2 = 0;
byte       shiftValue3 = 0;
byte       shiftValue4 = 0;
byte       shiftValue5 = 0;
byte       shiftValue6 = 0;

// PATTERN LENGTH STUFF
const byte patLengthPin = A1;
int        patLengthPotRead = 0;
byte       steps = 0;             // length of sequence (1 - 32 steps)

// CHANNEL SELECT STUFF
const byte channelPotReadPin = A0;
int        channelPotRead = 0;
byte       currentChannel = 0;    // current channel (0-5)
byte       lastChannel = 0;

// OUTPUTS
const byte outPin1 = A3;           // pins for trigger and LED outputs
const byte outPin2 = 3;
const byte outPin3 = 4;
const byte outPin4 = 5;
const byte outPin5 = 6;
const byte outPin6 = 7;
const byte bankLEDPin = 13;             // pin TBD  for bank LED

//MODE SELECT SWITCH

const int MODESWITCH = A5; // 1 is MIDI, 0 is Ext.CLK (standard)
boolean MODE;

byte Sequence[12][32] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
};


//EXTRA DECLARATIONS

const int midi_notes[6] = {

  36, //C1  -> BASS DRUM
  38, //D1  -> SNARE
  42, //F#1 -> HI-HAT
  49, //C#2 -> CRASH
  51, //D#2 -> RIDE
  56  //G#2 -> COWBELL

};

int midiStates[6] = { 0, 0, 0, 0, 0, 0 };
int Outs[6] = {outPin1, outPin2, outPin3, outPin4, outPin5, outPin6,};

int SpeedState = 0;   // State of the speed potentiometer
int Steps = 24;       // Speed
int HalfStep = 12;
int StepFind = 0;

int PatternState = 0;     // State of the pattern length potentiometer
int Length = 4;          // Pattern length

int ChannelState = 0;     // State of the channel potentiometer
int Channel = 0;          // Channel
int ChannelChannels[6] = {0, 0, 0, 1, 1, 1};

int LastButtonState = HIGH;
int ButtonState = HIGH;
int ButtonPressed = HIGH;

int ClearState = HIGH;

int DeleteState = HIGH;

int FillState = HIGH;

int Fill[12] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};

int ResetState = HIGH;
int LastResetState = HIGH;

int BankState = HIGH;
int LastBankState = HIGH;


int bank = 0;

byte dummyState = 0;

int mutesState[6] = {deletebuttonState, resetButtonState, clearButtonState, fillButtonState, bankButtonState, dummyState};
int lastmuteState[6] = {0, 0, 0, 0, 0, 0};


void startupLED() {
  digitalWrite(bankLEDPin, HIGH);
  delay(200);
  digitalWrite(bankLEDPin, LOW);
  delay(180);
  digitalWrite(bankLEDPin, HIGH);
  delay(160);
  digitalWrite(bankLEDPin, LOW);
  delay(140);
  digitalWrite(bankLEDPin, HIGH);
  delay(120);
  digitalWrite(bankLEDPin, LOW);
  delay(100);
  digitalWrite(bankLEDPin, HIGH);
  delay(80);
  digitalWrite(bankLEDPin, LOW);
  delay(60);
  digitalWrite(bankLEDPin, HIGH);
  delay(40);
  digitalWrite(bankLEDPin, LOW);
  delay(20);
  digitalWrite(bankLEDPin, HIGH);
  delay(60);
  digitalWrite(bankLEDPin, LOW);

  digitalWrite(outPin1, HIGH);
  delay(100);
  digitalWrite(outPin2, HIGH);
  delay(100);
  digitalWrite(outPin3, HIGH);
  delay(100);
  digitalWrite(outPin4, HIGH);
  delay(100);
  digitalWrite(outPin5, HIGH);
  delay(100);
  digitalWrite(outPin6, HIGH);
  delay(100);
  digitalWrite(outPin1, LOW);
  digitalWrite(outPin2, LOW);
  digitalWrite(outPin3, LOW);
  digitalWrite(outPin4, LOW);
  digitalWrite(outPin5, LOW);
  digitalWrite(outPin6, LOW);

}

//  isr() - or Interupt Service Routine - quickly handle interrupts from the clock input
//  See "attachInterrupt" in Setup
//  If clock input is triggered (rising edge), set the reading of clock in to be HIGH



//UNCOMMENT THIS AND debugger(); for debugging purposes

//void debugger() {
//  if (debug == 1) {
//    Serial.print("Clock In = ");
//    Serial.println(clkInState);
//
//
//    Serial.print("Big Button = ");
//    Serial.print(recordButtonState);
//
//
//    Serial.print("Big Button = ");
//    Serial.print(recordButtonState);
//    //    Serial.print("  Bank Button = ");
//    //    Serial.print(ButtonBankSelectState[BankArrayNumber]);
//    Serial.print("  Clear Button = ");
//    Serial.print(clearButtonState);
//    Serial.print("  Fill Button = ");
//    Serial.print(fillButtonState);
//    Serial.print("  Reset Button = ");
//    Serial.print(resetButtonState);
//    Serial.print("  Delete Button = ");
//    Serial.println(deletebuttonState);
//
//
//    // POT TESTING
//    Serial.print("Current Channel = ");
//    Serial.print(currentChannel);
//    Serial.print("  Beat Shift = ");
//    Serial.print(shiftValue);           // Should be between 0 - 8
//    Serial.print("  Steps = ");
//    Serial.println(steps);             // Should be between 1 - 32
//
//    Serial.print("  Switch = ");
//    Serial.println(MODE);
//  }
//}

void isr() {

  clkInState = HIGH;

}


void setup() {

  MIDI.begin(11); //MIDI CHANNEL 11, BUT CAN BE SET TO ANYTHING FROM 1 TO 16
  MIDI.setHandleNoteOn(NoteOn);
  pinMode(outPin1, OUTPUT);
  pinMode(outPin2, OUTPUT);
  pinMode(outPin3, OUTPUT);
  pinMode(outPin4, OUTPUT);
  pinMode(outPin5, OUTPUT);
  pinMode(outPin6, OUTPUT);
  pinMode(bankLEDPin , OUTPUT);
  pinMode(clkInPin, INPUT);     //SHOULDNT USE PULLUP as Clock input is high (use hardware pulldown)

  pinMode(recordButtonPin, INPUT_PULLUP); //ALL INPUTS ARE PULLUP (EXCEPT CLOCK), NO NEED TO USE EXTERNAL RESISTORS -> LESS SOLDERING IS ALWAYS GOOD! :)
  pinMode(deleteButtonPin, INPUT_PULLUP); //IF YOU HAVE EXTERNAL RESISTORS SET THESE INPUTS TO 'INPUT' (DELETE THE _PULLUP PART)
  pinMode(clearButtonPin, INPUT_PULLUP);  //AND SET ALL THE STATES OF THE CORRESPONDING BUTTONS FROM HIGH TO LOW AND VICEVERSA
  pinMode(bankButtonPin, INPUT_PULLUP);   //FROM THE RECORD BUTTON
  pinMode(resetButtonPin, INPUT_PULLUP);
  pinMode(fillButtonPin, INPUT_PULLUP);   //..TO THIS BUTTON

  pinMode(patLengthPin, INPUT_PULLUP);  // IF POTS ARE REVERSED, REVERSE CABLES ON PIN 1 & PIN 3
  pinMode(shiftPin, INPUT_PULLUP);      // OR CHANGE FROM INPUT_PULLUP TO INPUT (AS YOU WOULD DO FOR THE BUTTONS) [not sure?]

  // read pattern length pot value on start up
  // PUT THIS IN A FUNCTION

  patLengthPotRead = analogRead(patLengthPin);
  if (0 < patLengthPotRead) {
    steps = 1;
  }
  if (150 < patLengthPotRead) {
    steps = 2;
  }
  if (300 < patLengthPotRead) {
    steps = 4;
  }
  if (500 < patLengthPotRead) {
    steps = 8;
  }
  if (750 < patLengthPotRead) {
    steps = 16;
  }
  if (1000 < patLengthPotRead) {
    steps = 32;
  }

  startupLED();


  // Interupt Service Routine for Clock Input
  // Syntax: attachInterrupt(digitalPinToInterrupt(pin), ISR, mode)
  // ISR: the interupt service routine to call when the interupt occurs (when clock pin goes high)
  // Rising: trigger when the pin goes from low to high

  attachInterrupt(digitalPinToInterrupt(2), isr, RISING);

  // if debug mode is on, run the serial monitor
  if (debug == 1) {
    Serial.begin(9600);
  }


} //END SETUP


u8 clk;

void modecheck() {

  MODE = digitalRead(MODESWITCH); // 1 is MIDI, 0 is Ext.CLK (standard)

  switch (MODE) {
    case (1):
      midiclock();
      break;

    case (0):
      extclock();
      break;
  }
}


void extclock() {

  // Get button readings
  recordButtonState = digitalRead(recordButtonPin);
  deletebuttonState = digitalRead(deleteButtonPin);
  clearButtonState = digitalRead(clearButtonPin);
  resetButtonState = digitalRead(resetButtonPin);
  fillButtonState = digitalRead(fillButtonPin);
  bankButtonState = digitalRead(bankButtonPin);


  if (clkInState == HIGH) {
    currentStep  = (currentStep + 1);
    currentStep1 = (currentStep1 + 1);
    currentStep2 = (currentStep2 + 1);
    currentStep3 = (currentStep3 + 1);
    currentStep4 = (currentStep4 + 1);
    currentStep5 = (currentStep5 + 1);
    currentStep6 = (currentStep6 + 1);

    // Output each channels sequence
    // when I moved out of if statement, it created longer duty cycles (fill would run into itself for each beat, leading to constant ON state)
    digitalWrite(outPin1, Sequence[0 + bankState[0]] [currentStep1 + shiftValue1] || fillState[0]); // Logical OR results in a true if either of the two operands is true.
    digitalWrite(outPin2, Sequence[1 + bankState[1]] [currentStep2 + shiftValue2] || fillState[1]);
    digitalWrite(outPin3, Sequence[2 + bankState[2]] [currentStep3 + shiftValue3] || fillState[2]);
    digitalWrite(outPin4, Sequence[3 + bankState[3]] [currentStep4 + shiftValue4] || fillState[3]);
    digitalWrite(outPin5, Sequence[4 + bankState[4]] [currentStep5 + shiftValue5] || fillState[4]);
    digitalWrite(outPin6, Sequence[5 + bankState[5]] [currentStep6 + shiftValue6] || fillState[5]);
    delay(10);              //do this with a time diff?
    digitalWrite(outPin1, LOW);
    digitalWrite(outPin2, LOW);
    digitalWrite(outPin3, LOW);
    digitalWrite(outPin4, LOW);
    digitalWrite(outPin5, LOW);
    digitalWrite(outPin6, LOW);

    clkInState = LOW;      // isr triggers on rising edge of clock signal. isr sets clkInState to be HIGH, we so need to set to LOW here.


  }


  // RECORD BUTTON - record the sequence of the current pattern
  if (recordButtonState == LOW && millis() - time > debounce) {
    // Sequence[currentChannel + bankState[currentChannel]] [currentStep1 + 1 + newShiftValue1] = 1;  -- tried it this way, but doesn't work with "shift" (currentStep is dif for each channel)
    if (currentChannel == 0) {
      Sequence[currentChannel + bankState[currentChannel]] [currentStep1 + 1 + shiftValue1] = 1;
    }
    else if (currentChannel == 1) {
      Sequence[currentChannel + bankState[currentChannel]] [currentStep2 + 1 + shiftValue2] = 1;
    }
    else if (currentChannel == 2) {
      Sequence[currentChannel + bankState[currentChannel]] [currentStep3 + 1 + shiftValue3] = 1;
    }
    else if (currentChannel == 3) {
      Sequence[currentChannel + bankState[currentChannel]] [currentStep4 + 1 + shiftValue4] = 1;
    }
    else if (currentChannel == 4) {
      Sequence[currentChannel + bankState[currentChannel]] [currentStep5 + 1 + shiftValue5] = 1;
    }
    else if (currentChannel == 5) {
      Sequence[currentChannel + bankState[currentChannel]] [currentStep6 + 1 + shiftValue6] = 1;
    }

    time = millis();
  }



  // Turn on Bank LED if bank is on for the current channel
  digitalWrite(bankLEDPin, bankState[currentChannel]);


  // BANK BUTTON
  // If the bank button is pressed set the bank state for the current channel to 6 if 0 and vice versa
  // Bank is 0 or 6 to add 6 to get the correct pattern (current channel + 6)
  if (bankButtonState == LOW && millis() - time > debounce) {
    if (bankState[currentChannel] == 6) {
      bankState[currentChannel] = 0;
    }
    else {
      bankState[currentChannel] = 6;
    }
    time = millis();
  }

  // SHIFT POT
  // Read the beat shift pot and determine how many beats to shift the pattern for the current channel
  shiftPotRead = analogRead(shiftPin);
  if (0 < shiftPotRead)   {
    shiftValue = 0;
  }
  if (127 < shiftPotRead) {
    shiftValue = 1;
  }
  if (254 < shiftPotRead) {
    shiftValue = 2;
  }
  if (383 < shiftPotRead) {
    shiftValue = 3;
  }
  if (511 < shiftPotRead) {
    shiftValue = 4;
  }
  if (638 < shiftPotRead) {
    shiftValue = 5;
  }
  if (767 < shiftPotRead) {
    shiftValue = 6;
  }
  if (895 < shiftPotRead) {
    shiftValue = 7;
  }
  if (1000 < shiftPotRead) {
    shiftValue = 8;
  }

  // if the shift value changes, set the new shift value for the current channel
  if (shiftValue != lastShiftValue) {
    if (currentChannel == 0) {
      shiftValue1 = shiftValue;
    }
    if (currentChannel == 1) {
      shiftValue2 = shiftValue;
    }
    if (currentChannel == 2) {
      shiftValue3 = shiftValue;
    }
    if (currentChannel == 3) {
      shiftValue4 = shiftValue;
    }
    if (currentChannel == 4) {
      shiftValue5 = shiftValue;
    }
    if (currentChannel == 5) {
      shiftValue6 = shiftValue;
    }
  }


  // CHANNEL SELECT POT
  channelPotRead = analogRead(channelPotReadPin);
  if (20 > channelPotRead)  {
    currentChannel = 0;
  }
  if (170 < channelPotRead) {
    currentChannel = 1;
  }
  if (240 < channelPotRead) {
    currentChannel = 2;
  }
  if (420 < channelPotRead) {
    currentChannel = 3;
  }
  if (750 < channelPotRead) {
    currentChannel = 4;
  }
  if (1000 < channelPotRead) {
    currentChannel = 5;
  }


  // CLEAR PATTERN BUTTON
  // If the clear button is pressed, remove all triggers from the current pattern
  if (clearButtonState == LOW) {
    for (int i = 1; i < 32; i++) {
      Sequence[currentChannel + bankState[currentChannel]][i] = 0;
    }
  }

  // FILL BUTTON
  // If the fill button pressed, the current channel should go in to fill mode
  // Fill can only be on one channel at a time
  if (fillButtonState == LOW) {
    fillState[currentChannel] = 1;
    if (currentChannel != lastChannel) {
      fillState[lastChannel] = 0;
    }
  }
  else {
    fillState[currentChannel] = 0;
  }

  // DELETE BUTTON -> FUNCTION TO BE DELETED/SUBSTITUTED (?)
  // If the delete button is pressed, do set step of the current pattern to be 0
  if (deletebuttonState == LOW && millis() - time > debounce) {
    Sequence[currentChannel + bankState[currentChannel]][currentStep + 1] = 0;
    time = millis();
  }

  // RESET BUTTON
  // If the reset button is pressed, set the current steps to 0 (start from step 1 of the patterns)
  if (resetButtonState == LOW && millis() - time > debounce) {
    currentStep = 0;
    currentStep1 = 0;
    currentStep2 = 0;
    currentStep3 = 0;
    currentStep4 = 0;
    currentStep5 = 0;
    currentStep6 = 0;
    time = millis();
  }


  // Determine how many steps the looping pattern is
  patLengthPotRead = analogRead(patLengthPin);
  if (0 < patLengthPotRead) {
    steps = 1;
  }
  if (150 < patLengthPotRead) {
    steps = 2;
  }
  if (300 < patLengthPotRead) {
    steps = 4;
  }
  if (500 < patLengthPotRead) {
    steps = 8;
  }
  if (750 < patLengthPotRead) {
    steps = 16;
  }
  if (1000 < patLengthPotRead) {
    steps = 32;
  }

  //this bit starts the sequence over again
  if (currentStep >= steps) {
    currentStep = 0;
  }
  if ((currentStep1  + shiftValue1) >= steps) {
    currentStep1 = 0;
  }
  if ((currentStep2  + shiftValue2) >= steps) {
    currentStep2 = 0;
  }
  if ((currentStep3  + shiftValue3) >= steps) {
    currentStep3 = 0;
  }
  if ((currentStep4  + shiftValue4) >= steps) {
    currentStep4 = 0;
  }
  if ((currentStep5  + shiftValue5) >= steps) {
    currentStep5 = 0;
  }
  if ((currentStep6  + shiftValue6) >= steps) {
    currentStep6 = 0;
  }

  lastChannel = currentChannel;              //update the lastChannel, used for the Fill button
  lastShiftValue = shiftValue;


  //  debugger();

}

// END

void NoteOn(byte channel, byte pitch, byte velocity) {

  //  digitalWrite(bankLEDPin, lastmuteState[currentChannel]);

  for (int i = 0; i < 6; i++) {

    if (pitch == midi_notes[i]) {

      digitalWrite(Outs[i], HIGH);
      delay(1);
      digitalWrite(Outs[i], LOW);
    }
    else if (velocity == 0) {

      digitalWrite(Outs[i], LOW);
    }

  }
}

void midiclock() {


  recordButtonState = digitalRead(recordButtonPin);

  channelPotRead = analogRead(channelPotReadPin);
  if (20 > channelPotRead)  {
    currentChannel = 0;
  }
  if (170 < channelPotRead) {
    currentChannel = 1;
  }
  if (240 < channelPotRead) {
    currentChannel = 2;
  }
  if (420 < channelPotRead) {
    currentChannel = 3;
  }
  if (750 < channelPotRead) {
    currentChannel = 4;
  }
  if (1000 < channelPotRead) {
    currentChannel = 5;
  }


  //AUDIT MODE: When in MIDI mode, press the big button to listen to the sounds of the corresponding channel selected with the rotary switch

  if (recordButtonState == LOW && millis() - time > debounce) {

    digitalWrite(Outs[currentChannel], HIGH);
    time = millis();


  } else  {
    digitalWrite(Outs[currentChannel], LOW);

  }



  MIDI.read();

}

void loop() {
  modecheck();
}
