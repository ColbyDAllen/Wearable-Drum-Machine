// File and MIDI handling
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

// INDEX # -->  0     1     2    3
int FSRpin[] = {A17, A16, A15, A14};
const int FSRs = 4;  
int Note [] = {60, 61, 62, 63};  
int counter [FSRs];         
int VELMASK = 0;
int ATMASK = 0;
int AFTERTHRESH = 50; 
int THRESH = 45; 
int VELTIME = 500;   
int AFTERTIME = 2500;  
int MIDIMIN = 20;  
void NoteOnSend (int);   
void PolyTouchSend (int);
//void testingArguments (int);  

int channel [] = {0, 1, 2, 3};    
const int SAMPLEs = 4;
int counter2 [SAMPLEs]; 
void SampleOnSend (int);    
void gain (unsigned int channel, float gain);

// Our Real Time Clock
#include <RTClib.h>
RTC_DS3231 RTC;
bool HAS_RTC = false;

// Audio pins and values
#define AUDIO 8
#define AUDIO_DEBUG_PIN 3
int lastPlayState = 0;
bool play = false;

// Marker pins and values
#define PLACE_MARKER_PIN 5
int lastMarkState = 0;
int nextMarker = 1;

const int chipSelect = BUILTIN_SDCARD;  // Change to "10" for audio shield microSD 
#define CHIP_SELECT BUILTIN_SDCARD  // Change to "10" for audio shield microSD

#define HAS_MORE_BYTES 0x80           /* it's not "note off, channel 1", 
                                      // it's the bit mask 0x01000000, 
                                      // used to set the the 7th bit to 1 
                                      // as per the spec requirement 
                                      // for variable length quantities.
                                      
#define NOTE_OFF_EVENT 0x80           // Status Byte = 1000 nnnn
#define NOTE_ON_EVENT 0x90            // Status Byte = 1001 nnnn
#define CONTROL_CHANGE_EVENT 0xB0     // Status Byte = 1011 nnnn
#define PITCH_BEND_EVENT 0xE0         // Status Byte = 1110 nnnn
#define AFTER_TOUCH_POLY_EVENT 0xA0   // Status Byte = 1010 nnnn

// we use a 2 minute idling timeout (in millis)
#define RECORDING_TIMEOUT 120000
unsigned long lastLoopCounter = 0;
unsigned long loopCounter = 0;

unsigned long startTime = 0;
unsigned long lastTime = 0;

#define FILE_FLUSH_INTERVAL 400
String filename;
File file;

