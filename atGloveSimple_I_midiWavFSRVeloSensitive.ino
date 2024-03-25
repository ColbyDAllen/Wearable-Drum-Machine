/**
  3.25.2024:
  This file is outdated.
  New code for 
    FSR sensors, 
    MIDI File Storage, and 
    MIDI Looper 
  can be found under "Version 1" directory.
*/

// Libraries
#include "AudioSampleKickkhronos.h"
#include "AudioSampleHh1khronos.h"
#include "AudioSampleSnarekhronos.h"
#include "AudioSampleHh2khronos.h"
#include <Bounce.h> // Don't think I still need this. TBD.
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlayMemory          playMem3;       //xy=248.75,260.0000057220459
AudioPlayMemory          playMem4;       //xy=248.75,325.0000057220459
AudioPlayMemory          playMem2;       //xy=251.25,193.7500057220459
AudioPlayMemory          playMem1;       //xy=256.25,135.0000057220459
AudioMixer4              mixer1;         //xy=498.75000762939453,230.00000381469727
AudioOutputI2S           i2s1;           //xy=735.0000114440918,230.00000190734863
AudioConnection          patchCord1(playMem3, 0, mixer1, 2);
AudioConnection          patchCord2(playMem4, 0, mixer1, 3);
AudioConnection          patchCord3(playMem2, 0, mixer1, 1);
AudioConnection          patchCord4(playMem1, 0, mixer1, 0);
AudioConnection          patchCord5(mixer1, 0, i2s1, 0);
AudioConnection          patchCord6(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=433.75000762939453,493.7500057220459
// GUItool: end automatically generated code

//// GLOBAL VARIABLES ////
// INDEX # -->  0     1     2    3
int FSRpin[] = {A17, A16, A15, A14};  /*"Relevant analog pins connected to FSRs"
                                      --"FSRpin[]" later gets read by "analogRead(..." function 
                                      --in void loop(){} section! */
const int FSRs = 4;                   /*number of FSRs // 
                                      // --"FSRs" later called in Condition 
                                      // --part of For-Loop in void loop(){} section!*/
int Note [] = {60, 61, 62, 63};       /*MIDI note numbers ... listed in order of FSRpin*/
int counter [FSRs];                   /*"counter[4]", in other words.*/
int VELMASK = 0;
int ATMASK = 0;
int AFTERTHRESH = 50;         /*"Analog sensor value above which aftertouch messages are sent"
                              50 in decimal = 0110010 in binary*/

int THRESH = 45;              /*"Analog sensor value above which note / velocity is recognised"
                              --"THRESH", an integer-type variable, is later 
                              --called in void loop() {...}
                              45 in decimal = 0101101 in binary)*/

int VELTIME = 500;            /*"Counter value at which point velocity is sampled ...
                              counter is zero when first touched, velocity is sampled X ticks later
                              500 ticks sounds like a lot, but the teensy LC is clocked at 48Mhz"       
                              500 in decimal (0111110100 binary)*/

int AFTERTIME = 2500;         /*"Counter value at which point aftertouch is sampled ...
                              every X ticks of a touch, until released ...
                              you don't want too many aftertouch messages per touch, 
                              and 2500 gives a surprising number"
                              2500 in decimal (0100111000100 in binary)*/
int MIDIMIN = 20;             /*bottom MIDI value for the MIDI velocity AND aftertouch messages*/
void NoteOnSend (int);        /*One of 3 main functions in addition to void setup(); and void loop();*/
void PolyTouchSend (int);     /*One of 3 main functions in addition to void setup(); and void loop();*/

////following global variables relating to Wav2Sketch/output audio:
int channel [] = {0, 1, 2, 3};                /*Mixer channels for wav2sketch audio samples, listed 
                                              in order of their respective .wav samples & analog pins*/
const int SAMPLEs = 4;
int counter2 [SAMPLEs];  
void SampleOnSend (int);                      /*One of 3 main functions in addition to void setup(); and void loop(); */
void gain (unsigned int channel, float gain); /*Function that goes within void SampleOnSend(int); function. 
                                              Confusing to set up because the names for second parameter 
                                              "gain" varied as "level" in the ArduinoIDE>TeensyExample sketches*/

void setup() {
  Serial.begin (32500);

  AudioMemory(10);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  mixer1.gain(0, 0.4);
  mixer1.gain(1, 0.4);
  mixer1.gain(2, 0.4);
  mixer1.gain(3, 0.4);
}

void loop () {
  for (int i = 0; i < FSRs; i++) {
    int FSRRead = analogRead(FSRpin[i]);
    if (FSRRead > THRESH) {
      counter[i] ++;
      if (!(VELMASK & (1 << i)) && (counter[i] == VELTIME)) {   /*"(1 << i)" means "(1x2)^i", and is: 1 if 
                                                                i=0, 2 if i=1, 4 if i=2, or 8 if i=3*/
        VELMASK |= (1 << i);                   
        counter [i] = 0;
        NoteOnSend (i);
        SampleOnSend (i);                                       /*Key to getting variable-velocity wav2sketch 
                                                                samples playing on Adrian's FSR sketch*/
        }
      if (counter [i] == AFTERTIME) {
        counter [i] = 0;
        PolyTouchSend(i);
      }
    }
    else {                                                      /*When FSRRead(0-1023) is NOT greater than 45, 
                                                                do this: {...}*/
      if (VELMASK & (1 << i)) {          
        usbMIDI.sendNoteOff (Note[i], 0, 10); 
        VELMASK &= ~ (1 << i);                 
        counter [i] = 0;
      }
    }
  }
}

void NoteOnSend (int j) {
  int FSRRead = analogRead(FSRpin [j]);                         /*"int FSRRead": range of 0 to 1023*/
  int velocity = map (FSRRead, 0, 800, MIDIMIN, 127);           /*For input range why not 0 to 1023?*/
  usbMIDI.sendNoteOn (Note[j], velocity, 10);
}


void SampleOnSend (int k) {                                     /*Function added to Adrian's sketch for getting 
                                                                audio samples to play simulataneously with existing 
                                                                MIDI output signal coded in void 
                                                                NoteOnSend(int j) function*/
  analogReadResolution(7);
  int FSRRead2 = analogRead(FSRpin [k]);
  float gain = map (FSRRead2, 0, 127, .2, 1.0);
  mixer1.gain(channel[k], gain);
  if (analogRead(A17) >= THRESH) {
    playMem1.play(AudioSampleHh2khronos); }
  if (analogRead(A16) >= THRESH) {
    playMem2.play(AudioSampleSnarekhronos); }
  if (analogRead(A15) >= THRESH) {
    playMem3.play(AudioSampleHh1khronos); }
  if (analogRead(A14) >= THRESH) {
    playMem4.play(AudioSampleKickkhronos); }
}

void PolyTouchSend (int j) {                                     /*Don't think this is needed, but why mess with 
                                                                  what's working right?*/
  int FSRRead = analogRead(FSRpin [j]);
  if (FSRRead > AFTERTHRESH) {
    int aftertouch = map (FSRRead, 0, 800, MIDIMIN, 127);        /*Again, why input range of 0-800?*/
    usbMIDI.sendPolyPressure (Note[j], aftertouch, 10);
  }
}

/* 
Citations and Helpful Links:
1-- https://www.pjrc.com/teensy/gui/ --
      "Mixer" Function description listed in the lovely Teensy Audio Design Tool  
  gain(channel, level);
  Adjust the amplification or attenuation. "channel" must be 0 to 3. "level" 
  may be any floating point number from 0 to 32767.0. 1.0 passes the signal 
  through directly. Level of 0 shuts the channel off completely. 
  Between 0 to 1.0 attenuates the signal, and above 1.0 amplifies it. 
  Negative numbers may also be used, to invert the signal. All 4 channels 
  have separate gain settings.
2--https://forum.pjrc.com/threads/31797-Teensy-FSR-based-MIDI-controller--
      THANK YOU Adrian!!! The majority of the code used in this sketch was drafted from Adrian's FSR MIDI sketch 
  that can be found at the above link.
*/
