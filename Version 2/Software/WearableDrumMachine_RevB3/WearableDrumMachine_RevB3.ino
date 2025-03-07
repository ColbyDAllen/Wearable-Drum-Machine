// main.cpp or main.ino

#include "Globals.h"
#include "AudioSystem.h"
#include "MIDIHandler.h"
#include "FSRHandler.h"
#include "MIDIFileWriter.h"
extern MIDIFileWriter midiFileWriter;
#include "SequencerHandler.h"
#include "SdBrowse.h"


#include "AudioSampleKickkhronos.h" 
#include "AudioSampleHh1khronos.h" 
#include "AudioSampleSnarekhronos.h" 
#include "AudioSampleHh2khronos.h" 
#include <Bounce.h> 
#include <Audio.h> 
#include <Wire.h> 
#include <SPI.h> 
#include <SD.h> // switched to <SdFat.h> on 1.20.25 
//#include <SdFat.h> 
#include <MIDI.h> 
//#include <usb_midi.h> 
#include <SerialFlash.h> 
#include <ResponsiveAnalogRead.h> 
#include <RTClib.h> // if you want RTC usage 

#include "LoopedNoteFunctions.h"



AudioSystem audioSys; // your global or static instance
MIDIHandler midiHandler; // your global or static instance
FSRHandler fsrHandler;
SequencerHandler sequencerHandler;
MIDIFileWriter midiFileWriter;
SDBrowse sdBrowse;
// ... or pass references around as needed

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("Booting FULL Wearable Drum + SerialFlash approach...");

  // Then call your AudioSystem init:
  audioSys.initAudioSystem();

  // Pins 
  pinMode(RECPin, INPUT_PULLUP); 
  pinMode(startPin, INPUT_PULLUP); 
  pinMode(pushRotaryPin, INPUT_PULLUP); 
  pinMode(optAPin, INPUT_PULLUP); 
  pinMode(optBPin, INPUT_PULLUP); 
  pinMode(sampleBrowsePin, INPUT_PULLUP); 
  pinMode(undoPin, INPUT_PULLUP); 
  pinMode(clockTrigPin, INPUT_PULLUP); 
  for (int i=0; i<MAX_TRACKS; i++) { 
    pinMode(trackPin[i], INPUT_PULLUP); 
  } 
  pinMode(RECLEDPin, OUTPUT); 
  pinMode(PLAYledPin, OUTPUT); 
  for(int j=0; j<MAX_TRACKS; j++) { 
    pinMode(trackLEDPin[j], OUTPUT); // TODO disable all unsused pins to save power 
  } 
  pinMode(PLACE_MARKER_PIN, INPUT); 

  // // MIDI Setup
  // usbMIDI.setHandleNoteOn(myNoteOn); 
  // usbMIDI.setHandleNoteOff(myNoteOff); 
  // usbMIDI.setHandleControlChange(myControlChange); 
  // usbMIDI.setHandleAfterTouchPoly(myAfterTouchPoly); 
  // usbMIDI.setHandleStart(Handle_Start); 
  // usbMIDI.begin(); 

  midiHandler.initMIDI();

    // If RTC: 
    /* 
      if(RTC.begin()) { 
        SdFile::dateTimeCallback(dateTime); 
        HAS_RTC = true;
    } 
    */ 

  // init SD 
  if(!SD.begin(BUILTIN_SDCARD)) { 
    Serial.println("SD.begin(BUILTIN_SDCARD) fail!"); 
  } else { 
    Serial.println("SD OK. "); 
  } 

  // init SerialFlash on pin 6 
  if(!SerialFlash.begin(6)) { 
    Serial.println("SerialFlash.begin(6) FAIL or chip not detected!"); 
  } else { 
    Serial.println("SerialFlash is ready on pin 6. (Ensure EraseEverything done.)"); 
  } 

  midiFileWriter.creatNextFile(); 
  if(file) { 
    midiFileWriter.writeMidiPreamble(); 
  } 

  fsrHandler.initFSRs(); // sets up each FSR with ResponsiveAnalogRead

  
  // ... plus any other top-level initialization (e.g. pinModes, scanning SD, etc.).
}

void loop() {

  midiHandler.updateMIDI(); // read midi
  // usbMIDI.read(); // handle real-time MIDI

  #ifdef SEND_INT_CLOCK
    SendClock();
    //sequencer.SendClock(); 
  #endif

  // Update Sequencer logic
  sequencerHandler.update(); // this calls if(PLAY){SequenceNotes();...}

  // Sample browsing
  sdBrowse.sampleBrowse();
  clockTrigSwitch();

  // FSR scanning
  fsrHandler.updateFSRs(); // scans them repeatedly

  midiFileWriter.checkForMarker();
  midiFileWriter.updateFile();

  sdBrowse.updatePreview();





}






//===================================================================
// SendClock, myNoteOnLooped, myNoteOffLooped
//===================================================================
void SendClock(){
  if(incomingClock!=1){
    if(micros()-clockTime>=clockLenght){
      clockTime=micros();
      usbMIDI.sendRealTime(usbMIDI.Clock);
    }
  }
}

void myNoteOnLooped(byte channel, byte note, byte velocity){
  int delta = midiFileWriter.getDelta(); //NEW
  midiFileWriter.writeToFile(NOTE_ON_EVENT, note, velocity, delta);
}

void myNoteOffLooped(byte channel, byte note, byte velocity){
  int delta = midiFileWriter.getDelta(); //NEW
  midiFileWriter.writeToFile(NOTE_OFF_EVENT, note, velocity, delta);
}




void Handle_Clock(){
  if(incomingClock<2){
    if(incomingClock==1){
      usbMIDI.sendRealTime(MIDI_NAMESPACE::Clock);
    }
    clockTick++;
    if(clockTick==1){
      firstClockTick=micros();
    } else if(clockTick>24){
      clockTick=0;
      incomingClock=1;
      beatLenght=micros()-firstClockTick;
      if(beatLenght!=prevBeatLenght && !waitingFirstSeq){
        //Compute_Note_Times();
        prevBeatLenght=beatLenght;
      }
    }
  }
}

void Compute_Note_Times(){
  double multiplier=(double)beatLenght/(double)prevBeatLenght;
  loopLenght= (unsigned long)(loopLenght*multiplier);
  for(int i=0;i<MAX_NOTES;i++){
    for(int j=0;j<MAX_SEQUENCES;j++){
      if(noteTime[i][j]<4294967295){
        noteTime[i][j]= (unsigned long)(noteTime[i][j]*multiplier);
      }
    }
  }
}

void clockTrigSwitch(){
  if(digitalRead(clockTrigPin)!=clockTrigState){
    clockTrigState=!clockTrigState;
    if(clockTrigState==HIGH){
      trigCount++;
      if(trigCount==1){
        firstTrigTick=micros();
      } else if(trigCount>1){
        trigCount=0;
        incomingClock=2;
        beatLenght=micros()-firstTrigTick;
        if(beatLenght!=prevBeatLenght && !waitingFirstSeq){
          //Compute_Note_Times();
          prevBeatLenght=beatLenght;
        }
      }
    }
  }
}

//===================================================================
// dateTime for optional RTC
//===================================================================
void dateTime(uint16_t* date, uint16_t* time){
  DateTime d = RTC.now();
  *date = FAT_DATE(d.year(), d.month(), d.day());
  *time = FAT_TIME(d.hour(), d.minute(), d.second());
}

