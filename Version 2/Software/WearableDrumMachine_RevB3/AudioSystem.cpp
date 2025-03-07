// AudioSystem.cpp

#include "AudioSystem.h"

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

void AudioSystem::initAudioSystem() {
    // If the monolith had AudioMemory() calls here, do so:
    AudioMemory(24);

    // If the monolith had sgtl5000_1.enabled(), set volume, etc., do that too:
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.7);
    sgtl5000_1.adcHighPassFilterDisable(); // //TODO: Look into this.... enabled may result in less power draw i.e. no low frequencies? 
    // etc.

    // If the monolith sets mixer1 gains:
    mixer1.gain(0, 0.4); 
    mixer1.gain(1, 0.4); 
    mixer1.gain(2, 0.4); 
    mixer1.gain(3, 0.4); 

    translator_mixer.gain(0,1.0); 
    translator_mixer.gain(1,1.0); 
    translator_mixer.gain(2,0.0); 
    translator_mixer.gain(3,0.0); 

    finalMixer.gain(0,1.0); 
    finalMixer.gain(1,1.0); 
    finalMixer.gain(2,1.0); 
    finalMixer.gain(3,0.0); 

    // We'll also have flashMixer for the 4 flash players 
    flashMixer.gain(0,1.0); 
    flashMixer.gain(1,1.0); 
    flashMixer.gain(2,1.0); 
    flashMixer.gain(3,1.0); 

    // In setup(), initialize the patch cord to connect translator_mixer output to queue1: // the new patch cord for real-time capturing 
    patchCordQueue = new AudioConnection(translator_mixer, 0, queue1, 0); 

    // Build patch cords from translator_mixer => finalMixer(0), 
    // mixer1 => finalMixer(1), flashMixer => finalMixer(2) 
    patchCord5 = new AudioConnection(translator_mixer, 0, finalMixer, 0); 
    patchCord6 = new AudioConnection(mixer1, 0, finalMixer, 1); 
    flashMixerToFinal = new AudioConnection(flashMixer, 0, finalMixer, 2); patchCord7 = new AudioConnection(finalMixer, 0, i2s1, 0); 
    patchCord8 = new AudioConnection(finalMixer, 0, i2s1, 1);

    // Ensure all flashPlayer objects are stopped at startup 
    for (int i = 0; i < 4; i++) { 
      flashPlayer[i].stop(); // Ensures clean state for each flashPlayer 
    } 

    // Establish connections between flashPlayers and flashMixer
    for(int i=0; i<4; i++){ 
      flashPatch[i] = new AudioConnection(flashPlayer[i], 0, flashMixer, i); 
    } 
    






    // Now replicate the patch cords:
    // Instead of static constructor style, we do:
    patchCord1 = new AudioConnection(playMem3, 0, mixer1, 2);
    patchCord2 = new AudioConnection(playMem4, 0, mixer1, 3);
    patchCord3 = new AudioConnection(playMem2, 0, mixer1, 1);
    patchCord4 = new AudioConnection(playMem1, 0, mixer1, 0);

    patchCordWavLeft  = new AudioConnection(playSdWav1, 0, translator_mixer, 0);
    patchCordWavRight = new AudioConnection(playSdWav1, 1, translator_mixer, 1);



}
