// File and MIDI handling
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

// FSR To MIDI Note Handling
int FSRpin[] = {A17, A16, A15, A14};   
const int FSRs = 4;  
int Note [] = {60, 61, 62, 63};  
int counter [FSRs];         
int VELMASK = 0;
int ATMASK = 0;
int AFTERTHRESH = 50; 
int THRESH = 45; 
int VELTIME = 500;   
int AFTERTIME = 2500;  
int MIDIMIN = 20; 
void NoteOnSend (int);
void PolyTouchSend (int);

// FSR To Audio Sample Handling 
int channel [] = {0, 1, 2, 3};    
const int SAMPLEs = 4;
int counter2 [SAMPLEs]; 
void SampleOnSend (int);                 
void gain (unsigned int channel, float gain);      

// Sequencer Array 
int Mdbchannel [] = {0, 1, 2, 3};
// ====================================================================================== 

// Real Time Clock
#include <RTClib.h>
RTC_DS3231 RTC;
bool HAS_RTC = false;

// Audio pins and values
#define AUDIO 8
#define AUDIO_DEBUG_PIN 6
int lastPlayState = 0;
bool play = false;

// Marker pins and values
#define PLACE_MARKER_PIN 9
int lastMarkState = 0;
int nextMarker = 1;

const int chipSelect = BUILTIN_SDCARD; 
#define CHIP_SELECT BUILTIN_SDCARD  
#define HAS_MORE_BYTES 0x80 

#define NOTE_OFF_EVENT 0x80 
#define NOTE_ON_EVENT 0x90
#define CONTROL_CHANGE_EVENT 0xB0
#define PITCH_BEND_EVENT 0xE0
#define AFTER_TOUCH_POLY_EVENT 0xA0

// we use a 2 minute idling timeout (in millis)
#define RECORDING_TIMEOUT 120000
unsigned long lastLoopCounter = 0;
unsigned long loopCounter = 0;

unsigned long startTime = 0; 
unsigned long lastTime = 0;

#define FILE_FLUSH_INTERVAL 400
String filename;
File file;

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
// ======================================================================================

// FSR Sensor Response Curve Modification
ResponsiveAnalogRead *ANALOG_PINS[FSRs];
// ======================================================================================

// Sequencing Structure Setup
#define MAX_TRACKS 4  
#define MAX_NOTES 500    
#define MAX_SEQUENCES 4  
#define DISABLE_THRU    
#define METRONOME_NOTE 64
#define METRONOME_CHANNEL 10  
//#define SEND_INT_CLOCK  //uncomment or delete this line if you dont want internal clock to be sent out
//#define EXT_CLOCK       //uncomment or delete this line if you dont want clock to be received
// ======================================================================================

// Sequencer Pin Assignment
const byte RECPin = 32;   
const byte startPin = 31; 
const byte clockTrigPin = 27;  
const byte pushRotaryPin = 16; 
/*const byte ECHOPin = 27;*/
const byte ECHOPin = 24; 
const byte undoPin = 25;

// Track Selection Pin Assignment
const byte trackPin[MAX_TRACKS] = { 28, 29, 3, 2 };  

// Variable Tempo Pin Assignment
const byte optAPin = 34;
const byte optBPin = 33;

// Record & Playback LED Pin Assignment
const byte RECLEDPin = 36;
const byte PLAYledPin = 37;

// Track Selection LED Pin Assignment
const byte trackLEDPin[MAX_TRACKS] = { 15, 17, 5, 4 };
bool LEDdummyState[MAX_TRACKS] = { LOW, LOW, LOW, LOW }; 

// Various Feature Handling Setup
bool RECState;
bool startState;
bool ECHOState;
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
bool metronomeActive = true;   
bool MIDI_ECHO = 1;         
const int debounceTime = 100;
const int rotaryDebounceTime = 20;   
int timeFrame;
int currentTrack;
int currentSequence;
// ======================================================================================

/*
  Bytes To Contain Looping MIDI Events
*/
byte MIDIdata1[MAX_NOTES][MAX_SEQUENCES];   // holds [MAX_NOTES][MAX_SEQUENCES] [macros], byte, 8 bits
byte MIDIdata2[MAX_NOTES][MAX_SEQUENCES]; // holds [MAX_NOTES][MAX_SEQUENCES] macros, byte, 8 bits
byte MIDIchannel[MAX_NOTES][MAX_SEQUENCES];  // holds [MAX_NOTES][MAX_SEQUENCES] macros, byte, 8 bits
byte noteTrack[MAX_NOTES][MAX_SEQUENCES];   // holds [MAX_NOTES][MAX_SEQUENCES] macros, byte, 8 bits
unsigned long noteTime[MAX_NOTES][MAX_SEQUENCES];   // holds [MAX_NOTES][MAX_SEQUENCES] macros, unsigned long, 32 bits
int noteSeqCounter[MAX_SEQUENCES];    // holds [MAX_SEQUENCES], int, 16 bits
int noteNumber[MAX_SEQUENCES];    // holds [MAX_SEQUENCES], int, 16 bits
// ======================================================================================

