#include "AudioSynthKS.h"

void AudioSynthKS::update(void){

	audio_block_t *block, *excitement;
	int16_t s0, s1, s2, s3;
	int i;
	int magic1;

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
		excitement = receiveReadOnly(0); // could be null?
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

			// Adding a third element creates ring-mod-type stuff, and also brings about some tremolo related to BUFSIZE.
			// That would be a nice effect to control ...
			// TODO: put a knob on BUFSIZE and a knob on magic1
			//s2 = buffer[cursorMinus(magic1)];
			//
			// Also: when this is a fixed value (not relative to buflen) then crazy FM-sounding shit happens ...
			// TODO: an absolute/pitched toggle for these knob vals.
			//s2 = buffer[cursorMinus(128)];

			// average them (comb filter),
			// place the result in the output and in the buffer.
			block->data[i] = buffer[cursor] = ((s0 + s1) / 2); 

			// This ought to do the same thing, but it causes noise and crashes ... wtf?
			//block->data[i] = buffer[cursor] = int((s0 + s1) * 0.5); 

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
		if (++cursor == BUFSIZE) {
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
