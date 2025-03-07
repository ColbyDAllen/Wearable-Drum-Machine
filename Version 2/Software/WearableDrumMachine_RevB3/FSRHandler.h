// FSRHandler.h

#ifndef FSR_HANDLER_H
#define FSR_HANDLER_H

#include "Globals.h"
#include <ResponsiveAnalogRead.h>



class FSRHandler {
public:
    // Call this once at startup to set up ResponsiveAnalogRead
    void initFSRs();

    // Call this repeatedly in loop() to check FSR thresholds,
    // handle velocity timing, etc. — exactly like the monolith’s loop logic
    void updateFSRs();

private:
    // The monolith had separate functions for note on/off or aftertouch.
    // We can keep them private here, or you can replicate the exact function names from your monolith.
    void NoteOnSend(int fsrIndex);
    void PolyTouchSend(int fsrIndex);
    void NoteOffSend(int fsrIndex); // if you also had an explicit note-off function

    // If you have any per-FSR state or counters, you can store them here
    // instead of using global variables. Or you can still rely on Globals.
};

#endif // FSR_HANDLER_H