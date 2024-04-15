# StandaloneMIDISequencingDrummingGlove

This project is still in development. 

On the hardware side, I'm focused on creating more reliable FSR finger sensors.
Weather resistant (magentic charger?) enclosure design is backlogged.

On the circuitry side, I'm designing a custom PCB that incorporates power management, sensor/usb/programmig ports, and an embedded speaker module. 

The software side is still in my backlog. want to fine-tune / build-out the MIDI looping functionality and create a richer way to sewuence these patterns. variable tempo control is also backlogged.

----

04/01/2024 | Regarding software/code:
Until I've split these up into functional sections, the .ino sketches will build on each other in the following order:
1) FSR Input
2) SMF File Storage
3) MIDI Looper
4) ...
