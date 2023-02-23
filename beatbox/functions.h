#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

// File used for play-back:
// If cross-compiling, must have this file available, via this relative path,
// on the target when the application is run. This example's Makefile copies the wave-files/
// folder along with the executable to ensure both are present.
#define SOURCE_FILE "wave-files/100060__menegass__gui-drum-splash-hard.wav"
//#define SOURCE_FILE "wave-files/100053__menegass__gui-drum-cc.wav"

#define SAMPLE_RATE   44100
#define NUM_CHANNELS  1
#define SAMPLE_SIZE   (sizeof(short)) 	// bytes per sample

// Store data of a single wave file read into memory.
// Space is dynamically allocated; must be freed correctly!
typedef struct {
	int numSamples;
	short *pData;
} wavedata_t;

// Prototypes:
snd_pcm_t *Audio_openDevice();
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct);
void Audio_playFile(snd_pcm_t *handle, wavedata_t *pWaveData);

typedef struct threadController{
    //Hit on X
    int hitX;
    //Hit on Y
    int hitY;
    //Hit on Z
    int hitZ;
    //boolean int to control startup and shutdown of threads
    int programRunning;
    //array of thread ID's
    pthread_t* threadIDs;
    //i2c file desc
    int i2cFileDesc;
} threadController;

void startProgram(threadController* threadArgument);

void waitForProgramEnd(threadController* threadArgument);

void* playSound(void* args);

void* monitorAccelerometer(void* args);

void* printData(void* args);