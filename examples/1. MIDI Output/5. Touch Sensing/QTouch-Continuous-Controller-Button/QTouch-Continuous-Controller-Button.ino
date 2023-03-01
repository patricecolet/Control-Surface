/** 
 * This is an example that demonstrates the use of MIDI Control Change Buttons.
 *
 * @boards  SAMD21 only
 * 
 * Connections
 * -----------
 * 
 * - A0: QTouch
 * 
 * 
 * Behavior
 * --------
 * 
 * - When Qtouch pressed, a MIDI Control Change message with a 
 *   value of 0x7F (127) is sent for General Purpose Controller #1.
 * - When Qtouch is released, a MIDI Control Change message with a 
 *   value of 0x00 (0) is sent for General Purpose Controller #1
 * 
 * Mapping
 * -------
 * 
 * Select the Arduino as a custom MIDI controller in your DAW, and use the 
 * MIDI learn option to assign the button to a function.
 * 
 * Written by PieterP, 2019-09-29  
 * https://github.com/tttapa/Control-Surface
 */

#include <Control_Surface.h> // Include the Control Surface library

// Instantiate a MIDI over USB interface.
USBMIDI_Interface midi;

// Instantiate a CCButton object
CCQTouchButton button {
  // Push button on pin A0
  A0,
  // General Purpose Controller #1 on MIDI channel 1:
  {MIDI_CC::General_Purpose_Controller_1, CHANNEL_1},
};

void setup() {
  Control_Surface.begin(); // Initialize Control Surface
}

void loop() {
  Control_Surface.loop(); // Update the Control Surface
}