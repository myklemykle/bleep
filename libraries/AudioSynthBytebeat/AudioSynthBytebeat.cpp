#include "Arduino.h"
#include "AudioSynthBytebeat.h"

void AudioPlaySdRaw::update(void)
{
        unsigned int i, n;
        audio_block_t *block;

        // only update if we're playing
        if (!playing) return;

        // allocate the audio blocks to transmit
        block = allocate(); 
        if (block == NULL) return;

        if (rawfile.available()) {
                // we can read more data from the file...
                n = rawfile.read(block->data, AUDIO_BLOCK_SAMPLES*2);
                file_offset += n;
                for (i=n/2; i < AUDIO_BLOCK_SAMPLES; i++) {
                        block->data[i] = 0;
                }
                transmit(block);
        } else {
                rawfile.close();
                playing = false;
        }
        release(block);
}
