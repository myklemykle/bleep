#include "AudioSynthKS.h"

void AudioSynthKS::update(void){

	audio_block_t *block, *excitement;
	int16_t s0, s1; 
	int i;

	if (!running) return;

	block = allocate(); 
	if (block == NULL) return;

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
	} else {  // DEBUG
		excitement = NULL;
	}

	for (i=0; i < AUDIO_BLOCK_SAMPLES; i++) {
		//if (triggered) {
		if (excitement != NULL) {
			block->data[i] = buffer[cursorPlus(i)] = excitement->data[i];
			//block->data[i] = excitement->data[i];
		} else {
			// K/S algorithm at its simplest:
			// load the cursor sample & the one just behind it
			s0 = buffer[cursorPlus(i)];
			s1 = buffer[i == 0 ? cursorMinus(1) : cursorPlus(i-1)];
			//
			// average them (comb filter),
			// place the result in the output and in the buffer.
			block->data[i] = buffer[cursorPlus(i)] = ((s0 + s1) / 2);
		}

		incCursor();
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
