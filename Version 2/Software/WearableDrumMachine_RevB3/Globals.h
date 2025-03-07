// Globals.h
// For Run-time variables (e.g., bool RECState;, int currentTrack;, arrays that hold track states, etc.) → into Globals.h/.cpp.
// Globals.h/.cpp (or a similarly named pair) contains all the “live state” the program uses at run time.

#ifndef GLOBALS_H
#define GLOBALS_H

// // step 1: define FSRs as a compile-time constant
// static constexpr int FSRs = 4;

#include "Config.h" // So that MAX_TRACKS, MAX_NOTES, etc. are known
#include "AudioSampleKickkhronos.h"
#include "AudioSampleHh1khronos.h"
#include "AudioSampleSnarekhronos.h"
#include "AudioSampleHh2khronos.h"
#include <Bounce.h> 
#include <Audio.h>    
#include <Wire.h>     
#include <SPI.h>      
#include <SD.h>   
#include <MIDI.h>
#include <SerialFlash.h> 
#include <ResponsiveAnalogRead.h> 
#include <RTClib.h> 

// -- FSR to MIDI Note Handling --
extern int FSRpin[]; 
extern const int FSRs; 
extern int Note[]; 
extern int counter[]; 
extern int VELMASK; 
extern int AFTERTHRESH; 
extern int THRESH; 
extern int VELTIME; 
extern int AFTERTIME; 
extern int MIDIMIN; 

// Baked-in "Memory" Audio Sample Handling --
extern int channel[]; 
extern const int SAMPLEs;
extern int counter2[];

// If you want the forward declarations (like the monolith had them literally):
void NoteOnSend(int);
void PolyTouchSend(int);
void SampleOnSend(int);

// Optional RTC
extern RTC_DS3231 RTC; 
extern bool HAS_RTC;
extern int lastMarkState;
extern int nextMarker;

// SD & MIDI file
extern unsigned long lastLoopCounter;
extern unsigned long loopCounter;
extern unsigned long startTime;
extern unsigned long lastTime;
extern String filename;
extern File file;

// WAV Browsing + Final Mixer
extern const int MAX_FILES;
extern String fileList[];
extern int totalFiles;
extern int browseIndex;
extern bool inBrowseMode;

extern bool previewActive;
extern unsigned long previewStart;

extern String fsrSamplePath[4];
extern String fsrFlashRaw[4];

// =============================================================
// Looper / Recording / MIDI State
// =============================================================
extern bool LEDdummyState[MAX_TRACKS]; 
extern bool RECState; 
extern bool startState; 
extern bool undoState; 
extern bool pushRotaryState; 
extern bool optA_state; 
extern bool clockTrigState; 

extern bool trackState[MAX_TRACKS]; 
extern bool muteTrack[2*MAX_TRACKS]; 
extern unsigned long TblinkTime[MAX_NOTES]; 
extern bool waitingFirstNote[MAX_SEQUENCES]; 
extern bool waitingFirstSeq; 

extern bool RECORDING; 
extern bool PLAY; 
extern unsigned long TIME; 
extern unsigned long beatTIME; 
extern unsigned long pressRECTime; 
extern unsigned long startLoopTime; 
extern unsigned long endLoopTime; 
extern unsigned long loopLenght; 
extern unsigned long beatLenght; 
extern unsigned long prevBeatLenght; 
extern unsigned long trigTime; 
extern unsigned long clockLenght; 
extern unsigned long clockTime; 
extern unsigned long firstClockTick; 
extern unsigned long firstTrigTick; 
extern byte clockTick; 
extern byte trigCount; 
extern byte incomingClock; 
extern bool metronomeActive; 
extern bool MIDI_ECHO; 
extern const int debounceTime; 
extern const int rotaryDebounceTime; 
extern int timeFrame; 
extern int currentTrack; 
extern int currentSequence; 

//============================================================
// Additional Sequencer / FSR / Preview Variables
//============================================================
extern byte MIDIdata1[MAX_NOTES][MAX_SEQUENCES];
extern byte MIDIdata2[MAX_NOTES][MAX_SEQUENCES];
extern byte MIDIchannel[MAX_NOTES][MAX_SEQUENCES];
extern byte noteTrack[MAX_NOTES][MAX_SEQUENCES];
extern unsigned long noteTime[MAX_NOTES][MAX_SEQUENCES];
extern int noteSeqCounter[MAX_SEQUENCES];
extern int noteNumber[MAX_SEQUENCES];

// For FSR analog reading:
extern ResponsiveAnalogRead* ANALOG_PINS[4];
//extern ResponsiveAnalogRead* ANALOG_PINS[FSRs];
// // step 2: declare the external pointer array with dimension FSRs
// extern ResponsiveAnalogRead* ANALOG_PINS[FSRs];

// For run-time states:
extern bool isPreviewing; // To track if a preview is in progress 
extern bool currentlyTransferring; // To prevent concurrent actions 


// Debugging or tracking playback states
extern bool flashPlayerStates[4];




// ... any other extern variables from the rest of your code ...

#endif // GLOBALS_H
