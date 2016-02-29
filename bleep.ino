/*

	 the plan:

	 main loop polls midi
	interrupts manage audio?

	 listen on all channels for now.

	 noteOn handler ... we'll be monophonic for now & let every note-on
	 set the pitch & start the sound if it's not already going.
	 
	 noteOff handler stops the sound.

	 controlChange handler:
	 	we'll see what the knobs send, but probably General Purpose.
		also the pitch & bend will send pitch & bend.
		Each voice will need a handler for some of that stuff,
		so the overall handler should sift out global stuff & then
		punt to a voice handler.

		(this teensy-tone code I'm copping dealt with changing voices by just assigning
		one MIDI channel to each voice.  there's probably a change-voice message to use
		instead, but i'll get to that when I have more than one voice.)

		0) beep to test audio connections.
	 	1) first voice will be a simple oscilator as a test.
		2) then i will start hooking knobs to things, learning about midi CC etc.

*/

// teensy audio layout: wave -> filter -> mixer -> i2s

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "AudioSynthBytebeat.h"

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthBytebeat       bytebeat1;         //xy=64,90
AudioSynthWaveform       waveform1;      //xy=73,47
AudioMixer4              mixer2;         //xy=236,54
AudioEffectEnvelope      envelope1;      //xy=390,51
AudioFilterBiquad        biquad1;        //xy=574,45
AudioOutputI2S           i2s1;           //xy=689,153
AudioMixer4              mixer1;         //xy=712,33
AudioConnection          patchCord1(bytebeat1, 0, mixer2, 1);
AudioConnection          patchCord2(waveform1, 0, mixer2, 0);
AudioConnection          patchCord3(mixer2, envelope1);
AudioConnection          patchCord4(envelope1, biquad1);
AudioConnection          patchCord5(biquad1, 0, mixer1, 0);
AudioConnection          patchCord6(mixer1, 0, i2s1, 0);
AudioConnection          patchCord7(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=878,163
// GUItool: end automatically generated code


static int maxEnvelopeTime = 50; // ms
static float noteAmpl = 0;
static uint8_t waveType = 0;
const int ledPin = 13;

//////
// MIDI stuff
/////

//////
// couldn't find a lib to convert midi note number to freq yet ...
// this is taken from mozzi.  performance is probably bad, but 
// midival used to be a float which is worse ...

float ccToFreq(int midival)
{
	 // code from AF_precision_synthesis sketch, copyright 2009, Adrian Freed.
	 float f = 0.0;
	 if(midival) f = 8.1757989156 * pow(2.0, midival/12.0);
	 return f;
}

//////
// How to turn a midi CC integer 0-127 into a arbitrarily large number of miliseconds?
// let's have it be exponential between 0 and some higest value.

int ccToMs(byte midival, int maxMs){
  return maxMs * midival * midival / 16129; // 127^2
}

#define ccToUnit(value) ((float)value/127);
///////
// Update the filter
//
static uint8_t filterType = 0;
float filterFreq = 440;
float filterQ = 1;

void updateFilter(void){
	switch (filterType) {
		case 0: 
			// how to disable?
		case 1: 
			biquad1.setLowpass(0, filterFreq, filterQ);
			break;
		case 2: 
			biquad1.setHighpass(0, filterFreq, filterQ);
			break;
		case 3: 
			biquad1.setBandpass(0, filterFreq, filterQ);
			break;
		case 4: 
			biquad1.setNotch(0, filterFreq, filterQ);
			break;
	}
}

void OnNoteOn (byte channel, byte note, byte velocity) {
	digitalWrite(ledPin, HIGH);   // set the LED on
	
	Serial.print("Note On, ch=");
	Serial.print(channel, DEC);
	Serial.print(", note=");
	Serial.print(note, DEC);
	Serial.print(", velocity=");
	Serial.print(velocity, DEC);
	Serial.println();
	
	noteAmpl = (float)velocity / 127;

	AudioNoInterrupts();
	if (waveType == 4) { 
		bytebeat1.time(0);
	} else { 
		waveform1.frequency(ccToFreq(note));
		waveform1.amplitude(noteAmpl);
	}
	mixer2.gain(1,noteAmpl);
	envelope1.noteOn();
	AudioInterrupts();
}

void OnNoteOff (byte channel, byte note, byte velocity) {
	digitalWrite(ledPin, LOW);   // set the LED on
	//waveform1.amplitude(0);
	envelope1.noteOff();
}

void OnControlChange(byte channel, byte control, byte value) {
	int tempMs;

// TODO:
// Standard MIDI ids where possible (gain, ADSR)
// Use program change insted of knob for ... changing program!
// Work out how to use relative/continuous controllers instead of absolute?


	 switch(control) {
			case 1: 
				bytebeat1.setRecipe(value);
				Serial.println("bytebeat change");
			break;
				
			case 7: // channel volume
				mixer1.gain(0, (float)value/127);
				Serial.print("gain: ");
				Serial.println((float)value/127);
			break;
			
			case 114: // waveform / preset / bank select
				waveType = value % 5;
				Serial.print("waveform: ");
				switch(waveType) {
					case 0: 
						waveform1.begin(WAVEFORM_SINE);
						Serial.println("sine");
					break;
					case 1: 
						waveform1.begin(WAVEFORM_SAWTOOTH);
						Serial.println("sawtooth");
					break;
					case 2: 
						waveform1.begin(WAVEFORM_SQUARE);
						Serial.println("square");
					break;
					case 3: 
						waveform1.begin(WAVEFORM_TRIANGLE);
						Serial.println("triangler");
					break;
					case 4: 
						Serial.println("bytebeat");
					break;
				}

				// switch between generators:
				switch(waveType) {
					case 0:
					case 1:
					case 2:
					case 3:
						AudioNoInterrupts();
						waveform1.amplitude(1); // control waveform1 by amplitude()
						bytebeat1.stop();
						mixer2.gain(1,0); // control bytebeat by mixer gain
						mixer2.gain(0,noteAmpl); // control bytebeat by mixer gain
						AudioInterrupts();
					break;
					case 4:
						AudioNoInterrupts();
						waveform1.amplitude(0);
						bytebeat1.time(0);
						bytebeat1.start();
						mixer2.gain(0,0); 
						mixer2.gain(1,noteAmpl); 
						AudioInterrupts();
					break;
				}
			break;
			
			//case 18: // param 1 
			//	bytebeat1.setRecipe(value);
			 // Serial.println("bytebeat change");
			//break;
			// filter stuff:
			case 74: // filter type
				filterType = value % 5;
				switch (filterType) { 
					case 1: 
						Serial.println("lowpass");
						break;
					case 2:
						Serial.println("highpass");
						break;
					case 3:
						Serial.println("bandpass");
						break;
					case 4:
						Serial.println("notch");
						break;
				}
				updateFilter();
				break;

			case 71: // filter freq
				Serial.print("filter freq: ");
				filterFreq = ccToFreq(value);
				Serial.println(filterFreq, DEC);
				updateFilter();
				break;

			case 76: // filter q
				Serial.print("filter q: ");
				//filterQ = ccToUnit(value);
				Serial.println(filterQ, DEC);
				updateFilter();
				break;
			
			case 91: // something about time ... 
				// max envelope time (multiplier for AD&R) -- from 100 to 12800
				maxEnvelopeTime = (value + 1) * 100;
				Serial.print("max envelope time: ");
				Serial.println(maxEnvelopeTime, DEC);
			break;
			
			case 73: // attack
				tempMs = ccToMs(value, maxEnvelopeTime);
				Serial.print("attack: ");
				Serial.print(tempMs, DEC);
				Serial.println(" ms");
				envelope1.attack(tempMs);
			break;
			
			case 75:  // decay
				tempMs = ccToMs(value, maxEnvelopeTime);
				Serial.print("decay: ");
				Serial.print(tempMs, DEC);
				Serial.println(" ms");
				envelope1.decay(tempMs);
			break;
			
			 case 79:  // sustain
			 Serial.print("sustain: ");
			 Serial.print((float) value/127);
			 Serial.println();
			 envelope1.sustain((float) value / 127);
			break;
			
			case 72:  // release
				tempMs = ccToMs(value, maxEnvelopeTime);
				Serial.print("release: ");
				Serial.print(tempMs, DEC);
				Serial.println(" ms");
				envelope1.release(tempMs);
			break;
			
			default: 
				Serial.print("Control Change, ch=");
				Serial.print(channel, DEC);
				Serial.print(", control=");
				Serial.print(control, DEC);
				Serial.print(", value=");
				Serial.print(value, DEC);
				Serial.println();
			break; 
				 /*      
			case 2: bendRange = map(value, 0, 127, 1, 12); 
			// CC2 sets the global bend range between 1 and 12.
			break;
	 
			case 4: // CC4, which turns on the Arpeggiator for that channel.
			break;

			case 5: // CC5, which controls the Arpeggiator Speed.
			break;

			case 6: 
			// CC6, which controls the polypulse on channel 2, 
			// and PWM Follower on channel 1.
			break;

			case 20: //CC20, which controls the portamento speed.
			break;
	 */
	 }

}

//////////////////////////////////////////
//  SETUP AND LOOP
//////////////////////////////////////////

void setup() {
	
		// initialize the digital pin as an output.
	pinMode(ledPin, OUTPUT);
	
	Serial.begin(115200);
	Serial.print("hi");

	AudioMemory(12); // must actually check this later, hmm ...

	sgtl5000_1.enable(); 
				sgtl5000_1.volume(0.6);
	// midi handlers:
	usbMIDI.setHandleNoteOff(OnNoteOff);
	usbMIDI.setHandleNoteOn(OnNoteOn) ;
	usbMIDI.setHandleControlChange(OnControlChange) ;
	//usbMIDI.setHandlePitchChange(OnPitchChange);

	// setup filter:
	// Butterworth filter, 12 db/octave
	biquad1.setLowpass(0, 800, 0.707);

	// setup mixers:
	mixer1.gain(0,1);
	mixer2.gain(0,1); // bring up waveform
	mixer2.gain(1,0); // take down bytebeat

	// start oscilator
	waveform1.amplitude(0);
	waveform1.frequency(440); // not that it matters yet?
	//waveform1.set_ramp_length(10); // ten sample ramp in/out
	waveform1.begin(WAVEFORM_SINE); // can this be called twice?

// beep?
	waveform1.amplitude(0.5);
				envelope1.noteOn();
	delay(100);
	waveform1.amplitude(0);

	Serial.println(" there.");
}


void loop() {

// Look for a MIDI message off of the USB, and execute callback
// functions, handled to OnNoteOn, OnNoteOff, and OnControlChange

	 usbMIDI.read();

// and I guess the waveforms all get handled by interrupts someplace...
	

}