// GUItool: begin automatically generated code
AudioPlayMemory          playMem3;       //xy=248.75,260.0000057220459
AudioPlayMemory          playMem4;       //xy=248.75,325.0000057220459
AudioPlayMemory          playMem2;       //xy=251.25,193.7500057220459
AudioPlayMemory          playMem1;       //xy=256.25,135.0000057220459
AudioMixer4              mixer1;         //xy=498.75000762939453,230.00000381469727
AudioOutputI2S           i2s1;           //xy=735.0000114440918,230.00000190734863
AudioConnection          patchCord1(playMem3, 0, mixer1, 2);
AudioConnection          patchCord2(playMem4, 0, mixer1, 3);
AudioConnection          patchCord3(playMem2, 0, mixer1, 1);
AudioConnection          patchCord4(playMem1, 0, mixer1, 0);
AudioConnection          patchCord5(mixer1, 0, i2s1, 0);
AudioConnection          patchCord6(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=433.75000762939453,493.7500057220459
// GUItool: end automatically generated code

MIDI_CREATE_DEFAULT_INSTANCE();

/**
   Set up our inline MIDI recorder
*/
void setup() {
  //MIDI.begin();
  usbMIDI.begin();
  Serial.begin (32500);
  //Serial.begin (31250); //either baud rate works
  AudioMemory(10);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  mixer1.gain(0, 0.4);
  mixer1.gain(1, 0.4);
  mixer1.gain(2, 0.4);
  mixer1.gain(3, 0.4);

//==========================================================================
  // set up MIDI handling
  MIDI.begin(MIDI_CHANNEL_OMNI);
  usbMIDI.begin();

  usbMIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandleControlChange(myControlChange);
  usbMIDI.setHandlePitchChange(myPitchChange);
  usbMIDI.setHandleAfterTouchPoly(myAfterTouchPoly);

  /*
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleControlChange(handleControlChange);
  //MIDI.setHandleAfterTouchPoly(handlePolyAfterTouch);
  */

//==========================================================================
  // set up the tone playing button
  pinMode(AUDIO_DEBUG_PIN, INPUT);
  pinMode(AUDIO, OUTPUT);
  tone(AUDIO, 440, 200);

  // set up the MIDI marker button
  pinMode(PLACE_MARKER_PIN, INPUT);

  // set up RTC interfacing
  if (RTC.begin()) {
    // uncomment this line to set the current date/time on the RTC
    // RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));

    // if the RTC works, we can tell the SD library
    // how it can check for the current time when it
    // needs timestamping for file creation/writing.
    SdFile::dateTimeCallback(dateTime);
    HAS_RTC = true;
    tone(AUDIO, 880, 100);
  }

  // set up SD card functionality and allocate a file
  pinMode(CHIP_SELECT, OUTPUT);
  if (SD.begin(CHIP_SELECT)) {
    creatNextFile();
    if (file) {
      writeMidiPreamble();
      tone(AUDIO, 1760, 100);
    }
  }
}

void dateTime(uint16_t* date, uint16_t* time) {
  DateTime d = RTC.now();
  *date = FAT_DATE(d.year(), d.month(), d.day());
  *time = FAT_TIME(d.hour(), d.minute(), d.second());
}

/**
    We could use the EEPROM to store this number,
    but since we're not going to get timestamped
    files anyway, just looping is also fine.
*/
void creatNextFile() {
  for (int i = 1; i < 1000; i++) {
    filename = "file-";
    if (i < 10) filename += "0";
    if (i < 100) filename += "0";
    filename += String(i);
    filename += String(".mid");

    if (!SD.exists(filename.c_str())) {
      file = SD.open(filename.c_str(), FILE_WRITE);
      return;
    }
  }
}

/**
   Set up a new MIDI file with some boiler plate byte code
*/
void writeMidiPreamble() {
  byte header[] = {
    0x4D, 0x54, 0x68, 0x64,   // "MThd" chunk
    0x00, 0x00, 0x00, 0x06,   // chunk length (from this point on)
    0x00, 0x00,               // format 0
    0x00, 0x01,               // one track
    0x01, 0xD4                // data rate = 458 ticks per quarter note
  };
  file.write(header, 14);

  byte track[] = {
    0x4D, 0x54, 0x72, 0x6B,   // "MTrk" chunk
    0x00, 0x00, 0x00, 0x00    // chunk length placeholder (MSB)
  };
  file.write(track, 8);

  byte tempo[] = {
    0x00,                     // time delta (of zero)
    0xFF, 0x51, 0x03,         // tempo op code
    0x06, 0xFD, 0x1F          // real rate = 458,015μs per quarter note (= 134.681 BPM)
  };
  file.write(tempo, 7);
}

/**
   The program loop consists of flushing our file to disk,
   checking our buttons to see if they just got pressed,
   and then handling MIDI input, if there is any.
*/
void loop() {
    for (int i = 0; i < FSRs; i++) {
    int FSRRead = analogRead(FSRpin[i]);
    if (FSRRead > THRESH) {
      counter[i] ++;
      if (!(VELMASK & (1 << i)) && (counter[i] == VELTIME)) {
        VELMASK |= (1 << i);                   
        counter [i] = 0;
        NoteOnSend (i);
        //testingArguments (i);
        SampleOnSend (i);                                      
        }
      if (counter [i] == AFTERTIME) {
        counter [i] = 0;
        PolyTouchSend(i);
      }
    }
    else {                                                    
      if (VELMASK & (1 << i)) {           
        //MIDI.sendNoteOff (Note[i], 0, 1);
        usbMIDI.sendNoteOff (Note[i], 0, 1);
        myNoteOff(1, Note[i], 0);
        VELMASK &= ~ (1 << i);                 
        counter [i] = 0;
      }
    }
  }
  checkForMarker();
  setPlayState();
  updateFile();
  //MIDI.read();
  usbMIDI.read();
}


// ======================================================================================


/**
   We flush the file's in-memory content to disk
   every 400ms, allowing. That way if we take the
   SD card out, it's basically impossible for any
   data to have been lost.
*/
void updateFile() {
  loopCounter = millis();
  if (loopCounter - lastLoopCounter > FILE_FLUSH_INTERVAL) {
    checkReset();
    lastLoopCounter = loopCounter;
    file.flush();
  }
}

/**
   This "function" would normally crash any kernel that tries
   to run it by violating memory access. Instead, the Arduino's
   watchdog will auto-reboot, giving us a software "reset".
*/
void(* resetArduino) (void) = 0;

/**
  if we've not received any data for 2 minutes, and we were
  previously recording, we reset the arduino so that when
  we start playing again, we'll be doing so in a new file,
  rather than having multiple sessions with huge silence
  between them in the same file.
*/
void checkReset() {
  if (startTime == 0) return;
  if (!file) return;
  if (millis() - lastTime > RECORDING_TIMEOUT) {
    file.close();
    resetArduino();
  }
}

/**
   A little audio-debugging: pressing the button tied to the
   audio debug pin will cause the program to play notes for
   every MIDI note-on event that comes flying by.
*/
void setPlayState() {
  int playState = digitalRead(AUDIO_DEBUG_PIN);
  if (playState != lastPlayState) {
    lastPlayState = playState;
    if (playState == 1) {
      play = !play;
    }
  }
}

/**
   This checks whether the MIDI marker button got pressed,
   and if so, writes a MIDI marker message into the track.
*/
void checkForMarker() {
  int markState = digitalRead(PLACE_MARKER_PIN);
  if (markState  != lastMarkState) {
    lastMarkState = markState;
    if (markState == 1) {
      writeMidiMarker();
    }
  }
}

/**
  Write a MIDI marker to file, by writing a delta, then
  the op code for "midi marker", the number of letters
  the marker label has, and then the label (using ASCII).

  For simplicity, the marker labels will just be a
  sequence number starting at "1".
*/
void writeMidiMarker() {
  if (!file) return;

  // delta + event code
  writeVarLen(file, getDelta());
  file.write(0xFF);
  file.write(0x06);                             // https://www.recordingblogs.com/wiki/midi-marker-meta-message#:~:text=The%20second%20byte%20is%20the,marker%20comment%20in%20ASCII%20text.
                                                // Oddson using usbmidi.h -- https://forum.pjrc.com/threads/38367-Teensy-2-0-USB-MIDI-and-SysEx
  // If we have an RTC available, we can write the clock time
  // Otherwise,  write a sequence number.

  if (HAS_RTC) {
    DateTime d = RTC.now();
    byte len = 20;
    writeVarLen(file, len);

    char marker[len]; // will hold strings like "2021/01/23, 10:53:31"
    sprintf(marker, "%04d/%02d/%02d, %02d:%02d:%02d", d.year(), d.month(), d.day(), d.hour(), d.minute(), d.second());
    file.write(marker, len);
  }

  else {
    // how many letters are we writing?
    byte len = 1;
    if (nextMarker > 9) len++;
    if (nextMarker > 99) len++;
    if (nextMarker > 999) len++;
    writeVarLen(file, len);

    // our label:
    byte marker[len];
    String(nextMarker++).getBytes(marker, len);
    file.write(marker, len);
  }
}

// ======================================================================================
void NoteOnSend (int j) {
  int FSRRead = analogRead(FSRpin [j]);                       
  int velocity = map (FSRRead, 0, 800, MIDIMIN, 127);     
  //MIDI.sendNoteOn (Note[j], velocity, 1);
  usbMIDI.sendNoteOn (Note[j], velocity, 1);
  myNoteOn(1, Note[j], velocity);
}

// ======================================================================================

void PolyTouchSend (int j) {                                 
  int FSRRead = analogRead(FSRpin [j]);
  if (FSRRead > AFTERTHRESH) {
    int pressure = map (FSRRead, 0, 800, MIDIMIN, 127);     
    //MIDI.sendPolyPressure (Note[j], aftertouch, 1)
    usbMIDI.sendPolyPressure (Note[j], pressure, 1);
    myAfterTouchPoly(1, Note[j], pressure);
  }
}

// ======================================================================================
/*
  Reference to callbacks in void setup():
    usbMIDI.setHandleNoteOff(myNoteOff);
    usbMIDI.setHandleNoteOn(myNoteOn);
    usbMIDI.setHandleControlChange(myControlChange);
    usbMIDI.setHandlePitchChange(myPitchChange);
    //usbMIDI.setHandleAfterTouchPoly(myAfterTouchPoly);

    Order in which parameters are stored to writeToFile(); function:
      writeToFile(byte eventType, byte b1, byte b2, int delta) -> void
*/

// usbMIDI.h callbacks / writeToFile hand-offs ////
void myNoteOff(byte channel, byte note, byte velocity) {
  writeToFile(NOTE_OFF_EVENT, note, velocity, getDelta());
}
void myNoteOn(byte channel, byte note, byte velocity) {
  writeToFile(NOTE_ON_EVENT, note, velocity, getDelta());         
  if (play) tone(AUDIO, 440 * pow(2, (note - 69.0) / 12.0), 100);
}
void myControlChange(byte channel, byte control, byte value) {
  writeToFile(CONTROL_CHANGE_EVENT, control, value, getDelta());
}
void myPitchChange(byte channel, int bend) {
  bend += 0x2000; // MIDI bend uses the range 0x0000-0x3FFF, with 0x2000 as center. Double the size of other data byte channel messages. A 14-bit thing.
  byte lsb = bend & 0x7F;
  byte msb = bend >> 7;
  writeToFile(PITCH_BEND_EVENT, lsb, msb, getDelta());
}
void myAfterTouchPoly(byte channel, byte note, byte pressure)  {
  writeToFile(AFTER_TOUCH_POLY_EVENT, note, pressure, getDelta());
}



// MIDI.h callbacks / writeToFile hand-offs ////
/*
void handleNoteOff(byte channel, byte pitch, byte velocity) {
  writeToFile(NOTE_OFF_EVENT, pitch, velocity, getDelta());
}
void handleNoteOn(byte channel, byte pitch, byte velocity) {
  writeToFile(NOTE_ON_EVENT, pitch, velocity, getDelta());
  if (play) tone(AUDIO, 440 * pow(2, (pitch - 69.0) / 12.0), 100);
}
void handleControlChange(byte channel, byte cc, byte value) {
  writeToFile(CONTROL_CHANGE_EVENT, cc, value, getDelta());
}
void handlePitchBend(byte channel, int bend) {
  bend += 0x2000; // MIDI bend uses the range 0x0000-0x3FFF, with 0x2000 as center.
  byte lsb = bend & 0x7F;
  byte msb = bend >> 7;
  writeToFile(PITCH_BEND_EVENT, lsb, msb, getDelta());
}
*/

/*
void handlePolyAfterTouch(byte channel, byte pitch, byte pressure)  {
  writeToFile(AFTER_TOUCH_POLY_EVENT, pitch, pressure, getDelta());
}
*/

// ======================================================================================

void SampleOnSend (int k) {                                     /*Function added to Adrian's sketch for getting 
                                                                audio samples to play simulataneously with existing 
                                                                MIDI output signal coded in void 
                                                                NoteOnSend(int j) function*/
  analogReadResolution(7);
  int FSRRead2 = analogRead(FSRpin [k]);
  float gain = map (FSRRead2, 0, 127, .2, 1.0);
  mixer1.gain(channel[k], gain);
  if (analogRead(A17) >= THRESH) {
    playMem1.play(AudioSampleHh2khronos); }
  if (analogRead(A16) >= THRESH) {
    playMem2.play(AudioSampleSnarekhronos); }
  if (analogRead(A15) >= THRESH) {
    playMem3.play(AudioSampleHh1khronos); }
  if (analogRead(A14) >= THRESH) {
    playMem4.play(AudioSampleKickkhronos); }
}

// ======================================================================================

/**
   This calculates the number of ticks since the last MIDI event
*/
int getDelta() {
  if (startTime == 0) {
    // if this is the first event, even if the Arduino's been
    // powered on for hours, this should be delta zero.
    startTime = millis();
    lastTime = startTime;
    return 0;
  }
  unsigned long now = millis();
  unsigned int delta = (now - lastTime);        // Think "x2-x1"
  lastTime = now;                               // Reset "lastTime" for next iteration
  return delta;
}

/**
   Write "common" MIDI events to file, where common MIDI events
   all use the following data format:

     delta     eventType    b1     b2
     <delta> <event code> <byte> <byte>

   See the "Standard MIDI-File Format" for more information -- 
   http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html
*/
/*
  Try:
    usbMIDI.getData1()
    usbMIDI.getData2()
    usbMIDI.getChannel()
  ...if data isn't passing from " usbMIDI.h callbacks / writeToFile hand-offs "
*/
void writeToFile(byte eventType, byte b1, byte b2, int delta) {
  if (!file) return;
  writeVarLen(file, delta);             
  file.write(eventType);
  file.write(b1);
  file.write(b2);
}

/**
   Encode a unsigned 32 bit integer as variable-length byte sequence
   of, at most, 4 7-bit-with-has-more bytes. This function is supplied
   as part of the MIDI file format specification.
*/
void writeVarLen(File file, unsigned long value) {
  // capture the first 7 bit block         
  unsigned long buffer = value & 0x7f;    

  // shift in 7 bit blocks with "has-more" bit from the
  // right for as long as `value` has more bits to encode.
  while ((value >>= 7) > 0) {
    buffer <<= 8;
    buffer |= HAS_MORE_BYTES;              
    buffer |= value & 0x7f;
  }

  // Then unshift bytes one at a time for as long as the has-more bit is high.
  while (true) {
    file.write((byte)(buffer & 0xff));
    if (buffer & HAS_MORE_BYTES) {
      buffer >>= 8;
    } else {
      break;
    }
  }
}

/*
void testingArguments (int q) {

}
*/

/**
  Citations and Useful Links
    1--https://github.com/Pomax/arduino-midi-recorder
      Pomax's "MIDI Field Recorder" 
    2--https://forum.pjrc.com/threads/31797-Teensy-FSR-based-MIDI-controller
      Adrian's FSR Sensor Sketch
    3--https://www.pjrc.com/teensy/gui/
      PRJC's Auto-Generating Code GUI
*/