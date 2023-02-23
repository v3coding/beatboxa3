#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <alloca.h>


#include <linux/i2c.h>
#include "functions.h"

#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"
#define I2C_DEVICE_ADDRESS 0x18

#define REG_TURN_ON_ACCEL 0x20
#define READADDR 0xA8
#define AxL 0x28
#define AxH 0x29
#define AyL 0x2A
#define AyH 0x2B
#define AzL 0x2C
#define AzH 0x2D
#define REG_DIRA 0x00 // Zen Red uses: 0x02
#define REG_DIRB 0x01 // Zen Red uses: 0x03
#define REG_OUTA 0x14 // Zen Red uses: 0x00
#define REG_OUTB 0x15 // Zen Red uses: 0x01

static int initI2cBus(char* bus, int address){
    int i2cFileDesc = open(bus, O_RDWR);
    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if (result < 0) {
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }
    return i2cFileDesc;
}

static void writeI2cReg(int i2cFileDesc, unsigned char regAddr,unsigned char value){
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int res = write(i2cFileDesc, buff, 2);
    if (res != 2) {
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}

static unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr){
// To read a register, must first write the address
    int res = write(i2cFileDesc, &regAddr, sizeof(regAddr));
    if (res != sizeof(regAddr)) {
        perror("I2C: Unable to write to i2c register.");
        exit(1);
    }
// Now read the value and return it
    char value = 0;
    res = read(i2cFileDesc, &value, sizeof(value));
    if (res != sizeof(value)) {
        perror("I2C: Unable to read from i2c register");
        exit(1);
    }
    return value;
}

void* playSound(void* args){
    threadController* threadData = (threadController*) args;
    while(threadData->programRunning){
        sleep(0.2);
    }
    pthread_exit(0);
}

void* monitorAccelerometer(void* args){
    printf("Monitor Accelerometer Thread Started\n");
    threadController* threadData = (threadController*) args;

    unsigned char buffx[9];
    unsigned char buffy[9];
    unsigned char buffz[9];
    memset(buffx,0,sizeof(buffx));
    memset(buffy,0,sizeof(buffy));
    memset(buffz,0,sizeof(buffz));

    int xhit = 0;
    int yhit = 0;
    int zhit = 0;

    while(threadData->programRunning){
       // threadData->Ax = readI2cReg(threadData->i2cFileDesc,AxL);
       // threadData->Ay = readI2cReg(threadData->i2cFileDesc,AyL);
       // threadData->Ax = readI2cReg(threadData->i2cFileDesc,AzL);
       
       *buffx = readI2cReg(threadData->i2cFileDesc,0x28);
       *(buffx + 4) = readI2cReg(threadData->i2cFileDesc,0x29);
       
       *buffy = readI2cReg(threadData->i2cFileDesc,0x2A);
       *(buffy + 4) = readI2cReg(threadData->i2cFileDesc,0x2B);
       
       *buffz = readI2cReg(threadData->i2cFileDesc,0x2C);
       *(buffz + 4) = readI2cReg(threadData->i2cFileDesc,0x2D);

        int16_t x = (buffx[4] << 8) | (buffx[0]);
        int16_t y = (buffy[4] << 8) | (buffy[0]);
        int16_t z = (buffz[4] << 8) | (buffz[0]);


        if(z > 31000 || z < -10000){
           // printf("Hit Z\n");
            zhit = 1;
            //threadData->hitZ = 1;
        }
        if(x > 28000 || x < -28000){
            //printf("Hit X\n");
            xhit = 1;
            //threadData->hitX = 1;
        }
        if(y > 28000 || y < -28000){
           // printf("Hit Y\n");
            yhit = 1;
            //threadData->hitY = 1;
        }

        if((z < 17000 && z > 15000) && zhit){
            zhit = 0;
            threadData->hitZ = 1;
        }

        if((x < 2000 && x > -2000) && xhit){
            xhit = 0;
            threadData->hitX = 1;
        }

        if((y < 2000 && y > -2000) && yhit){
            yhit = 0;
            threadData->hitY = 1;
        }

        //printf("x = %d y = %d z = %d\n",x,y,z);


        sleep(0.4);
    }
    pthread_exit(0);
}

void* printData(void* args){
    printf("Print Data Thread Started\n");
    threadController* threadData = (threadController*) args;

    while(threadData->programRunning){
      if(threadData->hitX){
        printf("Hit X\n");
        threadData->hitX = 0;
      }
      if(threadData->hitY){
        printf("Hit Y\n");
        threadData->hitY = 0;
      }
      if(threadData->hitZ){
        printf("Hit Z\n");
        threadData->hitZ = 0;
      }
        sleep(0.4);
    }
    pthread_exit(0);
}

