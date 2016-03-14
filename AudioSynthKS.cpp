#include "AudioSynthKS.h"

void AudioSynthKS::update(void){

	audio_block_t *block, *excitement;
	int16_t s0, s1; 
	int i;

	if (!running) return;

	block = allocate(); 
	if (block == NULL) return;

	// If buflen has been reduced by a wavelength change & cursor is now outside of bounds, reposition cursor.
	while (cursor >= buflen) {
		cursor -= buflen;
	}

	if (triggering) {
		// We've just been asked to excite our buffer 
		triggering = false;
		triggered = true;
		triggerPoint = cursor;
		Serial.print("triggerPoint: ");
		Serial.println(triggerPoint, DEC);
	}

	if (triggered) { //DEBUG
		// read a block of input excitement (noise, or whatever)
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
			s0 = buffer[cursor];
			s1 = buffer[cursorMinus(1)];

			// average them (comb filter),
			// place the result in the output and in the buffer.
			block->data[i] = buffer[cursor] = ((s0 + s1) / 2);
		}

		// increment cursor
		if (++cursor == buflen) {
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
