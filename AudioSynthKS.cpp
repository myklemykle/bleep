#include "AudioSynthKS.h"

void AudioSynthKS::update(void){

	audio_block_t *block, *excitement;
	int32_t s0, s1, s2, s3;
	int32_t acc;
	int i;

	if (!running) return;

	block = allocate(); 
	if (block == NULL) return;

	// If buflen has been reduced by a wavelength change & cursor is now outside of bounds, reposition cursor.
//	while (cursor >= buflen) {
//		cursor -= buflen;
//	}

	if (triggering) {
		// We've just been asked to excite our buffer 
		triggering = false;
		triggered = true;
		triggerPoint = cursorPlus(buflen);
		Serial.print("triggerPoint: ");
		Serial.println(triggerPoint, DEC);
	}

	if (triggered) { //DEBUG
		// read a block of input excitement (noise, or whatever you got)
		excitement = receiveReadOnly(_exciter); // could be null?
		if (excitement == NULL) {
			Serial.println("z"); // DEBUG
			release(block);
			return; //DEBUG
		}
	}

	for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		if (triggered) {
			block->data[i] = buffer[cursor] = excitement->data[i];
		} else {
			// K/S algorithm at its simplest:
			// load the cursor sample & the one just behind it
			// (In our travelling-buffer implementation, cursor-buflen == cursor)
			s0 = buffer[cursorMinus(1)];
			s1 = buffer[cursorMinus(buflen)]; 

			// TODO: put a knob on this bug:
			// Overshooting the beginning of the buffer throws tiny specs of random bugnoise into the tube ... which ends up sounding kind of nice
			// Seems like values past 32 become insipid ... adjust exponentially.
			//s1 = buffer[cursorMinus(buflen + 1)];

			// Adding a third element creates ring-mod-type stuff, and also brings about some tremolo related to _bufmax.
			// That would be a nice effect to control ...
			s2 = buffer[cursorMinus(_magic1)];
			//
			// Also: when this is a fixed value (not relative to buflen) then crazy FM-sounding shit happens ...
			// TODO: an absolute/pitched toggle for these knob vals.
			//s2 = buffer[cursorMinus(128)];

			// average them (comb filter),
			// then mix them back into the buffer (using decayBalance),
			// and put that in the output.
			// block->data[i] = buffer[cursor] = ((s0 + s1) / 2); 
			// "drum variant" according to K and S? 
			acc = ((s0 + s1) / 2) ;
			if (random(0,128) >= _drumness) // flip a coin
				acc = 0 - acc; // negate the samplE
			block->data[i] = buffer[cursor] = acc;

			/* workingish but parked for now?
			block->data[i] = buffer[cursor] = (int16_t)
				// how much of the original sample (s0) to mix?
				(((65535 - _decayBalance) * s0)>>16)  // will be zero when decayBalance = 0xffff
					+ 
				// how much of the new value to mix?
				((_decayBalance * ((s0 + s1) / 2) )>>16);  // will be zero when decayBalance = 0;
				*/

			// This ought to do the same thing, but it causes noise and crashes ... wtf?
			//block->data[i] = buffer[cursor] = (int16_t)((s0 + s1) * 0.5); 

			// Other things we could do instead:
			// If we subtract instead of adding, the whole thing drops an octave & also sounds slightly more lowpass filtered ... 
			// I believe the octave thing is because the entire waveform is inverted on every pass, so it becomes a half-wave.
			// The filter thing may be math rounding issues or similar ... otherwise i don't know.  At any rate, this does sound nice.
			// TODO: my kingdom for a 'scope ...
			// block->data[i] = buffer[cursor] = ((s0 - s1) / 2); 
			
			// Here's the same thing redrawn in the more 'general' form of a digital comb filter: 
			// (feedbackGain * buffer[cursor - feedbackDistance]) + (feedforwardGain * buffer[cursor + feedforwardDistance])
			// (with feedforwardDistance = 0, feedbackDistance = 1, and both gains at 0.5.
			// block->data[i] = buffer[cursor] = int( (0.5 * buffer[cursorMinus(1)]) + (0.5 * buffer[cursorMinus(buflen + 0)]));
			// much obliged: https://www.dsprelated.com/freebooks/filters/Analysis_Digital_Comb_Filter.html

			// three element version:
			//block->data[i] = buffer[cursor] = (int16_t)((s0 + s1 + s2) / 3); 
			//block->data[i] = buffer[cursor] = ((s0 + s1 + s2) / 3); 

			// four element version:
			//s3 = buffer[cursorMinus(23)];
			//block->data[i] = buffer[cursor] = ((s0 + s1 + s2 + s3) / 4); 
		}

		// increment cursor
		if (++cursor == _bufmax) {
			cursor = 0;
		}
		if (cursor == triggerPoint) {
			triggered = false;
		}
	}

	if (excitement != NULL)  {
		release(excitement);
	}
	transmit(block);
	release(block);
}
