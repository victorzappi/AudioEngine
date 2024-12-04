#include "render.h"
#include <cmath>


float frequency = 440.0;
float amplitude = 0.3;
//------------------------------------------------
float phase;
float inverseSampleRate;


bool setup(EngineContext *context, void *userData) {
    (void)userData; // not used, we mute warning

    inverseSampleRate = 1.0 / context->sampleRate;
	phase = 0.0;

    return true;
}


void render(EngineContext *context, void *userData) {
    (void)userData; // not used, we mute warning

   	for(int n=0; n<context->numOfSamples; n++)
	{
		float out = amplitude * sinf(phase);
		phase += 2.0f * (float)M_PI * frequency * inverseSampleRate;

		while(phase > 2.0f *M_PI)
			phase -= 2.0f * (float)M_PI;

        for (int chn = 0; chn<context->numOutChannels; chn++)
            context->framebufferOut[chn][n] = out;
    }
}

void cleanup(EngineContext *context, void *userData) {
    (void)context; // not used, we mute warning
    (void)userData; // not used, we mute warning
}
