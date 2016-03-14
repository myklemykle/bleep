
#include <Audio.h>
#include <Wire.h>
#include "AudioSynthBytebeat.h"
#include "AudioSynthKS.h"

// GUItool: begin automatically generated code
//AudioSynthNoisePink      pink1;          //xy=110,287  // if there's a difference for KS between pink & white noise, it's pretty subtle ...
AudioSynthNoiseWhite      pink1;          //xy=110,287  
AudioSynthKS 				     ks1; 
AudioSynthBytebeat       bytebeat1; 
AudioSynthWaveform       waveform1;      //xy=73,47
AudioMixer4              mixer2;         //xy=236,54
AudioEffectEnvelope      envelope1;      //xy=390,51
AudioFilterBiquad        biquad1;        //xy=574,45
AudioOutputI2S           i2s1;           //xy=689,153
AudioMixer4              mixer1;         //xy=712,33
AudioConnection          patchCord2(waveform1, 0, mixer2, 0);
AudioConnection          patchCord3(bytebeat1, 0, mixer2, 1);
AudioConnection          patchCord1(pink1, 0, ks1, 0);
AudioConnection          patchCord4(ks1, 0, mixer2, 2);
AudioConnection          patchCord5(mixer2, envelope1);
AudioConnection          patchCord6(envelope1, biquad1);
AudioConnection          patchCord7(biquad1, 0, mixer1, 0);
AudioConnection          patchCord8(mixer1, 0, i2s1, 0);
AudioConnection          patchCord9(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=878,163
// GUItool: end automatically generated code


static int maxEnvelopeTime = 50; // ms
static float noteAmpl = 0;
static uint8_t waveType = 5;
const int waveformTypes[4] = { WAVEFORM_SINE, WAVEFORM_SAWTOOTH, WAVEFORM_SQUARE, WAVEFORM_TRIANGLE};
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

//////
// In general, if we want a CC integer to map exponetially to a range:
float ccToRangeExp(uint8_t midival, float rangeMin, float rangeMax) {
	return rangeMin + ( (rangeMax-rangeMin) * midival * ((float)midival / 16129) ) ;
}

#define ccToUnit(value) ((float)value/127)
#define ccToQ(value) ccToRangeExp(value, 0.707, 40) // Q from +0.707 to +40

///////
// Update the filter
//
static uint8_t filterType = 0;
float filterFreq = 440;
float filterQ = 0.707;

void updateFilter(void){
	switch (filterType) {
		case 0: 
			// how to disable?
			biquad1.setLowpass(0, 50000, filterQ);
			break;
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
	Serial.println(noteAmpl, DEC);

	AudioNoInterrupts();
	if (waveType == 4) { 
		// bb mode
		bytebeat1.time(0);
		mixer2.gain(1,noteAmpl); 
		bytebeat1.start();
	} else if (waveType == 5) { 
		// ks mode
		//mixer2.gain(2,noteAmpl);
		mixer2.gain(2,1); // TESTING ...
		ks1.wavelength(AUDIO_SAMPLE_RATE / ccToFreq(note));
		ks1.trigger();
	} else { 
		// wave mode
		waveform1.frequency(ccToFreq(note));
		waveform1.amplitude(noteAmpl);
		mixer2.gain(0,1);
	}
	envelope1.noteOn();
	AudioInterrupts();
}

void OnNoteOff (byte channel, byte note, byte velocity) {
	Serial.print("Note Off, ch=");
	Serial.print(channel, DEC);
	Serial.print(", note=");
	Serial.print(note, DEC);
	Serial.print(", velocity=");
	Serial.print(velocity, DEC);
	Serial.println();
	
	digitalWrite(ledPin, LOW);   // set the LED off
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
			mixer1.gain(2, (float)value/127); // DEBUG
			Serial.print("gain: ");
			Serial.println((float)value/127);
		break;
		
		case 114: // waveform / preset / bank select
			waveType = (value % 6);
			Serial.print("waveform: ");
			switch(waveType) {
				case 0: 
					Serial.println("sine");
				break;
				case 1: 
					Serial.println("sawtooth");
				break;
				case 2: 
					Serial.println("square");
				break;
				case 3: 
					Serial.println("triangler");
				break;
				case 4: 
					Serial.println("bytebeat");
				break;
				case 5: 
					Serial.println("Karpluss-Strong");
				break;
			}

			AudioNoInterrupts();
			// switch between generators:
			switch(waveType) {
				case 0:
				case 1:
				case 2:
				case 3:
					waveform1.begin(waveformTypes[waveType]);
					waveform1.amplitude(noteAmpl); // control waveform1 gain by amplitude()
					mixer2.gain(0,1); // gain is already adjusted, mix at unity

					bytebeat1.stop();
					ks1.stop();
					mixer2.gain(1,0); 			
					mixer2.gain(2,0); 

				break;
				case 4:
					mixer2.gain(1,noteAmpl); // control bb gain with mixer
					
					waveform1.amplitude(0);
					ks1.stop();
					mixer2.gain(0,0); 
					mixer2.gain(2,0); 

				break;
				case 5:
					mixer2.gain(2,noteAmpl); // control ks gain with mixer

					bytebeat1.stop();
					waveform1.amplitude(0);
					mixer2.gain(0,0); 
					mixer2.gain(1,0); 

				break;
			}
			AudioInterrupts();

		break;
		
		case 18: // param 1 
			bytebeat1.setRecipe(value);
			Serial.println("bytebeat change");
		break;

		// filter stuff:
		case 74: // filter type
			filterType = value % 5;
			switch (filterType) { 
				case 0: 
					Serial.println("none");  // actually a lowpass with center freq at 50khz.
					break;
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
			filterQ = ccToQ(value);
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
			Serial.print("Max CPU so far:");
			Serial.println(AudioProcessorUsageMax(), DEC);
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

	AudioMemory(24); // must actually check this later, hmm ...

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
	mixer1.gain(2,1);

	// TODO: send simulated setup messages to configure current settings?

	// start oscilator
	waveform1.amplitude(0);
	waveform1.frequency(440); // not that it matters yet?
	//waveform1.set_ramp_length(10); // ten sample ramp in/out
	waveform1.begin(WAVEFORM_SINE); // can this be called twice?

	pink1.amplitude(1.0);
	ks1.clear();

// beep at startup:
	waveform1.amplitude(0.5);
	envelope1.noteOn();
	delay(100);
	waveform1.amplitude(0);

	Serial.print("hi");
	Serial.println(" there.");
}


void loop() {

// Look for a MIDI message off of the USB, and execute callback
// functions, handled to OnNoteOn, OnNoteOff, and OnControlChange

	 usbMIDI.read();

// and I guess the waveforms all get handled by interrupts someplace...
	

}

