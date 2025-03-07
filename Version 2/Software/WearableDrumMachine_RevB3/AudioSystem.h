// AudioSystem.h

#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include "Config.h"

// --- Place your baked-in sample headers here ---
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

// The AudioSystem class holds all audio objects for baked-in memory playback.
class AudioSystem {
public:
    void initAudioSystem();
    // The actual Teensy Audio objects:
    AudioPlayMemory     playMem3;
    AudioPlayMemory     playMem4;
    AudioPlayMemory     playMem2;
    AudioPlayMemory     playMem1;
    AudioMixer4         mixer1;
    AudioOutputI2S      i2s1;
    
    // We'll store pointers for the patch cords, to create them dynamically
    AudioConnection* patchCord1 = nullptr;
    AudioConnection* patchCord2 = nullptr;
    AudioConnection* patchCord3 = nullptr;
    AudioConnection* patchCord4 = nullptr;

    // Stereo Audio Codec Object
    AudioControlSGTL5000 sgtl5000_1;

    // NEW: single WAV player + new mixers
    AudioPlaySdWav  playSdWav1;
    AudioMixer4     translator_mixer; 
    AudioMixer4     finalMixer;

    // We'll store dynamic patch cords as pointers again:
    AudioConnection* patchCordWavLeft  = nullptr;
    AudioConnection* patchCordWavRight = nullptr;

    // We'll define them in setup() for clarity:  
    AudioConnection* patchCord5 = nullptr;
    AudioConnection* patchCord6 = nullptr;
    AudioConnection* patchCord7 = nullptr;
    AudioConnection* patchCord8 = nullptr;

    //=================================================================== 
    // SerialFlash-based Playback 
    // We'll record from "playSdWav1 -> translator_mixer -> queue" 
    // into a .raw on flash, then later play from that .raw via AudioPlaySerialflashRaw 
    //=================================================================== 

    AudioRecordQueue queue1; // for capturing the real-time data from translator_mixer 
    AudioConnection* patchCordQueue = nullptr; // dynamic connection for real-time capturing 

    // We'll have 4 flash players, so each FSR can trigger its own
    AudioPlaySerialflashRaw flashPlayer[4];
    AudioMixer4 flashMixer; // merges up to 4 players 
    AudioConnection* flashPatch[4]; 
    AudioConnection* flashMixerToFinal = nullptr;

};

#endif