void setup() {
  /**
    Initialize Rev D2 Audio Shield, 
    Wav2Sketch for audio sample feedback.
  */
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  mixer1.gain(0, 0.4);
  mixer1.gain(1, 0.4);
  mixer1.gain(2, 0.4);
  mixer1.gain(3, 0.4);

  pinMode(RECPin, INPUT_PULLUP);
  pinMode(startPin, INPUT_PULLUP);
  pinMode(pushRotaryPin, INPUT_PULLUP);
  pinMode(optAPin, INPUT_PULLUP);
  pinMode(optBPin, INPUT_PULLUP);
  pinMode(ECHOPin, INPUT_PULLUP);
  pinMode(undoPin, INPUT_PULLUP);
  pinMode(clockTrigPin, INPUT_PULLUP);
  for (int i = 0; i < MAX_TRACKS; i++) { 
    pinMode(trackPin[i], INPUT_PULLUP);
  }
  pinMode(RECLEDPin, OUTPUT);
  pinMode(PLAYledPin, OUTPUT);
  for (int j = 0; j < MAX_TRACKS; j++) { 
    pinMode(trackLEDPin[j], OUTPUT); 
  }
  
  /**
    Working callback functions to intializes MiDi:
  */
  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandleControlChange(myControlChange);
  usbMIDI.setHandleAfterTouchPoly(myAfterTouchPoly);
  usbMIDI.setHandleStart(Handle_Start);

#ifdef EXT_CLOCK
  usbMIDI.setHandleClock(Handle_Clock);            
#endif
  usbMIDI.begin();
#ifdef DISABLE_THRU
#endif
  RESETMIDI();

  /**
    Set up the MIDI marker button
  */
  pinMode(PLACE_MARKER_PIN, INPUT);

  /**
    Set up RTC interfacing:
      [ ] File timestamping to be included in future update.
  */
    /*
    if (RTC.begin()) {
      // uncomment this next line to set the current date/time on the RTC
      RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));

      // if the RTC works, we can tell the SD library
      // how it can check for the current time when it
      // needs timestamping for file creation/writing.
      SdFile::dateTimeCallback(dateTime);
      HAS_RTC = true;
      //tone(AUDIO, 880, 100);
    }
    */
  
  /**
    Set up SD card functionality and allocate a file:
  */
  pinMode(CHIP_SELECT, OUTPUT);
  if (SD.begin(CHIP_SELECT)) {
    creatNextFile();
    if (file) {
      writeMidiPreamble();
    }
  }

    /**
      Set Up "ResponsiveAnalogRead" Library Settings:
    */
    for (int i = 0; i < FSRs ; i++) {
    ANALOG_PINS[i] = new ResponsiveAnalogRead(FSRpin[i], true, 0.8);  
  }
}
// ======================================================================================

/**
  Callback functions associated with " SdFile::dateTimeCallback(dateTime); "
*/
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime d = RTC.now();
  *date = FAT_DATE(d.year(), d.month(), d.day());          
  *time = FAT_TIME(d.hour(), d.minute(), d.second());
}
// ======================================================================================

/**
    We could use the EEPROM to store this number,
    but since we're not going to get timestamped
    files anyway, just looping is also fine.
*/
void creatNextFile() {
  for (int i = 1; i < 1000; i++) {
    filename = "file-";
    if (i < 10) filename += "0";
    if (i < 100) filename += "0";
    filename += String(i);
    filename += String(".mid");

    if (!SD.exists(filename.c_str())) {
      file = SD.open(filename.c_str(), FILE_WRITE);
      return;
    }
  }
}
// ======================================================================================

/**
  SMF Format Boilerplate Preamble
*/
void writeMidiPreamble() {        
  byte header[] = {           // <Header Chunk> = <chunk type><length><format><ntrks><division>   

    0x4D, 0x54, 0x68, 0x64,   // "MThd" chunk <TYPE>          
//                                    (0x4D = 77 DEC = 0100 1101 BIN = 77 TWO'sCOMPLEMENT)
//                                    (0x54 = 84 DEC = 0101 0100 BIN = 84 TWO'sCOMPLEMENT)
//                                    (0x68 = 104 DEC = 0110 1000 BIN = 104 TWO'sCOMPLEMENT)
//                                    (0x64 = 100 DEC = 0110 0100 BIN = 100 TWO'sCOMPLEMENT)
                                    
    0x00, 0x00, 0x00, 0x06,   // chunk length (from this point on) <LENGTH>
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
//                                    (0x06 = 6 DEC = 0000 0110 BIN = 6 TWO'sCOMPLEMENT)
                                    
    0x00, 0x00,               // format 0 <FORMAT>
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
                                    
    0x00, 0x01,               // one track <NTRK>
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
//                                    (0x01 = 1 DEC = 0000 0001 BIN = 1 TWO'sCOMPLEMENT)
                                    
    0x01, 0xD4                // data rate = 458 ticks-per-quarter-note <DIVISION>
//                                    (0x01 = 1 DEC = 0000 0001 BIN = 1 TWO'sCOMPLEMENT)      // <-- Actual value used
//                                    (0xD4 = 212 DEC = 1101 0100 BIN = -44 TWO'sCOMPLEMENT)                      
  };
  file.write(header, 14);

  // For Format 1, add chunk? 
  byte track[] = {            // <Track Chunk> = <TYPE><LENGTHh><MTrk EVENT>+ // <-- "+" means at least one "MTrk" event must be present. 
                                    // <MTrk EVENT> is further broken down into "<MTrk event> = <DELTA-TIME><EVENT>"
    0x4D, 0x54, 0x72, 0x6B,   // "MTrk" chunk <TYPE>
//                                    (0x4D = 77 DEC = 0100 1101 BIN = 77 TWO'sCOMPLEMENT)
//                                    (0x54 = 84 DEC = 0101 0100 BIN = 84 TWO'sCOMPLEMENT)
//                                    (0x72 = 114 DEC = 0111 0010 BIN = 114 TWO'sCOMPLEMENT)
//                                    (0x6B = 107 DEC = 0110 1011 BIN = 107 TWO'sCOMPLEMENT)
                                    
    0x00, 0x00, 0x00, 0x00    // chunk length <LENGTH> placeholder (MSB) // <-- length to be updated later //
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)

                              // <MTrk EVENT>  ?
  };
  file.write(track, 8);

  byte tempo[] = {            // ...<Tempo Chunk> = <MTrk EVENT> ?  // <-- Just a guess. Still need to determine how this fits into .mid/SMF file.
                                  // ...may have to do with " void writeMidiMarker() {if (!file) return; ...} " function?
    0x00,                     // time delta (of zero)
//                                    (0x00 = 0 DEC = 0000 0000 BIN = 0 TWO'sCOMPLEMENT)    

    0xFF, 0x51, 0x03,         // tempo op code
//                                    (0xFF = 255 DEC = 1111 1111 BIN = -1 TWO'sCOMPLEMENT)
//                                    (0x51 = 81 DEC = 0101 0001 BIN = 81 TWO'sCOMPLEMENT)
//                                    (0x03 = 3 DEC = 0000 0011 BIN = 3 TWO'sCOMPLEMENT)
                                    
    0x06, 0xFD, 0x1F          // real rate = 458,015μs per quarter note (= 134.681 BPM)
//                                      (0x06 = 6 DEC = 0000 0110 BIN = 6 TWO'sCOMPLEMENT)
//                                      (0xFD = 253 DEC = 1111 1101 BIN = -3 TWO'sCOMPLEMENT)
//                                      (0x1F = 31 DEC = 0001 1111 BIN = 31 TWO'sCOMPLEMENT)
 
  };
  file.write(tempo, 7);
}
// ======================================================================================

