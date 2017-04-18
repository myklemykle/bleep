#include <AudioStream.h>
#include <Wire.h>

#define BUFSIZE 128*32 // probably too large ... TODO calculate the max size, for the lowest audible freq.

#define cursorMinus(number) (number > cursor) ? cursor + _bufmax - number : cursor - number
#define cursorPlus(number) (number + cursor >= _bufmax) ? cursor + number - _bufmax : cursor + number
#define incCursor() if (++cursor >= _bufmax) { cursor = 0; }

class AudioSynthKS : public AudioStream
{
public:
        AudioSynthKS() : AudioStream(2, inputQueueArray), 
					triggering(false), 
					triggered(false), 
					running(false), 
					cursor(0), 
					buflen(0), 
					_bufmax(BUFSIZE),
					_magic1(0),
					_magic2(0),
					_decayBalance(65535),
					_drumness(64),
					_exciter(0) {}

        virtual void update(void);

				void trigger(void){
					triggering = true;
					running = true;
				};

				// Can choose which waveform to take excitation from:
				void exciter(int s){
					_exciter = s;
				};

				void magic1(uint32_t m) {
					_magic1 = m;
				};

				void magic2(uint32_t m) {
					_magic2 = m;
				};

				void bufmax(uint32_t m) {
					if (m <= BUFSIZE) 
						_bufmax = m;
				}

				void decayBalance(int d) {
					if (0 <= d && d <= 65535)
						_decayBalance = d;
				}

				void drumness(int d) {
						_drumness = d;
				}

				// The actual buffer is larger than we need.  This sets the number of samples we need.
				void wavelength(uint32_t w){
					if (w <= _bufmax) {
						Serial.print("ks buflen:");
						Serial.println(w, DEC);
						buflen = w;
					} else {
						Serial.println("error: KS buffer too small for that wavelength");
					}
					_magic1 = (float)buflen / (3.1416 * 2);
					Serial.print("magic1 = ");
					Serial.println(_magic1, DEC);
				};

				void start(void){
					running = true;
				} ;

				void stop(void){
					running = false;
				};

				void clear(void){
					unsigned int i;
					for (i=0; i<_bufmax; i++){
						buffer[i]=0;
					}
				};

private:
				uint8_t _exciter;
				audio_block_t *inputQueueArray[1];
				int16_t buffer[BUFSIZE];
				bool triggering, triggered, running;
				uint32_t triggerPoint, cursor, buflen, _bufmax, _magic1, _magic2;
				uint16_t _decayBalance;  // between 0 and 65535
				uint8_t _drumness; // 0 - 127
};
