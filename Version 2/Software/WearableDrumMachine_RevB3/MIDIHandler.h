// MIDIHandler.h

#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <Arduino.h> // so byte is recognized
#include <MIDI.h> 


class MIDIHandler {
public:
    void initMIDI();
    void updateMIDI();  // calls usbMIDI.read() internally

    void Sequence_PANIC();
    void Track_PANIC();
    void Slim_PANIC();
    void FULL_PANIC();
    void MIDI_PANIC();
    void RESETMIDI();

    void FULL_LED_Blink(byte count);

    void Handle_Stop();
  
private:
    
    // private data for MIDI
    // e.g. ring buffers, or references to global state
};

void myNoteOn(byte channel, byte note, byte velocity);
void myNoteOff(byte channel, byte note, byte velocity);
void myControlChange(byte channel, byte control, byte value);
void myAfterTouchPoly(byte channel, byte note, byte pressure);
void Handle_Start();


#endif
