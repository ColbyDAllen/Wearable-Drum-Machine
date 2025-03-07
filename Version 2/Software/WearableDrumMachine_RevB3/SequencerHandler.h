// SequencerHandler.h

#ifndef SEQUENCER_HANDLER_H
#define SEQUENCER_HANDLER_H

#include "Globals.h" // so that PLAY, trackState[], etc. are known

class SequencerHandler {
public:
    // call every loop to update sequence logic
    void update();

private:
    // These functions replicate your monolith calls:
    void SequenceNotes();
    void REC_Switch();
    void Metronome_Switch();
    void Metronome_PLAY();
    void Opt_Encoder();
    void Undo_Switch();

    void TrackMutedLED();
    void Track_Switch();
    void Play_Switch();

    // If you had more advanced variables or states (like currentTrack, loopLength),
    // you could store them here instead of using the ones in Globals.
};

#endif