/**
   SMF file writing protion of program loop that consists 
   of flushing our file to disk, checking our buttons to 
   see if they just got pressed, and then handling 
   MIDI input, if there is any.
*/
void loop() {
/**
  Sequencer Portion:
*/
#ifdef SEND_INT_CLOCK   
  SendClock();
#endif
  if (PLAY) 
  {
    SequenceNotes();
    REC_Switch();
    Metronome_Switch();   
    Metronome_PLAY();    
    Opt_Encoder();      
    Undo_Switch();
  }
  TrackMutedLED();
  Track_Switch(); 
  Play_Switch();  
  ECHO_Switch();  
  clockTrigSwitch();  

    /**
      FSR Input Portion:
    */
    for (int i = 0; i < FSRs; i++) {
    int FSRRead = analogRead(FSRpin[i]);
    if (FSRRead > THRESH) {
      counter[i] ++;                                         
      if (!(VELMASK & (1 << i)) && (counter[i] == VELTIME)) { 
        VELMASK |= (1 << i);         
        counter [i] = 0;          
        NoteOnSend (i);      
        SampleOnSend (i);                                    
        }
      if (counter [i] == AFTERTIME) {
        counter [i] = 0;
        PolyTouchSend(i);
      }
    }
    else {                                                    
      if (VELMASK & (1 << i)) {           
        myNoteOff(1, Note[i], 0); 
        VELMASK &= ~ (1 << i);                 
        counter [i] = 0;
      }
    }
  }

  /**
    SMF File Writing Portion:
  */
  checkForMarker();
  setPlayState();
  updateFile();
  usbMIDI.read();   
}
// ======================================================================================

/**
   We flush the file's in-memory content to disk
   every 400ms, allowing. That way if we take the
   SD card out, it's basically impossible for any
   data to have been lost.
*/
void updateFile() {
  loopCounter = millis();
  if (loopCounter - lastLoopCounter > FILE_FLUSH_INTERVAL) {
    checkReset();
    lastLoopCounter = loopCounter;
    file.flush();
  }
}
// ======================================================================================

/**
   This "function" would normally crash any kernel that tries
   to run it by violating memory access. Instead, the Arduino's
   watchdog will auto-reboot, giving us a software "reset".
*/
void(* resetArduino) (void) = 0;
// ======================================================================================

/**
  if we've not received any data for 2 minutes, and we were
  previously recording, we reset the arduino so that when
  we start playing again, we'll be doing so in a new file,
  rather than having multiple sessions with huge silence
  between them in the same file.
*/
void checkReset() {
  if (startTime == 0) return; // not received any data for 2 minutes
  if (!file) return;          // we were not previously recording
  if (millis() - lastTime > RECORDING_TIMEOUT) {
    file.close();
    resetArduino();
  }
}
// ======================================================================================

/**
   A little audio-debugging: pressing the button tied to the
   audio debug pin will cause the program to play notes for
   every MIDI note-on event that comes flying by.
*/
void setPlayState() {
  int playState = digitalRead(AUDIO_DEBUG_PIN);
  if (playState != lastPlayState) {
    lastPlayState = playState;
    if (playState == 1) {
      play = !play;
    }
  }
}
// ======================================================================================

/**
   This checks whether the MIDI marker button got pressed,
   and if so, writes a MIDI marker message into the track.
*/
void checkForMarker() {
  int markState = digitalRead(PLACE_MARKER_PIN);
  if (markState  != lastMarkState) {
    lastMarkState = markState;
    if (markState == 1) {
      writeMidiMarker();
    }
  }
}
// ======================================================================================

/**
  Write a MIDI marker to file, by writing a delta, then
  the op code for "midi marker", the number of letters
  the marker label has, and then the label (using ASCII).

  For simplicity, the marker labels will just be a
  sequence number starting at "1".
*/
void writeMidiMarker() {
  if (!file) return;

  // delta + event code
  writeVarLen(file, getDelta());
  file.write(0xFF);              
  file.write(0x06);            
  
  // If we have an RTC available, we can write the clock time
  // Otherwise,  write a sequence number.
  if (HAS_RTC) {
    DateTime d = RTC.now();
    byte len = 20;
    writeVarLen(file, len);

    char marker[len]; // will hold strings like "2021/01/23, 10:53:31"
    sprintf(marker, "%04d/%02d/%02d, %02d:%02d:%02d", d.year(), d.month(), d.day(), d.hour(), d.minute(), d.second());
    file.write(marker, len);
  }
  else {
    // how many letters are we writing?
    byte len = 1;
    if (nextMarker > 9) len++;
    if (nextMarker > 99) len++;
    if (nextMarker > 999) len++;
    writeVarLen(file, len);

    // our label:
    byte marker[len];
    String(nextMarker++).getBytes(marker, len);
    file.write(marker, len);
  }
}
// ======================================================================================

