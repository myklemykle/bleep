#include <AudioStream.h>

#define BUFSIZE 128*32 // probably too large ... TODO calculate the max size, for the lowest audible freq.

#define cursorMinus(number) (number > cursor) ? cursor + buflen - number : cursor - number
#define cursorPlus(number) (number + cursor >= buflen) ? cursor + number - buflen : cursor + number
#define incCursor() if (++cursor >= buflen) { cursor = 0; }

class AudioSynthKS : public AudioStream
{
public:
        AudioSynthKS() : AudioStream(1, inputQueueArray), triggering(false), triggered(false), running(false), cursor(0) {}

        virtual void update(void);

				void trigger(void){
					triggering = true;
					running = true;
				};

				// The actual buffer is larger than we need.  This sets the number of samples we need.
				void wavelength(uint32_t w){
					if (w <= BUFSIZE) {
						buflen = w;
					} else {
						Serial.println("error: KS buffer too small for that wavelength");
					}
				};

				void start(void){
					running = true;
				} ;

				void stop(void){
					running = false;
				};

				void clear(void){
					int i;
					for (i=0; i<buflen; i++){
						buffer[i]=0;
					}
				};
private:
				audio_block_t *inputQueueArray[1];
				int16_t buffer[BUFSIZE];
				bool triggering, triggered, running;
				uint32_t triggerPoint, cursor, buflen;
};
