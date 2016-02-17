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

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform1;      //xy=217,306
AudioEffectEnvelope      envelope1;      //xy=310,369
AudioFilterBiquad        biquad1;        //xy=381,306
AudioMixer4              mixer1;         //xy=529,303
AudioOutputI2S           i2s1;           //xy=712,298
AudioConnection          patchCord1(waveform1, envelope1);
AudioConnection          patchCord2(envelope1, biquad1);
AudioConnection          patchCord3(biquad1, 0, mixer1, 0);
AudioConnection          patchCord4(mixer1, 0, i2s1, 0);
AudioConnection          patchCord5(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=954,312
// GUItool: end automatically generated code


const int ledPin = 13;

//////
// MIDI stuff
/////

//////
// couldn't find a lib to convert midi note number to freq yet ...
// this is taken from mozzi.  performance is probably bad, but 
// midival used to be a float which is worse ...

float mtof(int midival)
{
	 // code from AF_precision_synthesis sketch, copyright 2009, Adrian Freed.
	 float f = 0.0;
	 if(midival) f = 8.1757989156 * pow(2.0, midival/12.0);
	 return f;
}

//////
// How to turn a midi CC integer 0-127 into a number of miliseconds?
// let's have it be exponential between 0 and some higher value.

int ccToMs(byte midival, int maxMs){
  return maxMs * midival * midival / 16129; // 127^2
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
  
  AudioNoInterrupts();
  waveform1.frequency(mtof(note));
// fix a click?
  waveform1.phase(0);
  waveform1.amplitude((float)velocity / 127);
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
  
    Serial.print("Control Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", control=");
  Serial.print(control, DEC);
  Serial.print(", value=");
  Serial.print(value, DEC);
  Serial.println();

   switch(control) {
      case 1: 
      // Mod wheel, which controls different things for different oscillators.
      break;
        
      case 3: // top left in bank 0 (0,0,0) of knobs
        // overall gain
        mixer1.gain(0, (float)value/127);
      break;
      
      case 9: // one to the right (0,1,0)
        // waveform
        switch(value & 3) {
          case 0: 
            waveform1.begin(WAVEFORM_SINE);
          break;
          case 1: 
            waveform1.begin(WAVEFORM_SAWTOOTH);
          break;
          case 2: 
            waveform1.begin(WAVEFORM_SQUARE);
          break;
          case 3: 
            waveform1.begin(WAVEFORM_TRIANGLE);
          break;
        }
      break;
      
      // 16-19: second row (1, 0-3, 0) -- ADSR
      case 16: 
        tempMs = ccToMs(value, 50);
        Serial.print(tempMs, DEC);
        Serial.println();
        envelope1.attack(tempMs);
      break;
      
      case 17: 
        tempMs = ccToMs(value, 50);
        Serial.print(tempMs, DEC);
        Serial.println();
        envelope1.decay(tempMs);
      break;
      
       case 18: 
        envelope1.sustain((float) value / 127);
      break;
      
      case 19: 
        tempMs = ccToMs(value, 50);
        Serial.print(tempMs, DEC);
        Serial.println();
        envelope1.release(tempMs);
      break;
      
         /*      
      case 2: bendRange = map(value, 0, 127, 1, 12); 
      // CC2 sets the global bend range between 1 and 12.
      break;
      
      case 3: 
      // CC3, which controls different things for different oscillators, 
      // type of vibrato on channel 2.
         switch (channel){
            case 1: ch1_Oscillator(7, 0, 0, 0, value);
            break;
            case 2: ch2_Oscillator(7, 0, 0, 0, value);
            break;
            case 3: ch3_Oscillator(7, 0, 0, 0, value);
            break;
            case 4: ch4_Oscillator(7, 0, 0, 0, value);
            break;
            case 6: mainWaveformEngine(7, 0, 0, 0, value);
            break;
         }
      break;
   
      case 4: // CC4, which turns on the Arpeggiator for that channel.
         switch (channel){
            case 1: ch1_Arpeggiator(4, value);
            break;
            case 2: ch2_Arpeggiator(4, value);
            break;
            case 3: ch3_Arpeggiator(4, value);
            break;
            case 4: ch4_Arpeggiator(4, value);
            break;
            case 6: waveform_Arpeggiator(4, value);
            break;
         } 
      break;

      case 5: // CC5, which controls the Arpeggiator Speed.
         switch (channel){
            case 1: ch1_Arpeggiator(10, value);
            break;
            case 2: ch2_Arpeggiator(10, value);
            break;
            case 3: ch3_Arpeggiator(10, value);
            break;
            case 4: ch4_Arpeggiator(10, value);
            break;
            case 6: waveform_Arpeggiator(10, value);
            break;
         }
      break;

      case 6: 
      // CC6, which controls the polypulse on channel 2, 
      // and PWM Follower on channel 1.
         switch (channel){
            case 1: ch1_Oscillator(9, 0, 0, 0, value);
            break;
            case 2: ch2_Oscillator(9, 0, 0, 0, value);
            break;
            case 3: ch3_Oscillator(9, 0, 0, 0, value);
            break;
            case 4: ch4_Oscillator(9, 0, 0, 0, value);
            break;
            case 6: mainWaveformEngine(9, 0, 0, 0, value);
            break;
         }
      break;

      case 20: //CC20, which controls the portamento speed.
         switch (channel){
            case 1: ch1_Oscillator(12, 0, 0, 0, value);
            break;
            case 2: ch2_Oscillator(12, 0, 0, 0, value);
            break;
            case 3: ch3_Oscillator(12, 0, 0, 0, value);
            break;
            case 4: ch4_Oscillator(12, 0, 0, 0, value);
            break;
            case 6: mainWaveformEngine(12, 0, 0, 0, value);
            break;
         }
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
  Serial.print('hi');

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

	// setup mixer:
	mixer1.gain(0,1);

	// start oscilator
	waveform1.amplitude(0);
	waveform1.frequency(440); // not that it matters yet?
	waveform1.begin(WAVEFORM_SINE); // can this be called twice?

// beep?
	waveform1.amplitude(0.5);
        envelope1.noteOn();
	delay(200);
	waveform1.amplitude(0);

  Serial.print(' there.');
  Serial.println();
}


void loop() {

// Look for a MIDI message off of the USB, and execute callback
// functions, handled to OnNoteOn, OnNoteOff, and OnControlChange

	 usbMIDI.read();

// and I guess the waveforms all get handled by interrupts someplace...
	

}