/**
  Audio Sample Output Function:
*/
void SampleOnSend (int k) {                         
  int FSRRead2 = analogRead(FSRpin [k]);
  float gain = map (FSRRead2, 0, 127, .2, 1.0);   // output can actually range from ~0.0 to 4.0
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
// ======================================================================================

/**
   Get number of ticks since previous MIDI event
*/
int getDelta() {
  if (startTime == 0) {
    // if this is the first event, even if the Arduino's been
    // powered on for hours, this should be delta zero.
    startTime = millis();
    lastTime = startTime;
    return 0;
  }
  unsigned long now = millis();
  unsigned int delta = (now - lastTime);
  lastTime = now;
  return delta;
}
// ======================================================================================

/**
   Write "common" MIDI events to file, where common MIDI events
   all use the following data format:

     delta     eventType    b1     b2
     <delta> <event code> <byte> <byte>

   See the "Standard MIDI-File Format" for more information -- 
   http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html
*/
void writeToFile(byte eventType, byte b1, byte b2, int delta) {
  if (!file) return;
  writeVarLen(file, delta);
  file.write(eventType);
  file.write(b1);
  file.write(b2);
}
// ======================================================================================

/**
   Encode a unsigned 32 bit integer (4 Bytes) as variable-length byte sequence
   of, at most, 4 7-bit-with-has-more bytes. This function is supplied
   as part of the MIDI file format specification.
*/
void writeVarLen(File file, unsigned long value) {
  // Capture the first 7 bit block
  unsigned long buffer = value & 0x7f; 

  // Shift in 7 bit blocks with "has-more" bit from the
  // right for as long as `value` has more bits to encode.
  while ((value >>= 7) > 0) {
    buffer <<= 8;
    buffer |= HAS_MORE_BYTES;
    buffer |= value & 0x7f; 
  }

  // Then unshift bytes one at a time for as long as the has-more bit is high.
  while (true) {
    file.write((byte)(buffer & 0xff));      
    if (buffer & HAS_MORE_BYTES) {
      buffer >>= 8;
      } else {
      break;
    }
  }
}
// ======================================================================================

/**
  Sequencer Reset Function:
*/
  void RESETMIDI() {
    RECORDING = false;
    trigTime = millis();
    //clean ALL MIDI notes tracking
    for (int i = 0; i < MAX_NOTES; i++) { 
      for (int j = 0; j < MAX_SEQUENCES; j++) {        
        //MIDIdata1[i][j] = 0; //pitch or CC number
        //MIDIdata2[i][j] = 0; //velocity or CC value
        //MIDIchannel[i][j] = 0; //MIDI channel. Channels start from 1 because of the library
        noteTime[i][j] = 4294967295;  // largest value that can be stored in a 32-bit unsigned integer
      }
    }
    for (int k = 0; k < MAX_SEQUENCES; k++) {
      noteSeqCounter[k] = 0;
      waitingFirstNote[k] = true;
      noteNumber[k] = 0;
    }
    waitingFirstSeq = true;
    PLAY = true;
    FULL_PANIC();
    FULL_LED_Blink(5);
    digitalWrite(PLAYledPin, HIGH);
    digitalWrite(trackLEDPin[0], HIGH);
    currentTrack = 0;
    currentSequence = 0;
    loopLenght = 4294967295; 
    beatLenght = 500000; 
    prevBeatLenght = beatLenght;
    timeFrame = 16; 
    RECState = digitalRead(RECPin);
    undoState = digitalRead(undoPin);
    optA_state = digitalRead(optAPin);
    ECHOState = digitalRead(ECHOPin);   
    startState = digitalRead(startPin);
    clockTrigState = digitalRead(clockTrigPin);

    // Track Initialization
    for (int l = 0; l < MAX_TRACKS; l++) {      
      muteTrack[l] = false;  
    }
    incomingClock = 0;
    trigCount = 0;
  }
// ======================================================================================

  /**
    START - MIDI IN message:
  */
  void Handle_Start() {
    PLAY = true;
    digitalWrite(PLAYledPin, HIGH);
    for (int i = 0; i < MAX_SEQUENCES; i++) {
      noteSeqCounter[i] = 0;
    }
  }

  /**
    STOP - MIDI IN message:
  */
  void Handle_Stop() {
    PLAY = false;
    digitalWrite(PLAYledPin, LOW);
    FULL_PANIC();
  }
// ======================================================================================

  /**
    Clock handler:
  */
  void Handle_Clock() { 
    if (incomingClock < 2) {  
      if (incomingClock == 1) { 
        usbMIDI.sendRealTime(MIDI_NAMESPACE::Clock); 
        } 
      clockTick++;
      if (clockTick == 1) {
        firstClockTick = micros();
      } 
      else if (clockTick > 24) { 
        clockTick = 0;
        incomingClock = 1; 
        beatLenght = micros() - firstClockTick;
        if (beatLenght != prevBeatLenght && waitingFirstSeq == false) {
          prevBeatLenght = beatLenght;
        }
      }
    }
  }
// ======================================================================================

  /**
    compute note lenghts - NOT wORKING: notes computed from MIDI clock are too fast, 
    while computed from trig are tto slow (1/2 the awaited speed...)
    ^^^ Maybe I can address and fix this?
  */
  void Compute_Note_Times() {
    double multiplier = (double)beatLenght / (double)prevBeatLenght;
    loopLenght = loopLenght * multiplier;
    for (int i = 0; i < MAX_NOTES; i++) {
      for (int j = 0; j < MAX_SEQUENCES; j++) {
        if (noteTime[i][j] < 4294967295) {
          noteTime[i][j] = noteTime[i][j] * multiplier;
        }
      }
    }
  }
// ======================================================================================

/**
  Clock Trigger Function:
*/
  void clockTrigSwitch() {
    if (digitalRead(clockTrigPin) != clockTrigState /*&& millis()-trigTime > debounceTime*/) {
      clockTrigState = !clockTrigState;
      if (clockTrigState == HIGH) {
        trigCount++;
        if (trigCount == 1) {
          firstTrigTick = micros();
        } else if (trigCount > 1) {
          trigCount = 0;
          incomingClock = 2;  
          beatLenght = micros() - firstTrigTick;
          if (beatLenght != prevBeatLenght && waitingFirstSeq == false) {
            prevBeatLenght = beatLenght;
          }
        }
      }
    }
  }
// ======================================================================================

/**
  Intermediary function, defined globally, used 
  to pass sensor data from user's fingertips 
  to MIDI events
*/
void NoteOnSend (int j) {
  int FSRRead = analogRead(FSRpin[j]);
  ANALOG_PINS[j]->update(FSRRead);                                                                                      
  ANALOG_PINS[j]->hasChanged();
  FSRRead = ANALOG_PINS[j]->getValue(); 
  FSRRead = constrain(FSRRead, 0, 800);           
  int velocity = map (FSRRead, 0, 800, MIDIMIN, 127);
  myNoteOn(1, Note[j], velocity);
}
// ======================================================================================

/*
  Intermediary function, defined globally, used 
  to pass sensor data from user's fingertips 
  to MIDI events.
*/
void PolyTouchSend (int j) {
  int FSRRead = analogRead(FSRpin [j]);
    ANALOG_PINS[j]->update(FSRRead);
    ANALOG_PINS[j]->hasChanged();
    FSRRead = ANALOG_PINS[j]->getValue();
    // v-- constrain to account for tolerances
    FSRRead = constrain(FSRRead, 0, 1023);
  if (FSRRead > AFTERTHRESH) {
    int pressure = map (FSRRead, 0, 800, MIDIMIN, 127);  
    usbMIDI.sendPolyPressure (Note[j], pressure, 1);  
    myAfterTouchPoly(1, Note[j], pressure); 
  }
}
// ======================================================================================

  /**
    Note-on & Note-off Callback Functions: 
      When these MIDI events are received, it captures our parameters.
  */
  void myNoteOn(byte channel, byte note, byte velocity) {  
    writeToFile(NOTE_ON_EVENT, note, velocity, getDelta());
    if (MIDI_ECHO) {
      usbMIDI.sendNoteOn(note, velocity, 1); 
    }
    if (RECORDING && muteTrack[currentTrack] == false) {  
      if (waitingFirstNote[0] == true) {                  
        waitingFirstNote[0] = false;
        startLoopTime = micros();
      } 
      else if (waitingFirstNote[currentSequence] == true) {
        waitingFirstNote[currentSequence] = false;
      }
      if (noteNumber[currentSequence] < MAX_NOTES && currentSequence < MAX_SEQUENCES) {
        MIDIdata1[noteNumber[currentSequence]][currentSequence] = note; 
        MIDIdata2[noteNumber[currentSequence]][currentSequence] = velocity; 
        MIDIchannel[noteNumber[currentSequence]][currentSequence] = channel;
        noteTime[noteNumber[currentSequence]][currentSequence] = micros() - startLoopTime;
        noteTrack[noteNumber[currentSequence]][currentSequence] = currentTrack;
        noteNumber[currentSequence]++;
      }
    }
  }
  
  void myNoteOff(byte channel, byte note, byte velocity)  {  
    writeToFile(NOTE_OFF_EVENT, note, velocity, getDelta());
    if (MIDI_ECHO) {
      usbMIDI.sendNoteOff(note, 0, 1);    
    }
    if (RECORDING && waitingFirstNote[0] == false && muteTrack[currentTrack] == false) { 
      if (waitingFirstNote[currentSequence] == true) {
        waitingFirstNote[currentSequence] = false;
      }
      if (noteNumber[currentSequence] < MAX_NOTES && currentSequence < MAX_SEQUENCES) { 
        MIDIdata1[noteNumber[currentSequence]][currentSequence] = note;
        MIDIdata2[noteNumber[currentSequence]][currentSequence] = 0;
        MIDIchannel[noteNumber[currentSequence]][currentSequence] = channel;
        noteTime[noteNumber[currentSequence]][currentSequence] = micros() - startLoopTime;
        noteTrack[noteNumber[currentSequence]][currentSequence] = currentTrack;
        noteNumber[currentSequence]++;
      }
    }
  }
// ======================================================================================

  /**
    Polyphonice Aftertouch Callback Function: 
      Not yet part of looper/sequencer. Can be introduced in future sampler/synth update.
  */
  void myAfterTouchPoly(byte channel, byte note, byte pressure)  {
    writeToFile(AFTER_TOUCH_POLY_EVENT, note, pressure, getDelta());
  }
// ======================================================================================

  /**
    CC Message Callback Function:
      Not yet part of looper/sequencer. Can be introduced in future sampler/synth update.
  */
  void myControlChange(byte channel, byte control, byte value) {
    writeToFile(CONTROL_CHANGE_EVENT, control, value, getDelta());
    if (MIDI_ECHO) {
      usbMIDI.sendControlChange(control, value, channel);
    }
    if (RECORDING && waitingFirstNote[0] == false && muteTrack[currentTrack] == false) {
      if (waitingFirstNote[currentSequence] == true) {
        waitingFirstNote[currentSequence] = false;
      }
      if (noteNumber[currentSequence] < MAX_NOTES && currentSequence < MAX_SEQUENCES) {
        MIDIdata1[noteNumber[currentSequence]][currentSequence] = control;
        MIDIdata2[noteNumber[currentSequence]][currentSequence] = value;
        MIDIchannel[noteNumber[currentSequence]][currentSequence] = channel + 16; 
        noteTime[noteNumber[currentSequence]][currentSequence] = micros() - startLoopTime;
        noteTrack[noteNumber[currentSequence]][currentSequence] = currentTrack;
        noteNumber[currentSequence]++;
      }
    }
  }
// ======================================================================================

  /**
    Pitch Bend Callback Function: 
      Potentially resource-intensive function. Entirely commented out for now.   
  */
/*
  void myPitchChange(byte channel, int bend) {

      //bend += 0x2000; // MIDI bend uses the range 0x0000-0x3FFF, with 0x2000 as center. Double the size of other data byte channel messages. A 14-bit thing.
      //byte lsb = bend & 0x7F;
      //byte msb = bend >> 7;
      //writeToFile(PITCH_BEND_EVENT, lsb, msb, getDelta());

    if (MIDI_ECHO) {
      //MIDI.sendPitchBend(bend, channel);  //echo the message
      usbMIDI.sendPitchBend(bend, channel);
    }
    if (RECORDING && waitingFirstNote[0] == false && muteTrack[currentTrack] == false) {  //record PB parameters
      if (waitingFirstNote[currentSequence] == true) {
        waitingFirstNote[currentSequence] = false;
      }
      if (noteNumber[currentSequence] < MAX_NOTES && currentSequence < MAX_SEQUENCES) {
        MIDIdata1[noteNumber[currentSequence]][currentSequence] = bend >> 6;       // >>6 = /64
        MIDIchannel[noteNumber[currentSequence]][currentSequence] = channel + 32;  //+32 to distinguish notes and CCs from pitch bends

        noteTime[noteNumber[currentSequence]][currentSequence] = micros() - startLoopTime;
        noteTrack[noteNumber[currentSequence]][currentSequence] = currentTrack;
        noteNumber[currentSequence]++;
      }
    }
  }
*/
// ======================================================================================

  /** 
    Real-time MIDI Clock:
        Sends internal clock if no external clock incoming (if an internal clock is received, it's immediately echoed)
  */
  void SendClock() {
    if (incomingClock != 1) { 
      if (micros() - clockTime >= clockLenght) 
      {
        clockTime = micros();
        usbMIDI.sendRealTime(usbMIDI.Clock); 
      }
    }
  }
// ======================================================================================

/** 
  Looped MIDI To File Functions:
*/
  void myNoteOnLooped(byte channel, byte note, byte velocity)  {
    writeToFile(NOTE_ON_EVENT, note, velocity, getDelta());
  }

  void myNoteOffLooped(byte channel, byte note, byte velocity)  {
    writeToFile(NOTE_OFF_EVENT, note, velocity, getDelta());
  }
// ======================================================================================

/** 
  MIDI Looper (Note Sequencing) Function:
*/
  void SequenceNotes() {
    for (int i = 0; i < MAX_SEQUENCES; i++) {                               
      if (micros() - startLoopTime >= noteTime[noteSeqCounter[i] ] [i] ) {
        if (i != currentSequence) {  
          if (muteTrack[noteTrack[noteSeqCounter[i] ] [i] ] == false) {
            if (MIDIchannel[noteSeqCounter[i] ] [i] <= 16) {
              usbMIDI.sendNoteOn(MIDIdata1[noteSeqCounter[i] ] [i], MIDIdata2[noteSeqCounter[i] ] [i], MIDIchannel[noteSeqCounter[i] ] [i] );
                int Mdb2 = (MIDIdata2[noteSeqCounter[i] ] [i]); 
                int Mdb1 = (MIDIdata1[noteSeqCounter[i] ] [i]);
                if (Mdb2 != 0) {
                // Note 60:
                  if (Mdb1 == 60 )  {
                    float gain = map(Mdb2, 0, 127, 0.2, 4.0 );
                    mixer1.gain(0, gain);
                    playMem1.play(AudioSampleHh2khronos);
                  }
                // Note 61:
                  if (Mdb1 == 61 )  {
                    float gain = map(Mdb2, 0, 127, 0.2, 4.0 );
                    mixer1.gain(1, gain);
                    playMem2.play(AudioSampleSnarekhronos);
                  }
                // Note 62:
                  if (Mdb1 == 62) {
                    float gain = map(Mdb2, 0, 127, 0.2, 4.0 );
                    mixer1.gain(2, gain);
                    playMem3.play(AudioSampleHh1khronos);
                  }
                // Note 63:
                  if (Mdb1 == 63 ) {
                    float gain = map(Mdb2, 0, 127, 0.2, 4.0 );
                    mixer1.gain(3, gain);
                    playMem4.play(AudioSampleKickkhronos);
                  }
                  myNoteOnLooped(MIDIchannel[noteSeqCounter[i] ] [i], MIDIdata1[noteSeqCounter[i] ] [i], MIDIdata2[noteSeqCounter[i] ] [i]); 
                }
                if (Mdb2 == 0) {
                  myNoteOffLooped(MIDIchannel[noteSeqCounter[i] ] [i], MIDIdata1[noteSeqCounter[i] ] [i], MIDIdata2[noteSeqCounter[i] ] [i]);     
                }
            } 
            else if (MIDIchannel[noteSeqCounter[i]][i] <= 32) {  
              usbMIDI.sendControlChange(MIDIdata1[noteSeqCounter[i]][i], MIDIdata2[noteSeqCounter[i]][i], MIDIchannel[noteSeqCounter[i]][i] - 16);
            } 
            else { 
              usbMIDI.sendPitchBend(MIDIdata1[noteSeqCounter[i]][i] << 6, MIDIchannel[noteSeqCounter[i]][i] - 32);
            }
          }
        }
        noteSeqCounter[i]++;
      }
    }
    if (micros() - startLoopTime >= loopLenght) { 
      for (int j = 0; j < MAX_SEQUENCES; j++) {
        noteSeqCounter[j] = 0;
      }
      startLoopTime = micros();
      beatTIME = micros();
      if (waitingFirstNote[currentSequence] == false) 
      {
        currentSequence++;
        if (currentSequence >= MAX_SEQUENCES) { currentSequence = MAX_SEQUENCES; 
        }
      }
    }
  }
// ======================================================================================

  /**
    Record Switch Function:
  */
  void REC_Switch() {
    if (digitalRead(RECPin) != RECState && millis() - trigTime > debounceTime) {
      RECState = !RECState;
      trigTime = millis();
      if (RECState == LOW) {
        RECORDING = !RECORDING;
        digitalWrite(RECLEDPin, RECORDING);
        if (RECORDING) {
        }  
        else { 
          if (waitingFirstSeq) {
            loopLenght = micros() - startLoopTime;
            startLoopTime = micros();  //restarts loop cycle
            if (waitingFirstNote[0] == false) {
              waitingFirstSeq = false;
            }
            currentSequence++;
            for (int i = 0; i < MAX_SEQUENCES; i++) {
              noteSeqCounter[i] = 0;
            }
          }
        }
      }
    }
    if (RECState == HIGH) {
      pressRECTime = millis();
    } 
    else {
      if (millis() - pressRECTime > 3200) {
        pressRECTime = millis();
        if (currentTrack == 0) {
          RESETMIDI();
        } 
        else {
          for (int i = 0; i < MAX_NOTES; i++) {
            for (int j = 0; j < MAX_SEQUENCES; j++) {
              if (noteTrack[i][j] == currentTrack) {
                noteTime[i][j] = 4294967295;
              }
            }
          }
        }
      }
    }
  }
// ======================================================================================

  /**
    Echo ("Panic") Function:
  */
  void ECHO_Switch() {           
    if (digitalRead(ECHOPin) != ECHOState && millis() - trigTime > debounceTime) {
      ECHOState = !ECHOState;
      trigTime = millis();
      if (ECHOState == LOW) {
        MIDI_ECHO = !MIDI_ECHO;
      }
    }
  }
// ======================================================================================

  /**
    Undo Button Function:
  */
  void Undo_Switch() {
    if (digitalRead(undoPin) != undoState && millis() - trigTime > debounceTime) {
      undoState = !undoState;
      trigTime = millis();
      if (undoState == LOW) {
        currentSequence--;    
        if (currentSequence == 0) {
          RESETMIDI();
        } 
        else {
          Sequence_PANIC();
          for (int i = 0; i < MAX_NOTES; i++) {
            noteTime[i][currentSequence] = 4294967295;  
            waitingFirstNote[currentSequence] = true;
            noteNumber[currentSequence] = 0;
          }
        }
      }
    }
  }
// ======================================================================================

  /**
    Play Button Function:
  */
  void Play_Switch() {   
    if (digitalRead(startPin) != startState && millis() - trigTime > debounceTime) { 
      startState = !startState;
      trigTime = millis();
      if (startState == LOW) {
        PLAY = !PLAY;
        if (PLAY) {
          startLoopTime = micros();                   
          for (int i = 0; i < MAX_SEQUENCES; i++) {
            noteSeqCounter[i] = 0;
          }
        } 
        else { 
          Slim_PANIC();
        }
      }
    }
  }
// ======================================================================================

  /**
    Track Select Function:
      Select the track you want to record your MIDI sequence to. 
      By default track "one" is chosen, but you can change it by pressing 
      one of the tracks buttons (one for each track).
  */
  void Track_Switch() {
    for (int i = 0; i < MAX_TRACKS; i++) {
      if (digitalRead(trackPin[i]) != trackState[i] && millis() - trigTime > debounceTime) {  
        trackState[i] = !trackState[i];                                          
        trigTime = millis();
        if (trackState[0] == LOW && trackState[1] == LOW && trackState[2] == LOW) {  
          RESETMIDI();
        } 
        else if (trackState[0] == LOW && trackState[1] == LOW) { 
          FULL_PANIC();
        } 
        else if (trackState[i] == LOW) {
          if (currentTrack != i) {           
            currentTrack = i;              
          } 
          else {  
            muteTrack[i] = !muteTrack[i];
            Track_PANIC();
          }
          switch (i) {
            case 0:                            
              digitalWrite(trackLEDPin[0], HIGH); 
              digitalWrite(trackLEDPin[1], LOW); 
              digitalWrite(trackLEDPin[2], LOW); 
              digitalWrite(trackLEDPin[3], LOW);  
              break;
            case 1:
              digitalWrite(trackLEDPin[1], HIGH); 
              digitalWrite(trackLEDPin[0], LOW);  
              digitalWrite(trackLEDPin[2], LOW);
              digitalWrite(trackLEDPin[3], LOW);
              digitalWrite(trackLEDPin[4], LOW);  
              break;
            case 2:
              digitalWrite(trackLEDPin[2], HIGH);
              digitalWrite(trackLEDPin[0], LOW); 
              digitalWrite(trackLEDPin[1], LOW);  
              digitalWrite(trackLEDPin[3], LOW);
              digitalWrite(trackLEDPin[4], LOW);
              break;
            case 3:
              digitalWrite(trackLEDPin[3], HIGH);
              digitalWrite(trackLEDPin[0], LOW); 
              digitalWrite(trackLEDPin[1], LOW);  
              digitalWrite(trackLEDPin[2], LOW);
              digitalWrite(trackLEDPin[4], LOW);
              break;
            case 4:
              digitalWrite(trackLEDPin[4], HIGH);
              digitalWrite(trackLEDPin[0], LOW);  
              digitalWrite(trackLEDPin[1], LOW); 
              digitalWrite(trackLEDPin[2], LOW);
              digitalWrite(trackLEDPin[3], LOW);
              break;
          }
        }
      }
    }
  }
// ======================================================================================

  /**
  Track Mute LED Indicator Function:
  */
  void TrackMutedLED() {
    for (int i = 0; i < MAX_TRACKS; i++) {
      if (muteTrack[i] == true && millis() - TblinkTime[i] > 200) {
        TblinkTime[i] = millis();
        LEDdummyState[i] = !LEDdummyState[i];
        digitalWrite(trackLEDPin[i], LEDdummyState[i]); 
      }
    }
  }
// ======================================================================================

  /**
    Metronome On/Off (Encoder Push Button) Function:
  */
  void Metronome_Switch() {
    if (digitalRead(pushRotaryPin) != pushRotaryState && millis() - trigTime > debounceTime) {
      pushRotaryState = !pushRotaryState;
      trigTime = millis();
      if (pushRotaryState == LOW) {
        metronomeActive = !metronomeActive;
      }
    }
  }
// ======================================================================================

  /* 
    Metronome Output Function:
      [x] Currently sends "click track" in the form of E5 MIDI note over USB MIDI.
      [ ] To include in SMF MIDI & audio output in future update.
  */
  void Metronome_PLAY() {  
    if (metronomeActive == true) {
      if (micros() - beatTIME >= beatLenght) {
        beatTIME = micros();
        usbMIDI.sendNoteOn(METRONOME_NOTE, 70, METRONOME_CHANNEL);
      }
    }
  }
// ======================================================================================

  /**
    Metronome (Variable Tempo Control) Function:
  */
  void Opt_Encoder() {
    if (incomingClock == 0) {
      if (digitalRead(optAPin) != optA_state && millis() - trigTime > rotaryDebounceTime) {  
        optA_state = !optA_state;  
        trigTime = millis();    
        if (optA_state == HIGH) {
          if (digitalRead(optBPin) == HIGH) {
            beatLenght = beatLenght - 10000; 
          } 
          else {
            beatLenght = beatLenght + 10000;  
          }
          clockLenght = beatLenght / 24;  
        }
      }
    }
  }
// ======================================================================================

  /**
    Panic Functions:
  */
  void Sequence_PANIC() {
    for (int k = 0; k < MAX_NOTES; k++) {
      if (noteTrack[k][currentSequence] == currentTrack) {
        usbMIDI.sendNoteOn(MIDIdata1[k][currentSequence], 0, MIDIchannel[k][currentSequence]);
      }
    }
  }

  void Track_PANIC() {
    for (int k = 0; k < MAX_NOTES; k++) {
      for (int j = 0; j < MAX_SEQUENCES; j++) {
        if (noteTrack[k][j] == currentTrack) {
          usbMIDI.sendNoteOn(MIDIdata1[k][j], 0, MIDIchannel[k][j]);
        }
      }
    }
  }

  void Slim_PANIC() {
    for (int k = 0; k < MAX_NOTES; k++) {
      for (int j = 0; j < MAX_SEQUENCES; j++) {
        usbMIDI.sendNoteOn(MIDIdata1[k][j], 0, MIDIchannel[k][j]);
      }
    }
  }

  void FULL_PANIC() {
    for (int i = 0; i < 127; i++) {  
      for (int j = 0; j < 16; j++) {  
        usbMIDI.sendNoteOn(i, 0, j);       
      }
    }
  }

  void MIDI_PANIC() {
    for (int i = 1; i <= 16; i++) {
      usbMIDI.sendControlChange(123, 0, i);
    }
  }

  void FULL_LED_Blink(byte count) {
    for (int i = 0; i < count; i++) {
      digitalWrite(RECLEDPin, HIGH);
      digitalWrite(PLAYledPin, HIGH);
      for (int j = 0; j < MAX_TRACKS; j++) {
        digitalWrite(trackLEDPin[j], HIGH);
      }
      delay(200);                                
      digitalWrite(RECLEDPin, LOW);
      digitalWrite(PLAYledPin, LOW);
      for (int k = 0; k < MAX_TRACKS; k++) {
        digitalWrite(trackLEDPin[k], LOW);
      }
      delay(200);
    }
  }
// ======================================================================================

/* 
Citations and Helpful Links:
1--https://www.instructables.com/Arduino-Multi-track-MIDI-Loop-Station/
      The vast majority of sequencer/looper code in this progeram is sourced/modified from this excellent guide by Baritono Marchetto. 
2--https://forum.pjrc.com/threads/31797-Teensy-FSR-based-MIDI-controller--
      The basis for the force-sensing resistor input code is derived from Adrian's example sketch found here. 
3--https://projecthub.arduino.cc/pomax/creating-a-midi-pass-through-recorder-53d48c
      The SMF format code for writing MIDI files to a microSD card is pretty much plagiarized from this one-of-a-kind guide by Pomax.
*/