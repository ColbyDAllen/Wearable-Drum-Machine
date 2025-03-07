// MIDIFileWriter.h

#ifndef MIDI_FILE_WRITER_H
#define MIDI_FILE_WRITER_H

#include <Arduino.h> // so byte is recognized
#include <MIDI.h> 
#include "Config.h"
#include "Globals.h"


class MIDIFileWriter {
public:
    void creatNextFile();
    void writeMidiPreamble();

    void checkForMarker();
    void writeMidiMarker();

    void updateFile();
    void checkReset();

    int getDelta();
    void writeToFile(byte eventType, byte b1, byte b2, int delta);
    void writeVarLen(File file, unsigned long value);
    //void writeVarLen(File &file, unsigned long value);
private:
    // private data for MIDI
    // e.g. ring buffers, or references to global state
};


#endif