void runCommand(char* command)
{
    // Execute the shell command (output into pipe)
    FILE *pipe = popen(command, "r");
    // Ignore output of the command; but consume it
    // so we don't get an error when closing the pipe.
    char buffer[1024];
    while (!feof(pipe) && !ferror(pipe)) {
    if (fgets(buffer, sizeof(buffer), pipe) == NULL)
    break;
    // printf("--> %s", buffer); // Uncomment for debugging
    }
    // Get the exit code from the pipe; non-zero is an error:
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf(" command: %s\n", command);
        printf(" exit code: %d\n", exitCode);
    }
}

// Open the PCM audio output device and configure it.
// Returns a handle to the PCM device; needed for other actions.
snd_pcm_t *Audio_openDevice()
{
	snd_pcm_t *handle;

	// Open the PCM output
	int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Play-back open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	// Configure parameters of PCM output
	err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			NUM_CHANNELS,
			SAMPLE_RATE,
			1,			// Allow software resampling
			50000);		// 0.05 seconds per buffer
	if (err < 0) {
		printf("Play-back configuration error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	return handle;
}

// Read in the file to dynamically allocated memory.
// !! Client code must free memory in wavedata_t !!
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct)
{
	assert(pWaveStruct);

	// Wave file has 44 bytes of header data. This code assumes file
	// is correct format.
	const int DATA_OFFSET_INTO_WAVE = 44;

	// Open file
	FILE *file = fopen(fileName, "r");
	if (file == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
		exit(EXIT_FAILURE);
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	int sizeInBytes = ftell(file) - DATA_OFFSET_INTO_WAVE;
	fseek(file, DATA_OFFSET_INTO_WAVE, SEEK_SET);
	pWaveStruct->numSamples = sizeInBytes / SAMPLE_SIZE;

	// Allocate Space
	pWaveStruct->pData = malloc(sizeInBytes);
	if (pWaveStruct->pData == NULL) {
		fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n",
				sizeInBytes, fileName);
		exit(EXIT_FAILURE);
	}

	// Read data:
	int samplesRead = fread(pWaveStruct->pData, SAMPLE_SIZE, pWaveStruct->numSamples, file);
	if (samplesRead != pWaveStruct->numSamples) {
		fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
				pWaveStruct->numSamples, fileName, samplesRead);
		exit(EXIT_FAILURE);
	}

	fclose(file);
}

// Play the audio file (blocking)
void Audio_playFile(snd_pcm_t *handle, wavedata_t *pWaveData)
{
	// If anything is waiting to be written to screen, can be delayed unless flushed.
	fflush(stdout);

	// Write data and play sound (blocking)
	snd_pcm_sframes_t frames = snd_pcm_writei(handle, pWaveData->pData, pWaveData->numSamples);

	// Check for errors
	if (frames < 0)
		frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0) {
		fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
		exit(EXIT_FAILURE);
	}
	if (frames > 0 && frames < pWaveData->numSamples)
		printf("Short write (expected %d, wrote %li)\n", pWaveData->numSamples, frames);
}

void startProgram(threadController* threadArgument){
    //Configure I2C Pins
    runCommand("config-pin p9_18 i2c");
	runCommand("config-pin p9_17 i2c");
    threadArgument->i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    unsigned char outputValue = readI2cReg(threadArgument->i2cFileDesc,0x20);
    writeI2cReg(threadArgument->i2cFileDesc,REG_TURN_ON_ACCEL,0x00);
    printf("Validating accelerometer memory value at 0x20 is set to 0x00 which is 'off', value = %u\n",outputValue);
    writeI2cReg(threadArgument->i2cFileDesc,REG_TURN_ON_ACCEL,0x27);
    outputValue = readI2cReg(threadArgument->i2cFileDesc,0x20);
    printf("Validating accelerometer memory value at 0x20 is set to 0x01 which is 'on', value = %u\n",outputValue);

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    //Set "Running" boolean to indicate program is to run
    threadArgument->programRunning = 1;

    //Start acceleromter monitoring thread
    pthread_create(&tid, &attr, monitorAccelerometer, threadArgument);
    threadArgument->threadIDs[0] = tid;

    //Start data printing thread
    pthread_create(&tid, &attr, printData, threadArgument);
    threadArgument->threadIDs[1] = tid;

    //play sound thread
    pthread_create(&tid, &attr, playSound, threadArgument);
    threadArgument->threadIDs[2] = tid;

    //Wait for threads to gracefully return
    waitForProgramEnd(threadArgument);
}

void waitForProgramEnd(threadController* threadArgument){

    //Wait for accelerometer thread to join gracefully
    pthread_join(threadArgument->threadIDs[0],NULL);

    //Wait for printing thread to join gracefully
    pthread_join(threadArgument->threadIDs[1],NULL);

    //Wait for printing thread to join gracefully
    pthread_join(threadArgument->threadIDs[2],NULL);
}