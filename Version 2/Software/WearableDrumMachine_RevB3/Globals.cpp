// Globals.cpp

#include "Globals.h"
#include <MIDI.h> 
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


//===================================================================
// FSR to MIDI Note Handling
//===================================================================
int FSRpin[] = {A17, A16, A15, A14}; 
const int FSRs = 4; 
int Note[] = {60, 61, 62, 63}; 
int counter[FSRs] = {0,0,0,0}; 
int VELMASK = 0; 
int AFTERTHRESH = 50; 
int THRESH = 45; 
int VELTIME = 500; 
int AFTERTIME = 2500; 
int MIDIMIN = 20;

//===================================================================
// Baked-in "Memory" Audio Sample Handling
//===================================================================
int channel[] = {0,1,2,3}; 
const int SAMPLEs = 4; 
int counter2[SAMPLEs]; // can initialize with {0,0,0,0} if desired

// We'll define the actual SampleOnSend(...) function body in a .cpp 
// that deals with triggering memory playback, e.g., SampleHandler.cpp.


// The monolith also had "void NoteOnSend(int); void PolyTouchSend(int);"
// But we typically don't define functions in a .cpp that only has global data.
// We'll define those actual function *bodies* in FSRHandler.cpp or similar.

//===================================================================
// Optional RTC
//===================================================================
RTC_DS3231 RTC; 
bool HAS_RTC = false;
int lastMarkState = 0; 
int nextMarker = 1; 

//===================================================================
// SD & MIDI file
//===================================================================
unsigned long lastLoopCounter = 0;
unsigned long loopCounter     = 0; 
unsigned long startTime       = 0; 
unsigned long lastTime        = 0;
String filename;
File file;

//===================================================================
// WAV Browsing + Final Mixer
//===================================================================
const int MAX_FILES = 64; 
String fileList[MAX_FILES];
int totalFiles = 0;
int browseIndex=0;
bool inBrowseMode=false;

bool previewActive = false; // Whether a preview is currently playing 
unsigned long previewStart = 0; // Timestamp when the preview began 


String fsrSamplePath[4] = {"","","",""};  // e.g. "/Samples/snare.wav" // for void sampleOnSend(){}; 
String fsrFlashRaw[4]   = {"","","",""};  // e.g. "FSR_0.raw" // for bool dosdtoflashtransfer #define

// =============================================================
// Looper / Recording / MIDI State
// =============================================================
bool LEDdummyState[MAX_TRACKS] = {LOW, LOW, LOW, LOW}; 
bool RECState; 
bool startState; 
bool undoState; 
bool pushRotaryState; 
bool optA_state; 
bool clockTrigState; 
bool trackState[MAX_TRACKS]; 
bool muteTrack[2 * MAX_TRACKS]; 
unsigned long TblinkTime[MAX_NOTES]; 
bool waitingFirstNote[MAX_SEQUENCES]; 
bool waitingFirstSeq; 
bool RECORDING; 
bool PLAY; 
unsigned long TIME; 
unsigned long beatTIME; 
unsigned long pressRECTime; 
unsigned long startLoopTime; 
unsigned long endLoopTime; 
unsigned long loopLenght; 
unsigned long beatLenght; 
unsigned long prevBeatLenght; 
unsigned long trigTime; 
unsigned long clockLenght; 
unsigned long clockTime; 
unsigned long firstClockTick; 
unsigned long firstTrigTick; 
byte clockTick; 
byte trigCount; 
byte incomingClock; 
bool metronomeActive = false; 
bool MIDI_ECHO = true; 
const int debounceTime = 100; 
const int rotaryDebounceTime = 20; 
int timeFrame; 
int currentTrack; 
int currentSequence;

//============================================================
// Additional Sequencer / FSR / Preview Variables
//============================================================
byte MIDIdata1[MAX_NOTES][MAX_SEQUENCES] = {0};
byte MIDIdata2[MAX_NOTES][MAX_SEQUENCES] = {0};
byte MIDIchannel[MAX_NOTES][MAX_SEQUENCES] = {0};
byte noteTrack[MAX_NOTES][MAX_SEQUENCES] = {0};
unsigned long noteTime[MAX_NOTES][MAX_SEQUENCES] = {0};
int noteSeqCounter[MAX_SEQUENCES] = {0};
int noteNumber[MAX_SEQUENCES] = {0};

// Pointers to ResponsiveAnalogRead objects for each FSR:
ResponsiveAnalogRead* ANALOG_PINS[FSRs] = {nullptr, nullptr, nullptr, nullptr};

// Additional states
bool isPreviewing = false;         // To track if a preview is in progress
bool currentlyTransferring = false; // To prevent concurrent actions

// Debugging
bool flashPlayerStates[4] = {false, false, false}; 




