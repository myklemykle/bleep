#include "Arduino.h"
#include <AudioStream.h>

#ifndef AudioSynthBytebeat_h
#define AudioSynthBytebeat_h

#define NUMRECIPES 9

class AudioSynthBytebeat : public AudioStream
{
public:
        AudioSynthBytebeat() : AudioStream(0, NULL) {
					t = 0;
					recipe = 0;
					running = false;
				}
				void time(uint32_t tm) {
					t = tm;
				}
				void setRecipe(int r) {
					recipe = r %NUMRECIPES;
				}
				void nextRecipe() {
					recipe = ++recipe % NUMRECIPES;
				}
				void start(){
					running = true;
				}
				void stop(){
					running = false;
				}
        virtual void update(void);
private:
        uint32_t t;
				uint8_t recipe;
				bool running;
};

#endif
