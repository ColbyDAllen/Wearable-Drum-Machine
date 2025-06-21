# Wearable Drum Machine 
# StandaloneMIDISequencingDrummingGlove

12/15/2024 |

This project is mostly complete. Until I can find better a more suitable input/sensory solution, please consider this work on hiatus.

Further information can be found at www.colbydanielallen.com.

4/15/2024 |
This project is still in development. 

On the hardware side, I'm focused on creating more reliable FSR finger sensors.
Weather resistant (magentic charger?) enclosure design is backlogged.

On the circuitry side, I'm designing a custom PCB that incorporates power management, sensor/usb/programmig ports, and an embedded speaker module. 

On the software side there's a number of backlogged tasks. I want to fine-tune / build-out the MIDI looping functionality and create a richer way to sequence these patterns. Variable tempo control in standalone operation (via rotary encoder) is also backlogged.

----

04/01/2024 | Regarding software/code:
Until I've split these up into functional sections, the .ino sketches will build on each other in the following order:
1) FSR Input
2) SMF File Storage
3) MIDI Looper
4) ...
