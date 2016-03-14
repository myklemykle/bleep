#include "AudioSynthBytebeat.h"

void AudioSynthBytebeat::update(void){

	uint8_t i;
	uint8_t v;

	audio_block_t *block;

	if (! running) return;

	// allocate the audio block to transmit
	block = allocate(); 
	if (block == NULL) return;

	for (i=0; i < AUDIO_BLOCK_SAMPLES; i += 4) {
		switch (recipe) {
			case 0:
				v = ((t >> 10) & 42) * t;
				break;
			case 1:
				v = t*(t>>11&t>>8 & 0b01100011 &t>>3);  // 0b01100011 is a little less hectic than 123
				break;
			case 2:
				v = (t*5&(t>>7)) | (t*3&(t>>8)); /// t*4>>10 and t>>8 should be the same thing ...
				break;
			case 3:
				v = (t>>7|t|t>>6)*10 + ((t*t>>13|t>>6)<<2 ); // x << 2 and x * 4 should be the same thing ...
				break;
			case 4:
				v = ( ((t*("36364689"[t>>11&7]&15))/12&128)  + (( (((t>>9)^(t>>9)-2)%11*t) >>2|t>>13)&127) ) << 2; // 8khz version
				break;
			case 5:
				v = ((t<<2^t>>6)*2 + (t<<3^t>>7)) * ("11121114"[(t>>15)&7]-48);
				break;
			case 6:
				v = ((t<<1)^((t<<1)+(t>>7)&t>>12))|t>>(4-(1&(t>>19)))|t>>7;
				break;
			case 7:
				v = t*6&((t>>8|t<<4)) ^ t*4&((t>>7|t<<3)) ^ t*2&((t>>6|t<<2));
			  break;
			case 8:
				v = t*3 & (t/12>>8|t%3)%24 | (t>>8)&19  | (t>>8)&19 >> (t&((t>>1)>>11)%63) >> (t*3&(t>>7&t + t<<11));
			break;
		}
		t++;
		block->data[i] = v<<8;
		block->data[i+1] = v<<8;
		block->data[i+2] = v<<8;
		block->data[i+3] = v<<8;
	}

	transmit(block);
	release(block);
}
