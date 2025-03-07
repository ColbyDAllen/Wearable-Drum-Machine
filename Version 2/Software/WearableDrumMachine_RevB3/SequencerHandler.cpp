// SequencerHandler.cpp

#include "SequencerHandler.h"
#include "Globals.h"      // if you need direct usage of global variables
#include "MIDIHandler.h"  // if you call e.g. myNoteOn() from here
#include "LoopedNoteFunctions.h"
#include "AudioSystem.h"
extern AudioSystem audioSys; // global
#include "MIDIHandler.h"
extern MIDIHandler midiHandler; // global
#include "LoopedNoteFunctions.h"



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



void SequencerHandler::update()
{
    if (PLAY) {
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
}

void SequencerHandler::SequenceNotes() {
    for (int i = 0; i < MAX_SEQUENCES; i++) {
        // Trigger notes at the correct time
        if (micros() - startLoopTime >= noteTime[noteSeqCounter[i]][i]) {
            if (i != currentSequence) { // Avoid REC ECHO
                if (!muteTrack[noteTrack[noteSeqCounter[i]][i]]) {
                    byte ch = MIDIchannel[noteSeqCounter[i]][i];
                    byte d1 = MIDIdata1[noteSeqCounter[i]][i]; // Note
                    byte d2 = MIDIdata2[noteSeqCounter[i]][i]; // Velocity

                    // Trigger MIDI note
                    if (ch <= 16) {
                        usbMIDI.sendNoteOn(d1, d2, ch);
                        if (file) {
                            if (d2 > 0) myNoteOnLooped(ch, d1, d2);
                            else myNoteOffLooped(ch, d1, d2);
                        }

                        // Map note to playback objects
                        if (d2 > 0) {
                            if (d1 == 60) {
                                float gain = map(d2, 0, 127, 0.2, 4.0);
                                audioSys.mixer1.gain(0, gain);
                                // Check for flash sample for FSR 0
                                if (fsrFlashRaw[0].length() > 0) {
                                  audioSys.flashPlayer[0].play(fsrFlashRaw[0].c_str());
                                          // debug flash player timing
                                          // if (!flashPlayer[0].isPlaying())  {
                                          //   Serial.printf("Playing flash sample for FSR 0: %s\n", fsrFlashRaw[0].c_str());
                                          //   flashPlayer[0].play(fsrFlashRaw[0].c_str());
                                          // }
                                } else {
                                  audioSys.playMem1.play(AudioSampleHh1khronos); // Fallback baked-in sample
                                }
                            } else if (d1 == 61) {
                                float gain = map(d2, 0, 127, 0.2, 4.0);
                                audioSys.mixer1.gain(1, gain);
                                // Check for flash sample for FSR 1
                                if (fsrFlashRaw[1].length() > 0) {
                                    audioSys.flashPlayer[1].play(fsrFlashRaw[1].c_str());
                                } else {
                                    audioSys.playMem2.play(AudioSampleSnarekhronos); // Fallback baked-in sample
                                }
                            } else if (d1 == 62) {
                                float gain = map(d2, 0, 127, 0.2, 4.0);
                                audioSys.mixer1.gain(2, gain);
                                // Check for flash sample for FSR 2
                                if (fsrFlashRaw[2].length() > 0) {
                                    audioSys.flashPlayer[2].play(fsrFlashRaw[2].c_str());
                                } else {
                                    audioSys.playMem3.play(AudioSampleHh1khronos); // Fallback baked-in sample
                                }
                            } else if (d1 == 63) {
                                float gain = map(d2, 0, 127, 0.2, 4.0);
                                audioSys.mixer1.gain(3, gain);
                                // Check for flash sample for FSR 3
                                if (fsrFlashRaw[3].length() > 0) {
                                    audioSys.flashPlayer[3].play(fsrFlashRaw[3].c_str());
                                } else {
                                    audioSys.playMem4.play(AudioSampleKickkhronos); // Fallback baked-in sample
                                }
                            }
                      }
                    // } else if (ch <= 32)  {
                    //   // Handle control changes.
                    //   usbMIDI.sendControlChange(d1, d2, ch - 16);
                    // } else {
                    //   // Handle pitch bend or other events. 
                    //   usbMIDI.sendPitchBend(d1 << 6, ch - 32);
                    // }
                } 
            } 
            noteSeqCounter[i]++;
              // debugging for SequenceNotes() to confirm proper note timing and counters:
                // Serial.print("NoteSeqCounter: ");
                // Serial.println(noteSeqCounter[i]);
                // Serial.print("NoteTime: ");
                // Serial.println(noteTime[noteSeqCounter[i]][i]);
          } // end only play if not current sequence being recorded.
        } // end "if it's time to play the note" logic.
        // End-of-loop logic: Reset sequence counter if the loop ends

    if (micros() - startLoopTime >= loopLenght) {
      for (int j = 0; j < MAX_SEQUENCES; j++) {
        noteSeqCounter[j] = 0; 
      }

      /*
          //TODO: Remove the re-read of loop length (or beat?) length to try and tighten up the midi writing to match what midiUSB picks up. 
      */
      startLoopTime = micros();
      beatTIME = micros();        // probably omit this for higher loop--> loop accuracy timing with MIDI...

      // Move to the next sequence if applicable
      if (!waitingFirstNote[currentSequence]) {
        currentSequence++;
        if (currentSequence >= MAX_SEQUENCES) {     // Still shaky on how this will play out in edge cases.
          currentSequence = 0; // Reset to the first sequence
        }
      }
    }
  }
}


//===================================================================
// REC_Switch
//===================================================================
void SequencerHandler::REC_Switch() {
  if(digitalRead(RECPin)!=RECState && millis()-trigTime>debounceTime){
    RECState=!RECState;
    trigTime=millis();
    if(RECState==LOW){
      RECORDING=!RECORDING;
      digitalWrite(RECLEDPin, RECORDING);
      if(!RECORDING){
        if(waitingFirstSeq){
          loopLenght=micros()-startLoopTime;
          startLoopTime=micros();
          if(!waitingFirstNote[0]){
            waitingFirstSeq=false;
          }
          currentSequence++;
          for(int i=0;i<MAX_SEQUENCES;i++){
            noteSeqCounter[i]=0;
          }
        }
      }
    }
  }
  if(RECState==HIGH){
    pressRECTime=millis();
  } else {
    if(millis()-pressRECTime>3200){
      pressRECTime=millis();
      if(currentTrack==0){
        midiHandler.RESETMIDI();
      } else {
        // erase all notes in currentTrack
        for(int i=0;i<MAX_NOTES;i++){
          for(int j=0;j<MAX_SEQUENCES;j++){
            if(noteTrack[i][j]==currentTrack){
              noteTime[i][j]=4294967295;
            }
          }
        }
      }
    }
  }
}


//===================================================================
// Metronome_Switch, Metronome_PLAY
//===================================================================
void SequencerHandler::Metronome_Switch() {
  if(digitalRead(pushRotaryPin)!=pushRotaryState && millis()-trigTime>debounceTime){
    pushRotaryState=!pushRotaryState;
    trigTime=millis();
    if(pushRotaryState==LOW){
      metronomeActive=!metronomeActive;
    }
  }
}

void SequencerHandler::Metronome_PLAY(){
  if(metronomeActive){
    if(micros()-beatTIME>=beatLenght){
      beatTIME=micros();
      usbMIDI.sendNoteOn(METRONOME_NOTE,70,METRONOME_CHANNEL);
    }
  }
}


//===================================================================
// Opt_Encoder
//===================================================================
void SequencerHandler::Opt_Encoder(){
  if(incomingClock==0){
    if(digitalRead(optAPin)!=optA_state && millis()-trigTime>rotaryDebounceTime){
      optA_state=!optA_state;
      trigTime=millis();
      if(optA_state==HIGH){
        if(digitalRead(optBPin)==HIGH){
          beatLenght-=10000;
        } else {
          beatLenght+=10000;
        }
        clockLenght=beatLenght/24;
      }
    }
  }
}


//===================================================================
// Undo_Switch
//===================================================================
void SequencerHandler::Undo_Switch() {
  if(digitalRead(undoPin)!=undoState && millis()-trigTime>debounceTime){
    undoState=!undoState;
    trigTime=millis();
    if(undoState==LOW){
      currentSequence--;
      if(currentSequence==0){
        midiHandler.RESETMIDI();
      } else {
        midiHandler.Sequence_PANIC();
        for(int i=0;i<MAX_NOTES;i++){
          noteTime[i][currentSequence]=4294967295;
          waitingFirstNote[currentSequence]=true;
          noteNumber[currentSequence]=0;
        }
      }
    }
  }
}


//===================================================================
// TrackMutedLED
//===================================================================
void SequencerHandler::TrackMutedLED() {
  for(int i=0;i<MAX_TRACKS;i++){
    if(muteTrack[i] && (millis()-TblinkTime[i]>200)){
      TblinkTime[i]=millis();
      LEDdummyState[i]=!LEDdummyState[i];
      digitalWrite(trackLEDPin[i], LEDdummyState[i]);
    }
  }
}


//===================================================================
// Track_Switch
//===================================================================
void SequencerHandler::Track_Switch() {
  for(int i=0;i<MAX_TRACKS;i++){
    if(digitalRead(trackPin[i])!=trackState[i] && millis()-trigTime>debounceTime){
      trackState[i]=!trackState[i];
      trigTime=millis();
      if(trackState[0]==LOW && trackState[1]==LOW && trackState[2]==LOW){
        midiHandler.RESETMIDI();
      } else if(trackState[0]==LOW && trackState[1]==LOW){
        midiHandler.FULL_PANIC();
      } else if(trackState[i]==LOW){
        if(currentTrack!=i){
          currentTrack=i;
        } else {
          muteTrack[i]=!muteTrack[i];
          midiHandler.Track_PANIC();
        }
        switch(i){
          case 0:
            digitalWrite(trackLEDPin[0],HIGH);
            digitalWrite(trackLEDPin[1],LOW);
            digitalWrite(trackLEDPin[2],LOW);
            digitalWrite(trackLEDPin[3],LOW);
            break;
          case 1:
            digitalWrite(trackLEDPin[1],HIGH);
            digitalWrite(trackLEDPin[0],LOW);
            digitalWrite(trackLEDPin[2],LOW);
            digitalWrite(trackLEDPin[3],LOW);
            break;
          case 2:
            digitalWrite(trackLEDPin[2],HIGH);
            digitalWrite(trackLEDPin[0],LOW);
            digitalWrite(trackLEDPin[1],LOW);
            digitalWrite(trackLEDPin[3],LOW);
            break;
          case 3:
            digitalWrite(trackLEDPin[3],HIGH);
            digitalWrite(trackLEDPin[0],LOW);
            digitalWrite(trackLEDPin[1],LOW);
            digitalWrite(trackLEDPin[2],LOW);
            break;
        }
      }
    }
  }
}


//===================================================================
// Play_Switch
//===================================================================
void SequencerHandler::Play_Switch() {
  if(digitalRead(startPin)!=startState && millis()-trigTime>debounceTime){
    startState=!startState;
    trigTime=millis();
    if(startState==LOW){
      PLAY=!PLAY;
      if(PLAY){
        startLoopTime=micros();
        for(int i=0;i<MAX_SEQUENCES;i++){
          noteSeqCounter[i]=0;
        }
      } else {
        midiHandler.Slim_PANIC();
      }
    }
  }
}

