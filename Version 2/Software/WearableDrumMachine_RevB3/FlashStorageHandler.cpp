// FlashStorageHandler.cpp
#include "FlashStorageHandler.h"
#include "Globals.h"
#include "AudioSystem.h"
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



// If you have a global instance of AudioSystem, you might do:
// extern AudioSystem audioSys;

//===================================================================
// doSdToFlashTransfer: 
//   Called if user taps an FSR while in browse mode
//   to copy from /Samples/foo.wav => "FSR_n.raw" on flash

// AudioConnection something(translator_mixer, 0, queue1, 0);
// and a while loop reading queue1.available() 256-byte chunks. But that is not happening in doSdToFlashTransfer().
//===================================================================
// Update doSdToFlashTransfer to include currentlyTransferring
bool doSdToFlashTransfer(const String &sdWavPath, int fsrIndex) {     // NEW function
    if (!SD.exists(sdWavPath.c_str())) {
        Serial.printf("File missing on SD: %s\n", sdWavPath.c_str());
        return false;
    }

    audioSys.playSdWav1.play(sdWavPath.c_str());
    delay(5); // Allow playback to start

    if (!audioSys.playSdWav1.isPlaying()) {
        Serial.printf("Failed to start playback for: %s\n", sdWavPath.c_str());
        return false;
    }

    unsigned long wavLengthMs = audioSys.playSdWav1.lengthMillis();
    unsigned long approxBytes = (unsigned long)(wavLengthMs * 88.2); // Estimate size
    approxBytes = (approxBytes + 255) & 0xFFFFFF00; // Align to 256 bytes

    char rawName[32];
    snprintf(rawName, sizeof(rawName), "FSR_%d.raw", fsrIndex);

    if (SerialFlash.exists(rawName)) {
        Serial.printf("Removing existing file: %s\n", rawName);
        SerialFlash.remove(rawName);
    }

    if (!SerialFlash.create(rawName, approxBytes)) {
        Serial.printf("Failed to create flash file: %s\n", rawName);
        audioSys.playSdWav1.stop();
        return false;
    }

    SerialFlashFile transferFile = SerialFlash.open(rawName);
    if (!transferFile) {
        Serial.printf("Failed to open flash file: %s\n", rawName);
        audioSys.playSdWav1.stop();
        return false;
    }

    //AudioInterrupts();   // NEW

    audioSys.queue1.begin(); // Start recording from the audio stream
    unsigned long bytesWritten = 0;

    Serial.printf("Recording %s to flash (%lu bytes)...\n", sdWavPath.c_str(), approxBytes);

    while (audioSys.playSdWav1.isPlaying() || audioSys.queue1.available() > 0) {
      continueCopyingToFlash(transferFile, approxBytes - bytesWritten);     // comment this line out and uncomment below logic to unmodularize the writing of queue audio to flash (Where user presses FSR in BROWSE mode to assign sample)
        // if (queue1.available() > 0) {
        //     transferFile.write(queue1.readBuffer(), 256); // Write 256-byte chunks
        //     queue1.freeBuffer();
        //     bytesWritten += 256;

        //     if (bytesWritten >= approxBytes) {
        //         Serial.println("Reached allocated file size.");
        //         break;
        //     }
        // }
    }


    audioSys.queue1.end(); // Stop recording
    //queue1.clear();         // NEW
    transferFile.close();
    audioSys.playSdWav1.stop();

    Serial.printf("Finished recording %s (%lu bytes written).\n", rawName, bytesWritten);

    fsrFlashRaw[fsrIndex] = String(rawName);
    fsrSamplePath[fsrIndex] = ""; // Clear SD path as it's now on flash
    return true;
}


void continueCopyingToFlash(SerialFlashFile &file, unsigned long fileSize) {      /// NEW function
    while (audioSys.queue1.available() > 0) {
        if (fileSize <= 0) {
            audioSys.queue1.end();
            return;
        }

        file.write(audioSys.queue1.readBuffer(), 256);
        audioSys.queue1.freeBuffer();
        fileSize -= 256;
    }
}
