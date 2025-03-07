// Config.h
// Forcompile-time constants (the #defines and const byte pin definitions)
// Typically, Config.h or “configuration” headers hold macros (#define) or const declarations that never change at run time. They are set in stone at compile time.

#ifndef CONFIG_H
#define CONFIG_H

//===================================================================
// Includes & Setup
//===================================================================
#include <Arduino.h>
#include <Bounce.h> 
#include <Audio.h> 
#include <Wire.h> 
#include <SPI.h> 
#include <SD.h>   // or #include <SdFat.h> if you prefer
//#include <SdFat.h>
#include <MIDI.h> 
//#include <usb_midi.h>
#include <SerialFlash.h> 
#include <ResponsiveAnalogRead.h> 
#include <RTClib.h> // if you want RTC usage

// Then your pin definitions, constants, etc.
#include "core_pins.h" // if your original code used it, or anything else

// ------------------------------------------------------
// Optional RTC
// ------------------------------------------------------
#define PLACE_MARKER_PIN 9

// ------------------------------------------------------
// MIDI Macros
// ------------------------------------------------------
#define NOTE_OFF_EVENT          0x80
#define NOTE_ON_EVENT           0x90
#define CONTROL_CHANGE_EVENT    0xB0
#define PITCH_BEND_EVENT        0xE0
#define AFTER_TOUCH_POLY_EVENT  0xA0
#define RECORDING_TIMEOUT       120000
#define FILE_FLUSH_INTERVAL     400

//===================================================================
// WAV Browsing + Final Mixer
//===================================================================
#define sampleBrowsePin 24 // direct match to snippet

//=================================================================== 
// Additional Looper / Recording / MIDI 
//=================================================================== 
#define MAX_TRACKS 4 
#define MAX_NOTES 500 
#define MAX_SEQUENCES 4
#define DISABLE_THRU 
#define METRONOME_NOTE 64 
#define METRONOME_CHANNEL 10 
// #define SEND_INT_CLOCK 
// #define EXT_CLOCK 
const byte RECPin = 32; 
const byte startPin = 25; 
const byte clockTrigPin = 27; 
const byte pushRotaryPin= 16; 
const byte undoPin = 31; 
const byte trackPin[MAX_TRACKS] = {28,29,3,2}; 
const byte optAPin = 34; 
const byte optBPin = 33; 
const byte RECLEDPin = 36; 
const byte PLAYledPin = 37; 
const byte trackLEDPin[MAX_TRACKS] = {15,17,5,4}; 


// ... the rest of your #defines, pin mappings, etc.

#endif // CONFIG_H