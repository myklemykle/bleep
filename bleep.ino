
#include <Audio.h>
#include <Wire.h>
#include "AudioSynthBytebeat.h"
#include "AudioSynthKS.h"

// GUItool: begin automatically generated code
AudioSynthNoisePink      pink1;          //xy=110,287   // pink noise gives a bit less aggressive bite to the notes ... 
																												// less high freq. presence in general i guess.
//AudioSynthNoiseWhite      pink1;          //xy=110,287  
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
//AudioConnection          patchCord6(envelope1, biquad1);
//AudioConnection          patchCord7(biquad1, 0, mixer1, 0);
AudioConnection          patchCord6(envelope1, 0, mixer1, 0);
AudioConnection          patchCord8(mixer1, 0, i2s1, 0);
AudioConnection          patchCord9(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=878,163
// GUItool: end automatically generated code

////////////////////////////
// Parameters:
// NOTE: this is screaming for a typedef instead of a bunch of parallel arrays.
// NOTE: for all of those arrays, we are counting on the compiler to initialize the values to NULL/0.
////////////////////////////

// Max # of parameters
#define MAXPARAMETERS 256

// type of raw parameters: -128 to 127
typedef int8_t Parameter;

// Parameter memory:
static Parameter params[MAXPARAMETERS];

// Paramter update functions;
typedef void (*ParamUpdater)();
ParamUpdater paramUpdaters[MAXPARAMETERS];

// Midi controller -> parameter map:
uint8_t midi2param[MAXPARAMETERS];

// Indices of paramters (arbitrary, defined here only):
//http://stackoverflow.com/questions/10091825/constant-pointer-vs-pointer-on-a-constant-value
const unsigned int waveType 				= 1;
const unsigned int noteAmpl 				= 2;
const unsigned int bytebeatRecipe  = 3;
const unsigned int mixer1gain0     = 4;
const unsigned int maxEnvelopeTime = 5;

// Definitions of parameters:
#define MAXENVELOPETIME (params[maxEnvelopeTime] + 1) * 100
#define WAVETYPE (params[waveType] % 6)
#define NOTEAMPL (float)params[noteAmpl] / 127
#define BYTEBEATRECIPE params[bytebeatRecipe]
#define MIXER1GAIN0 (float)params[mixer1gain0] / 127

// Setting a parameter:
void setParam(int pIndex, Parameter val){
	Serial.print("setting parameter ");
	Serial.print(pIndex, DEC);
	Serial.print(" to ");
	Serial.println(val, DEC);
	params[pIndex] = val;
	if (paramUpdaters[pIndex] != NULL) {
		paramUpdaters[pIndex]();
	}
}

// Setting a param from a midi controller message, thru the map:
void setParamFromMidi(int controller, Parameter val){
	// midi2param is a one->many map so we have to check it all, ugh.
	int i;

	for (i=0; i<MAXPARAMETERS; i++){
		if(midi2param[i] == controller) {
			setParam(i, val);
		}
	}
}

// Initialization of parameters:
void initParams(){
	// Parameter updaters:
	paramUpdaters[bytebeatRecipe] = [](){ bytebeat1.setRecipe(BYTEBEATRECIPE); };
	paramUpdaters[mixer1gain0] = [](){ mixer1.gain(0, MIXER1GAIN0); };

	// midi->parameter mapping:
	midi2param[bytebeatRecipe] = 18;

	// initialize parameters & run updates:
	setParam(maxEnvelopeTime, 10);
	setParam(waveType, 5);
	setParam(noteAmpl, 0);
	setParam(mixer1gain0, 127);
}


//////////////////////
//// Constants:
//////////////////////
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
	
	setParam(noteAmpl, velocity);
	Serial.println(NOTEAMPL, DEC);

	AudioNoInterrupts();
	if (WAVETYPE == 4) { 
		// bb mode
		bytebeat1.time(0);
		mixer2.gain(1,NOTEAMPL); 
		bytebeat1.start();
	} else if (WAVETYPE == 5) { 
		// ks mode
		//mixer2.gain(2,NOTEAMPL);
		mixer2.gain(2,1); // TESTING ...
		ks1.wavelength(AUDIO_SAMPLE_RATE / ccToFreq(note));
		ks1.trigger();
	} else { 
		// wave mode
		waveform1.frequency(ccToFreq(note));
		waveform1.amplitude(NOTEAMPL);
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
			
		case 7: // channel volume
			setParam(mixer1gain0, value);
			Serial.print("gain: ");
			Serial.println(MIXER1GAIN0);
		break;
		
		case 114: // waveform / preset / bank select
			//waveType = (value % 6);
			setParam(waveType, value);
			Serial.print("waveform: ");
			switch(WAVETYPE) {
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
			switch(WAVETYPE) {
				case 0:
				case 1:
				case 2:
				case 3:
					waveform1.begin(waveformTypes[WAVETYPE]);
					waveform1.amplitude(NOTEAMPL); // control waveform1 gain by amplitude()
					mixer2.gain(0,1); // gain is already adjusted, mix at unity

					bytebeat1.stop();
					ks1.stop();
					mixer2.gain(1,0); 			
					mixer2.gain(2,0); 

				break;
				case 4:
					mixer2.gain(1,NOTEAMPL); // control bb gain with mixer
					
					waveform1.amplitude(0);
					ks1.stop();
					mixer2.gain(0,0); 
					mixer2.gain(2,0); 

				break;
				case 5:
					mixer2.gain(2,NOTEAMPL); // control ks gain with mixer

					bytebeat1.stop();
					waveform1.amplitude(0);
					mixer2.gain(0,0); 
					mixer2.gain(1,0); 

				break;
			}
			AudioInterrupts();

		break;
		
		case 18: // param 1 
			setParamFromMidi(18, value);
			Serial.println("bytebeat change (18)");
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
			//MAXENVELOPETIME = (value + 1) * 100;
			setParam(maxEnvelopeTime, value);
			Serial.print("max envelope time: ");
			Serial.println(MAXENVELOPETIME, DEC);
		break;
		
		case 73: // attack
			tempMs = ccToMs(value, MAXENVELOPETIME);
			Serial.print("attack: ");
			Serial.print(tempMs, DEC);
			Serial.println(" ms");
			envelope1.attack(tempMs);
		break;
		
		case 75:  // decay
			tempMs = ccToMs(value, MAXENVELOPETIME);
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
			tempMs = ccToMs(value, MAXENVELOPETIME);
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

	initParams();
	
		// initialize the digital pin as an output.
	pinMode(ledPin, OUTPUT);
	
	Serial.begin(115200);

	AudioMemory(24); // must actually check this later, hmm ...

	sgtl5000_1.enable(); 
	//sgtl5000_1.volume(0.6);
	sgtl5000_1.volume(0.7);

	// midi handlers:
	usbMIDI.setHandleNoteOff(OnNoteOff);
	usbMIDI.setHandleNoteOn(OnNoteOn) ;
	usbMIDI.setHandleControlChange(OnControlChange) ;
	//usbMIDI.setHandlePitchChange(OnPitchChange);

	// setup filter:
	// Butterworth filter, 12 db/octave
	biquad1.setLowpass(0, 800, 0.707);

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

