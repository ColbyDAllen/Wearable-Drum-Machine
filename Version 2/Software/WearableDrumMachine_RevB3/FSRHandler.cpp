// FSRHandler.cpp

#include "Globals.h"  // So we can see FSRs, Note[], THRESH, etc.
#include "FSRHandler.h"
#include "MIDIHandler.h" // or whichever file has myNoteOn, myNoteOff, etc.
// #include "SampleHandler.h" if you need to call SampleOnSend(...)
#include "AudioSystem.h" // so we can see audioSys if it's extern
extern AudioSystem audioSys; // let the compiler know there's a global instance

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

void FSRHandler::initFSRs() {
    // Set up each FSR with ResponsiveAnalogRead
    for(int i = 0; i < FSRs; i++){
        ANALOG_PINS[i] = new ResponsiveAnalogRead(FSRpin[i], true, 0.8);
    }
}

// FSR scanning
void FSRHandler::updateFSRs() {
  for (int i=0; i<FSRs; i++){
    int FSRRead = analogRead(FSRpin[i]);
    if(FSRRead > THRESH){
      counter[i]++;
      if(!(VELMASK & (1<<i)) && (counter[i]==VELTIME)){
        VELMASK |= (1<<i);
        counter[i]=0;
        NoteOnSend(i);
        SampleOnSend(i);
      }
      if(counter[i]==AFTERTIME){
        counter[i]=0;
        PolyTouchSend(i);
      }
    } else {
      if(VELMASK & (1<<i)){
        myNoteOff(1, Note[i], 0);
        VELMASK &= ~(1<<i);
        counter[i]=0;
      }
    }


      // FLASH FSR TRIGGER DEBUG
      for (int k = 0; k < 4; k++) {
          if (audioSys.flashPlayer[k].isPlaying()) {
              if (!flashPlayerStates[k]) {
                  flashPlayerStates[k] = true; // Transition to playing
                  Serial.printf("FSR %d playback started\n", k);
              }
          } else {
              if (flashPlayerStates[k]) {
                  flashPlayerStates[k] = false; // Transition to not playing
                  Serial.printf("FSR %d playback finished\n", k);
              }
          }
      }
    }
}


void FSRHandler::NoteOnSend(int j) {
  int FSRRead=analogRead(FSRpin[j]);
  ANALOG_PINS[j]->update(FSRRead);
  ANALOG_PINS[j]->hasChanged();
  FSRRead=ANALOG_PINS[j]->getValue();
  FSRRead=constrain(FSRRead,0,800);
  int velocity= map(FSRRead,0,800,MIDIMIN,127);
  myNoteOn(1,Note[j],velocity);
}


void FSRHandler::PolyTouchSend(int j) {
  int FSRRead=analogRead(FSRpin[j]);
  ANALOG_PINS[j]->update(FSRRead);
  ANALOG_PINS[j]->hasChanged();
  FSRRead=ANALOG_PINS[j]->getValue();
  FSRRead=constrain(FSRRead,0,800);
  if(FSRRead>AFTERTHRESH){
    int pressure= map(FSRRead,0,800,MIDIMIN,127);
    usbMIDI.sendPolyPressure(Note[j], pressure, 1);
    myAfterTouchPoly(1, Note[j], pressure);
  }
}

// void FSRHandler::NoteOffSend(int fsrIndex)
// {
//   //...
// }



