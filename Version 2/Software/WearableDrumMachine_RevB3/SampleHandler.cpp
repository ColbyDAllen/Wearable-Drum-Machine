// SampleHandler.cpp

#include "Globals.h"
#include "AudioSystem.h" // if you need to access playMem1, playMem2, etc.
extern AudioSystem audioSys; // global
#include "MIDIHandler.h"

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
// SampleOnSend
//   This function is called whenever an FSR is tapped outside of browse mode
//   We check if we have an assigned flash raw => play from flash
//   else if we have an assigned SD path => play from playSdWav1
//   else fallback to baked-in memory sample
//===================================================================
void SampleOnSend(int k)  {

    // Debug logging: Identify which FSR triggered and the sample type being played
    Serial.printf("FSR %d triggered. Checking sample type...\n", k);

  int FSRRead2 = analogRead(FSRpin[k]);
  float gainVal= map(FSRRead2,0,127,0.2,1.0);
  audioSys.mixer1.gain(channel[k], gainVal);

  // If we assigned a flash raw
  if(fsrFlashRaw[k].length()>0) {

      // debugging
      Serial.printf("FSR %d playing flash sample: %s\n", k, fsrFlashRaw[k].c_str());
      //Serial.println("Mixer gain for FSR %d: %f\n", k, mixer1.gain(channel[k]));


    // use flashPlayer[k]
    if (audioSys.flashPlayer[k].isPlaying()) {
      audioSys.flashPlayer[k].stop();
      delay(10);  // Allow time for cleanup
    }

        // DEBUGGING
        flashPlayerStates[k] = true; // Mark as playing when starting a sample

    audioSys.flashPlayer[k].play(fsrFlashRaw[k].c_str());
    if (!audioSys.flashPlayer[k].isPlaying()) {
    Serial.printf("FSR %d sample playback did not start as expected\n", k);
    }
  } else {
    /*
      Else fallback baked-in approach:

      Debugging: 
      */
      Serial.printf("FSR %d playing baked-in sample\n", k);
      /*
      
    */
    if (analogRead(A17)>=THRESH) {
       audioSys.playMem1.play(AudioSampleHh2khronos);
    }
    if (analogRead(A16)>=THRESH) {
       audioSys.playMem2.play(AudioSampleSnarekhronos);
    }
    if (analogRead(A15)>=THRESH) {
       audioSys.playMem3.play(AudioSampleHh1khronos);
    }
    if (analogRead(A14)>=THRESH) {
       audioSys.playMem4.play(AudioSampleKickkhronos);
    }
      //   // Play baked-in samples as fallback
      // if (k == 0) playMem1.play(AudioSampleHh2khronos);
      // if (k == 1) playMem2.play(AudioSampleSnarekhronos);
      // if (k == 2) playMem3.play(AudioSampleHh1khronos);
      // if (k == 3) playMem4.play(AudioSampleKickkhronos);
  }
}
