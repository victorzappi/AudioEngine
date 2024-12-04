#include "AudioEngine.h"

//#define BOUNCE // Uncomment to save audio of each run
#ifdef BOUNCE
#include <sndfile.h>
#include <string>
#include <dirent.h>  // To handle files in dirs
#include <fstream>   // Read/write to file ifstream/ofstream
#include <sstream>   // ostringstream
using namespace std;

float *outBuffer;

SF_INFO sfinfo;
SNDFILE *outfile;
string outfiledir = ".";
string outfilename = "bounce_";

int getNumOfWavFiles(string dirname) {
    DIR *dir;
    struct dirent *ent;
    int fileCnt = 0;

    // Adapted from http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
    if ((dir = opendir(dirname.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            // Ignore dotfiles and . and .. paths
            if (!strncmp(ent->d_name, ".", 1))
                continue;

            // Take only .wav files
            string name = string(ent->d_name);
            int len = name.length();
            if ((name[len - 4] == '.') && (name[len - 3] == 'w') && (name[len - 2] == 'a') && (name[len - 1] == 'v'))
                fileCnt++;
        }
        closedir(dir);
    } else {
        printf("Could not open directory %s!\n", dirname.c_str());
        return -1;
    }
    return fileCnt;
}
#endif

bool setup(EngineContext *context, void *userData) {
    (void)userData;

#ifdef BOUNCE
    // Allocate buffer for interleaved multichannel audio
    outBuffer = new float[context->numOfSamples * context->numOutChannels];

    // Change output file name according to the number of WAV files in the dir
    int fileNum = getNumOfWavFiles(outfiledir);
    ostringstream convert;
    convert << fileNum;
    outfilename += convert.str() + ".wav";

    // Save audio
    sfinfo.channels = context->numOutChannels;
    sfinfo.samplerate = context->sampleRate;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    outfile = sf_open(outfilename.c_str(), SFM_WRITE, &sfinfo);
    if (!outfile) {
        printf("Failed to open output file: %s\n", outfilename.c_str());
        return false;
    }
#else
    (void)context;
#endif

    return true;
}

void callReadAudioModulesBuffers(AudioEngine *engine, int numOfSamples) {
    // Directly call the private method because this function is a friend
    engine->readAudioModulesBuffers(numOfSamples);
}

void render(EngineContext *context, void *userData) {
    auto *engine = static_cast<AudioEngine *>(userData);

    // Call the engine to fill the output buffers
    callReadAudioModulesBuffers(engine, context->numOfSamples);

#ifdef BOUNCE
    // Interleave multichannel audio into outBuffer
    for (int sample = 0; sample < context->numOfSamples; ++sample) {
        for (int channel = 0; channel < context->numOutChannels; ++channel) {
            outBuffer[sample * context->numOutChannels + channel] = 
                context->framebufferOut[channel][sample];
        }
    }

    // Write interleaved buffer to file
    sf_count_t count = sf_write_float(outfile, outBuffer, context->numOfSamples * context->numOutChannels);
    if (count < context->numOfSamples * context->numOutChannels) {
        printf("Failed to write all samples: wrote %ld of %ld\n",
               count, context->numOfSamples * context->numOutChannels);
    }
#endif
}

void cleanup(EngineContext *context, void *userData) {
    (void)context;
    (void)userData;

#ifdef BOUNCE
    delete[] outBuffer;
    // Properly save audio and close file
    if (outfile) {
        sf_write_sync(outfile);
        sf_close(outfile);
    }
#endif
}
