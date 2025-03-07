// MIDIHandler.cpp

#include "Globals.h"
#include "MIDIHandler.h"
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
#include <SerialFlash.h> 
#include <ResponsiveAnalogRead.h> 
#include <RTClib.h> 
#include "MIDIFileWriter.h"
extern MIDIFileWriter midiFileWriter;




// Then in .cpp
void MIDIHandler::initMIDI() {
    usbMIDI.setHandleNoteOn(myNoteOn);
    usbMIDI.setHandleNoteOff(myNoteOff);
    usbMIDI.setHandleControlChange(myControlChange);
    usbMIDI.setHandleAfterTouchPoly(myAfterTouchPoly);
    usbMIDI.setHandleStart(Handle_Start);
    usbMIDI.begin();

    #ifdef DISABLE_THRU
    // usbMIDI.turnThruOff();
    #endif

    RESETMIDI(); 


}

void MIDIHandler::updateMIDI() {
   usbMIDI.read();

   
   // plus any other real-time logic
}




void myNoteOn(byte channel, byte note, byte velocity){
  int delta = midiFileWriter.getDelta(); //NEW
  midiFileWriter.writeToFile(NOTE_ON_EVENT, note, velocity, delta);
  if(MIDI_ECHO){
    usbMIDI.sendNoteOn(note, velocity, 1);
  }
  if(RECORDING && !muteTrack[currentTrack]){
    if(waitingFirstNote[0]){
      waitingFirstNote[0]=false;
      startLoopTime=micros();
    } else if(waitingFirstNote[currentSequence]){
      waitingFirstNote[currentSequence]=false;
    }
    if(noteNumber[currentSequence]<MAX_NOTES && currentSequence<MAX_SEQUENCES){
      MIDIdata1[noteNumber[currentSequence]][currentSequence]= note;
      MIDIdata2[noteNumber[currentSequence]][currentSequence]= velocity;
      MIDIchannel[noteNumber[currentSequence]][currentSequence]= channel;
      noteTime[noteNumber[currentSequence]][currentSequence]=micros()-startLoopTime;
      noteTrack[noteNumber[currentSequence]][currentSequence]=currentTrack;
      noteNumber[currentSequence]++;
    }
  }
}

void myNoteOff(byte channel, byte note, byte velocity){
  int delta = midiFileWriter.getDelta(); //NEW
  midiFileWriter.writeToFile(NOTE_OFF_EVENT, note, velocity, delta);
  if(MIDI_ECHO){
    usbMIDI.sendNoteOff(note,0,1);
  }
  if(RECORDING && !waitingFirstNote[0] && !muteTrack[currentTrack]){
    if(waitingFirstNote[currentSequence]){
      waitingFirstNote[currentSequence]=false;
    }
    if(noteNumber[currentSequence]<MAX_NOTES && currentSequence<MAX_SEQUENCES){
      MIDIdata1[noteNumber[currentSequence]][currentSequence]= note;
      MIDIdata2[noteNumber[currentSequence]][currentSequence]= 0;
      MIDIchannel[noteNumber[currentSequence]][currentSequence]= channel;
      noteTime[noteNumber[currentSequence]][currentSequence]= micros()-startLoopTime;
      noteTrack[noteNumber[currentSequence]][currentSequence]= currentTrack;
      noteNumber[currentSequence]++;
    }
  }
}

void myControlChange(byte channel, byte control, byte value){
  int delta = midiFileWriter.getDelta(); //NEW
  midiFileWriter.writeToFile(CONTROL_CHANGE_EVENT, control, value, delta);
  if(MIDI_ECHO){
    usbMIDI.sendControlChange(control, value, channel);
  }
  if(RECORDING && !waitingFirstNote[0] && !muteTrack[currentTrack]){
    if(waitingFirstNote[currentSequence]){
      waitingFirstNote[currentSequence]=false;
    }
    if(noteNumber[currentSequence]<MAX_NOTES && currentSequence<MAX_SEQUENCES){
      MIDIdata1[noteNumber[currentSequence]][currentSequence]= control;
      MIDIdata2[noteNumber[currentSequence]][currentSequence]= value;
      MIDIchannel[noteNumber[currentSequence]][currentSequence]= channel+16;
      noteTime[noteNumber[currentSequence]][currentSequence]= micros()-startLoopTime;
      noteTrack[noteNumber[currentSequence]][currentSequence]= currentTrack;
      noteNumber[currentSequence]++;
    }
  }
}

