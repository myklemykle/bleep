#include <AudioStream.h>

#define BUFSIZE 128*32 // probably too large ... TODO calculate the max size, for the lowest audible freq.

#define cursorMinus(number) (number > cursor) ? cursor + BUFSIZE - number : cursor - number
#define cursorPlus(number) (number + cursor >= BUFSIZE) ? cursor + number - BUFSIZE : cursor + number
#define incCursor() if (++cursor >= BUFSIZE) { cursor = 0; }

class AudioSynthKS : public AudioStream
{
public:
        AudioSynthKS() : AudioStream(1, inputQueueArray), triggering(false), triggered(false), running(false), cursor(0), buflen(0), magic1(0) {}

        virtual void update(void);

				void trigger(void){
					triggering = true;
					running = true;
				};

				// The actual buffer is larger than we need.  This sets the number of samples we need.
				void wavelength(uint32_t w){
					if (w <= BUFSIZE) {
						Serial.print("ks buflen:");
						Serial.println(w, DEC);
						buflen = w;
					} else {
						Serial.println("error: KS buffer too small for that wavelength");
					}
					magic1 = (float)buflen / 3.1416;
					Serial.print("magic1 = ");
					Serial.println(magic1, DEC);
				};

				void start(void){
					running = true;
				} ;

				void stop(void){
					running = false;
				};

				void clear(void){
					unsigned int i;
					for (i=0; i<BUFSIZE; i++){
						buffer[i]=0;
					}
				};
private:
				audio_block_t *inputQueueArray[1];
				int16_t buffer[BUFSIZE];
				bool triggering, triggered, running;
				uint32_t triggerPoint, cursor, buflen, magic1;
};
