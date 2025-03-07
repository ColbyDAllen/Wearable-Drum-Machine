// MIDIFileWriter.cpp

#include "Globals.h"
#include "MIDIFileWriter.h"

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
// creatNextFile & writeMidiPreamble
//===================================================================
void MIDIFileWriter::creatNextFile() {
  for(int i=1; i<1000; i++){
    filename = "file-";
    if(i<10)   filename += "0";
    if(i<100)  filename += "0";
    filename += String(i) + ".mid";
    if(!SD.exists(filename.c_str())){
      file = SD.open(filename.c_str(), FILE_WRITE);
      return;
    }
  }
}

// void creatNextFile() {
//     // from monolith
//     for(int i=1; i<1000; i++){
//         filename = "file-";
//         // ...
//         // open file on SD, set file = SD.open(...)
//     }
// }

void MIDIFileWriter::writeMidiPreamble() {
  byte header[] = {
    0x4D,0x54,0x68,0x64, 
    0x00,0x00,0x00,0x06, 
    0x00,0x00, 
    0x00,0x01,
    0x01,0xD4
  };
  if(file) file.write(header,14);

  byte track[] = {
    0x4D,0x54,0x72,0x6B,
    0x00,0x00,0x00,0x00
  };
  if(file) file.write(track,8);

  byte tempo[] = {
    0x00,
    0xFF,0x51,0x03,
    0x06,0xFD,0x1F
  };
  if(file) file.write(tempo,7);
}

//===================================================================
// updateFile, checkReset, checkForMarker, writeMidiMarker
//===================================================================
void MIDIFileWriter::updateFile() {
  loopCounter = millis();
  if(loopCounter - lastLoopCounter > FILE_FLUSH_INTERVAL){
    checkReset();
    lastLoopCounter = loopCounter;
    if(file) file.flush();
  }
}

void(* resetArduino)(void)=0;

void MIDIFileWriter::checkReset(){
  if(startTime==0) return;
  if(!file) return;
  if(millis()-lastTime > RECORDING_TIMEOUT){
    file.close();
    resetArduino();
  }
}



void MIDIFileWriter::checkForMarker() {
  int markState = digitalRead(PLACE_MARKER_PIN);
  if(markState!=lastMarkState){
    lastMarkState = markState;
    if(markState==1){
      writeMidiMarker();
    }
  }
}


  
void MIDIFileWriter::writeMidiMarker() {
  if(!file) return;
  writeVarLen(file, getDelta());
  file.write(0xFF);
  file.write(0x06);

  if(HAS_RTC){
    DateTime d = RTC.now();
    byte len=20;
    writeVarLen(file,len);
    char marker[len];
    sprintf(marker, "%04d/%02d/%02d, %02d:%02d:%02d",
            d.year(), d.month(), d.day(),
            d.hour(), d.minute(), d.second());
    file.write(marker, len);
  } else {
    byte len=1;
    if(nextMarker>9)   len++;
    if(nextMarker>99)  len++;
    if(nextMarker>999) len++;
    writeVarLen(file,len);
    byte marker[len];
    String(nextMarker++).getBytes(marker,len);
    file.write(marker,len);
  }
}


//===================================================================
// getDelta, writeToFile, writeVarLen
//===================================================================
int MIDIFileWriter::getDelta() {
  if(startTime==0){
    startTime=millis();
    lastTime=startTime;
    return 0;
  }
  unsigned long now=millis();
  unsigned int delta=(now-lastTime);
  lastTime=now;
  return delta;
}

void MIDIFileWriter::writeToFile(byte eventType, byte b1, byte b2, int delta){
  if(!file) return;
  writeVarLen(file, delta);
  file.write(eventType);
  file.write(b1);
  file.write(b2);
}

//void MIDIFileWriter::writeVarLen(File &file, unsigned long value){
void MIDIFileWriter::writeVarLen(File file, unsigned long value){
  unsigned long buffer=value & 0x7f;
  while((value>>=7)>0){
    buffer<<=8;
    buffer |= 0x80;
    buffer |= value & 0x7f;
  }
  while(true){
    file.write((byte)(buffer & 0xff));
    if(buffer & 0x80){
      buffer>>=8;
    } else {
      break;
    }
  }
}