void myAfterTouchPoly(byte channel, byte note, byte pressure){
  int delta = midiFileWriter.getDelta(); //NEW
  midiFileWriter.writeToFile(AFTER_TOUCH_POLY_EVENT, note, pressure, delta);
}

void Handle_Start(){
  PLAY=true;
  digitalWrite(PLAYledPin,HIGH);
  for(int i=0; i<MAX_SEQUENCES;i++){
    noteSeqCounter[i]=0;
  }
}

void MIDIHandler::Handle_Stop(){
  PLAY=false;
  digitalWrite(PLAYledPin,LOW);
  FULL_PANIC();
}



//===================================================================
// RESETMIDI
//===================================================================
void MIDIHandler::RESETMIDI() {
  RECORDING=false;
  trigTime=millis();

  for(int i=0;i<MAX_NOTES;i++){
    for(int j=0;j<MAX_SEQUENCES;j++){
      noteTime[i][j]=4294967295;
    }
  }
  for(int k=0;k<MAX_SEQUENCES;k++){
    noteSeqCounter[k]=0;
    waitingFirstNote[k]=true;
    noteNumber[k]=0;
  }
  waitingFirstSeq=true;
  PLAY=true;
  FULL_PANIC();
  FULL_LED_Blink(5);
  digitalWrite(PLAYledPin,HIGH);
  digitalWrite(trackLEDPin[0],HIGH);
  currentTrack=0;
  currentSequence=0;
  loopLenght=4294967295;
  beatLenght=500000;
  prevBeatLenght=beatLenght;
  timeFrame=16;
  RECState=digitalRead(RECPin);
  undoState=digitalRead(undoPin);
  optA_state=digitalRead(optAPin);
  startState=digitalRead(startPin);
  clockTrigState=digitalRead(clockTrigPin);

  for(int l=0;l<MAX_TRACKS;l++){
    muteTrack[l]=false;
  }
  incomingClock=0;
  trigCount=0;
}

//===================================================================
// Sequence_PANIC, Track_PANIC, Slim_PANIC, FULL_PANIC
//===================================================================
void MIDIHandler::Sequence_PANIC(){
  for(int k=0;k<MAX_NOTES;k++){
    if(noteTrack[k][currentSequence]==currentTrack){
      usbMIDI.sendNoteOn(MIDIdata1[k][currentSequence],0,
                         MIDIchannel[k][currentSequence]);
    }
  }
}

void MIDIHandler::Track_PANIC(){
  for(int k=0;k<MAX_NOTES;k++){
    for(int j=0;j<MAX_SEQUENCES;j++){
      if(noteTrack[k][j]==currentTrack){
        usbMIDI.sendNoteOn(MIDIdata1[k][j],0,MIDIchannel[k][j]);
      }
    }
  }
}

void MIDIHandler::Slim_PANIC(){
  for(int k=0;k<MAX_NOTES;k++){
    for(int j=0;j<MAX_SEQUENCES;j++){
      usbMIDI.sendNoteOn(MIDIdata1[k][j],0,MIDIchannel[k][j]);
    }
  }
}

void MIDIHandler::FULL_PANIC(){
  for(int i=0;i<127;i++){
    for(int j=0;j<16;j++){
      usbMIDI.sendNoteOn(i,0,j);
    }
  }
}

void MIDIHandler::MIDI_PANIC(){
  for(int i=1;i<=16;i++){
    usbMIDI.sendControlChange(123,0,i);
  }
}


//===================================================================
// FULL_LED_Blink
//===================================================================
void MIDIHandler::FULL_LED_Blink(byte count){
  for(int i=0;i<count;i++){
    digitalWrite(RECLEDPin,HIGH);
    digitalWrite(PLAYledPin,HIGH);
    for(int j=0;j<MAX_TRACKS;j++){
      digitalWrite(trackLEDPin[j],HIGH);      // why does triggering RECpin button while fsr flash sample assigned cause led pin 15 to start blinking, and make the whole system subsequently freeze?
    }
    delay(200);
    digitalWrite(RECLEDPin,LOW);
    digitalWrite(PLAYledPin,LOW);
    for(int k=0;k<MAX_TRACKS;k++){
      digitalWrite(trackLEDPin[k],LOW);
    }
    delay(200);
  }
}