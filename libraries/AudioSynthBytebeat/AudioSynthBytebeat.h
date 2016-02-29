#include <AudioStream.h>

class AudioSynthBytebeat : public AudioStream
{
public:
        AudioSynthBytebeat() : 
				AudioStream(0, NULL), time(0), 
        virtual void update(void);
private:
        unsigned int t;
};


