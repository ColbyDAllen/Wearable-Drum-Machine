// SdBrowse.cpp

#include "SdBrowse.h"
#include "Globals.h"
#include "AudioSystem.h"
#include "FlashStorageHandler.h"
#include "AudioSystem.h"
extern AudioSystem audioSys; // the global instance

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


void SDBrowse::scanSdForWavFiles(const char *folder){
  totalFiles=0;

  // playSdWav1.stop();     // NEW
  // AudioNoInterrupts();   // NEW


  File dir = SD.open(folder);
  if(!dir){
    Serial.print("Can't open folder: ");
    Serial.println(folder);
    return;
  }
  while(true){
    File entry=dir.openNextFile();
    if(!entry) break;
    if(!entry.isDirectory()){
      String fn= String(entry.name());
      fn.toLowerCase();
      if(fn.endsWith(".wav")){
        fileList[totalFiles]= String(folder)+"/"+fn;
        Serial.print("Found WAV: "); 
        Serial.println(fileList[totalFiles]);
        totalFiles++;
        if(totalFiles>=MAX_FILES) break;
      }
    }
    entry.close();
  }
  dir.close();
  browseIndex=0;
}

// Update previewWavFile to handle isPreviewing
void SDBrowse::previewWavFile(const String &path) {
    audioSys.playSdWav1.stop();
    if (!SD.exists(path.c_str())) {
        Serial.printf("File missing on SD: %s\n", path.c_str());
        return;
    }
    isPreviewing = true;
    Serial.printf("Previewing: %s\n", path.c_str());

    audioSys.playSdWav1.play(path.c_str());    // TODO: check if we need some sort of "flushing" mechanism to get the browse mode to not freeze after 3 to 5 toggles of the undopin i.e. browsing different samples.
    delay(5); // delay(80); // old 80, new 5
    if (!audioSys.playSdWav1.isPlaying()) {
        Serial.println("...not playing??");
        isPreviewing = false;
        return;
    }

    // Mark that a preview is in progress and record the start time
    // we do not loop waiting for 2 seconds. Instead, we set a flag (previewActive = true) and store previewStart = millis(). Then we immediately return to the main loop().
    previewActive = true;     // NEW
    previewStart = millis();  // NEW

    // // NEW
    // bool previewActive = true;
    // if (previewActive) {
    //   if (!playSdWav1.isPlaying() || millis() - startT > 2000) {
    //       playSdWav1.stop();
    //       previewActive = false;
    //   }
    // }

    Serial.println("...playing ~2s preview");
    unsigned long startT = millis();
    // while (playSdWav1.isPlaying()) {
    //     if (millis() - startT > 2000) {
    //         Serial.println("Stopping preview after 2s...");
    //         break;
    //     }
    //     delay(10);
    // }
    // playSdWav1.stop();
    // isPreviewing = false;
}

void SDBrowse::sampleBrowse() {
  static bool sampleBrowsePinState = false;
  bool readPin = digitalRead(sampleBrowsePin);

  // Toggle browse mode
  if (readPin != sampleBrowsePinState && millis() - trigTime > debounceTime) {
    sampleBrowsePinState = !sampleBrowsePinState;
    trigTime = millis();

    if (sampleBrowsePinState == LOW) {
      inBrowseMode = !inBrowseMode;
      if (inBrowseMode) {
        Serial.println("\n=== ENTER BROWSE MODE ===");
        scanSdForWavFiles("/Samples");
        if (totalFiles > 0) {
          previewWavFile(fileList[browseIndex]);
        } else {
          Serial.println("No .WAV files in /Samples!");
        }
      } else {
        Serial.println("=== EXIT BROWSE MODE ===");
      }
    }
  }

  if (!inBrowseMode) return;

  // Handle undoPin for cycling through files
  static bool undoPinState = false;
  bool readUndo = digitalRead(undoPin);
  if (readUndo != undoPinState && millis() - trigTime > debounceTime) {
    undoPinState = !undoPinState;
    trigTime = millis();

    if (undoPinState == LOW) {
      if (totalFiles > 0) {
        // Move to next file (wrap around at end)
        browseIndex++;
        if (browseIndex >= totalFiles) {
          browseIndex = 0;
        }
        // Non-blocking preview
        previewWavFile(fileList[browseIndex]);
      }
    }
  }

  // Assign selected file to an FSR on tap
  for (int i = 0; i < FSRs; i++) {
    int reading = analogRead(FSRpin[i]);
    if (reading > THRESH) {
      if (totalFiles > 0) {
        Serial.printf("Assigning & transferring %s to FSR #%d...\n", fileList[browseIndex].c_str(), i);
        bool success = doSdToFlashTransfer(fileList[browseIndex], i);
        if (success) {
          Serial.printf("FSR #%d successfully assigned to %s\n", i, fileList[browseIndex].c_str());
        } else {
          Serial.println("Transfer failed or was aborted!");
        }
      }
      delay(500); // Debounce
    }
  }
}

void SDBrowse::updatePreview() {
  // NEW Check if we're currently previewing a file
  if (previewActive) {// NEW
    // If the WAV file has finished, or if 2 seconds have passed, stop the preview// NEW
    if (!audioSys.playSdWav1.isPlaying() || (millis() - previewStart) > 2000) {// NEW
      audioSys.playSdWav1.stop();// NEW
      previewActive = false;// NEW
      Serial.println("Preview finished/stopped.");  // NEW
    }
  }
}


// void listFlashFiles() {
//   Serial.println("Listing files on SerialFlash:");
//   SerialFlash.opendir();
//   char filename[64];
//   while (SerialFlash.readdir(filename, sizeof(filename))) {
//     Serial.print("Found file: ");
//     Serial.println(filename);
//   }
// }
