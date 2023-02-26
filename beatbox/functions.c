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
#include "audioMixer_template.h"

#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"
#define I2C_DEVICE_ADDRESS 0x18

#define SOURCE_FILE1 "wave-files/100051__menegass__gui-drum-bd-hard.wav"
#define SOURCE_FILE2 "wave-files/100066__menegass__gui-drum-tom-mid-hard.wav"
#define SOURCE_FILE3 "wave-files/100061__menegass__gui-drum-splash-soft.wav"
#define SOURCE_FILE4 "wave-files/100055__menegass__gui-drum-co.wav"
#define SOURCE_FILE5 "wave-files/100066__menegass__gui-drum-tom-mid-hard.wav"
#define SOURCE_FILE6 "wave-files/100065__menegass__gui-drum-tom-lo-soft.wav"

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

void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

void configureInput(){
    FILE *pFile;
    pFile = fopen("/sys/class/gpio/gpio26/direction", "w");
    fprintf(pFile,"in");
    pFile = fopen("/sys/class/gpio/gpio46/direction", "w");
    fprintf(pFile,"in");
    pFile = fopen("/sys/class/gpio/gpio65/direction", "w");
    fprintf(pFile,"in");
    pFile = fopen("/sys/class/gpio/gpio47/direction", "w");
    fprintf(pFile,"in");
}

int readJoystick(int joystick){
    //1 is up, 2 is down
    FILE *pFile;

    if(joystick == 1){
        pFile = fopen("/sys/class/gpio/gpio26/value", "r");
    }
    if(joystick == 2){
        pFile = fopen("/sys/class/gpio/gpio46/value", "r");
    }
    if(joystick == 3){
        pFile = fopen("/sys/class/gpio/gpio65/value", "r");
    }
    if(joystick == 4){
        pFile = fopen("/sys/class/gpio/gpio47/value", "r");
    }
    char buff[1024];
    fgets(buff, 1024, pFile);
    //printf("Read : '%s'",buff);
    //printf("First Bit = %c\n",buff[0]);
    fclose(pFile);
    if(buff[0] == '0'){
        return 1;
    }
    return 0;
}

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

void* monitorJoystick(void* args){
    configureInput();

    threadController* threadData = (threadController*) args;

    threadData->volume = 80;
    AudioMixer_setVolume(threadData->volume);
    threadData->tempo = 120;

    while(threadData->programRunning){


        if(readJoystick(1)){
            if(threadData->volume < 100){
                threadData->volume += 1;
                AudioMixer_setVolume(threadData->volume);
            }
            printf("Volume : %d\n",threadData->volume);
            sleepForMs(150);
        }

        if(readJoystick(2)){
            if(threadData->volume > 0){
                threadData->volume -= 1;
                AudioMixer_setVolume(threadData->volume);
            }
            printf("Volume : %d\n",threadData->volume);
            sleepForMs(150);
        }

        if(readJoystick(3)){
            if(threadData->tempo > 40){
                threadData->tempo--;
            }
            printf("Tempo : %d\n",threadData->tempo);
            sleepForMs(150);
        }

        if(readJoystick(4)){
            if(threadData->tempo < 300){
                threadData->tempo++;
            }
            printf("Tempo : %d\n",threadData->tempo);
            sleepForMs(150);
        }
        
        sleepForMs(20);
    }
    pthread_exit(0);
}

void* printData(void* args){
    threadController* threadData = (threadController*) args;
    while(threadData->programRunning){
        sleep(0.2);
    }
    pthread_exit(0);
}

// void* monitorAccelerometer(void* args){
//     printf("Monitor Accelerometer Thread Started\n");
//     threadController* threadData = (threadController*) args;

//     unsigned char buffx[9];
//     unsigned char buffy[9];
//     unsigned char buffz[9];
//     memset(buffx,0,sizeof(buffx));
//     memset(buffy,0,sizeof(buffy));
//     memset(buffz,0,sizeof(buffz));

//     int xhit = 0;
//     int yhit = 0;
//     int zhit = 0;
//     int hit = 0;

//     while(threadData->programRunning){
//        // threadData->Ax = readI2cReg(threadData->i2cFileDesc,AxL);
//        // threadData->Ay = readI2cReg(threadData->i2cFileDesc,AyL);
//        // threadData->Ax = readI2cReg(threadData->i2cFileDesc,AzL);
       
//        *buffx = readI2cReg(threadData->i2cFileDesc,0x28);
//        *(buffx + 4) = readI2cReg(threadData->i2cFileDesc,0x29);
       
//        *buffy = readI2cReg(threadData->i2cFileDesc,0x2A);
//        *(buffy + 4) = readI2cReg(threadData->i2cFileDesc,0x2B);
       
//        *buffz = readI2cReg(threadData->i2cFileDesc,0x2C);
//        *(buffz + 4) = readI2cReg(threadData->i2cFileDesc,0x2D);

//         int16_t x = (buffx[4] << 8) | (buffx[0]);
//         int16_t y = (buffy[4] << 8) | (buffy[0]);
//         int16_t z = (buffz[4] << 8) | (buffz[0]);


//         if((z > 28000 || z < -7000) && !hit){
//            // printf("Hit Z\n");
//             hit = 1;
//             zhit = 1;
//             //threadData->hitZ = 1;
//         }
//         if((x > 25000 || x < -25000) && !hit){
//             //printf("Hit X\n");
//             hit = 1;
//             xhit = 1;
//             //threadData->hitX = 1;
//         }
//         if((y > 25000 || y < -25000) && !hit){
//            // printf("Hit Y\n");
//            hit = 1;
//            yhit = 1;
//             //threadData->hitY = 1;
//         }

//         if((z < 18000 && z > 15000) && zhit){
//             zhit = 0;
//             hit = 0;
//             threadData->hitZ = 1;
//         }

//         if((x < 3000 && x > -3000) && xhit){
//             xhit = 0;
//             hit = 0;
//             threadData->hitX = 1;
//         }

//         if((y < 3000 && y > -3000) && yhit){
//             yhit = 0;
//             hit = 0;
//             threadData->hitY = 1;
//         }

//         //printf("x = %d y = %d z = %d\n",x,y,z);


//         sleepForMs(10);
//     }
//     pthread_exit(0);
// }

void* monitorAccelerometerX(void* args){
    threadController* threadData = (threadController*) args;

    unsigned char buffx[9];
    memset(buffx,0,sizeof(buffx));
    int xhit = 0;

    while(threadData->programRunning){
       *buffx = readI2cReg(threadData->i2cFileDesc,0x28);
       *(buffx + 4) = readI2cReg(threadData->i2cFileDesc,0x29);
        int16_t x = (buffx[4] << 8) | (buffx[0]);

        if((x > 25000 || x < -25000)){
            threadData->hitX = 1;
            sleepForMs(200);
        }
        sleepForMs(10);
    }
    pthread_exit(0);
}

void* monitorAccelerometerY(void* args){
    threadController* threadData = (threadController*) args;

    unsigned char buffy[9];
    memset(buffy,0,sizeof(buffy));
    int yhit = 0;

    while(threadData->programRunning){
       *buffy = readI2cReg(threadData->i2cFileDesc,0x2A);
       *(buffy + 4) = readI2cReg(threadData->i2cFileDesc,0x2B);
        int16_t y = (buffy[4] << 8) | (buffy[0]);

        if((y > 25000 || y < -25000)){
            threadData->hitY = 1;
            sleepForMs(200);
        }
        sleepForMs(10);
    }
    pthread_exit(0);
}

void* monitorAccelerometerZ(void* args){
    threadController* threadData = (threadController*) args;

    unsigned char buffz[9];
    memset(buffz,0,sizeof(buffz));
    int zhit = 0;

    while(threadData->programRunning){
       
       *buffz = readI2cReg(threadData->i2cFileDesc,0x2C);
       *(buffz + 4) = readI2cReg(threadData->i2cFileDesc,0x2D);
        int16_t z = (buffz[4] << 8) | (buffz[0]);


        if((z > 28000 || z < -7000)){
            threadData->hitZ = 1;
            sleepForMs(200);
        }
        sleepForMs(10);
    }
    pthread_exit(0);
}

void* playSound(void* args){
    printf("Print Data Thread Started\n");
    threadController* threadData = (threadController*) args;

    wavedata_t sampleFile1;
    wavedata_t sampleFile2;
    wavedata_t sampleFile3;
    wavedata_t sampleFile4;
    wavedata_t sampleFile5;
    wavedata_t sampleFile6;

    AudioMixer_init();
    AudioMixer_readWaveFileIntoMemory(SOURCE_FILE1, &sampleFile1);
    AudioMixer_readWaveFileIntoMemory(SOURCE_FILE2, &sampleFile2);
    AudioMixer_readWaveFileIntoMemory(SOURCE_FILE3, &sampleFile3);
    AudioMixer_readWaveFileIntoMemory(SOURCE_FILE4, &sampleFile4);
    AudioMixer_readWaveFileIntoMemory(SOURCE_FILE5, &sampleFile5);
    AudioMixer_readWaveFileIntoMemory(SOURCE_FILE6, &sampleFile6);




    while(threadData->programRunning){
        int mode = threadData->mode;
      if(threadData->hitX){
        printf("Hit X\n");
        if(mode == 1){
            AudioMixer_queueSound(&sampleFile1);
        } else{
            AudioMixer_queueSound(&sampleFile4);
        }
        threadData->hitX = 0;
      }
      if(threadData->hitY){
        if(mode == 1){
            AudioMixer_queueSound(&sampleFile2);
        } else{
            AudioMixer_queueSound(&sampleFile5);
        }
        printf("Hit Y\n");
        threadData->hitY = 0;
      }
      if(threadData->hitZ){
        if(mode == 1){
            AudioMixer_queueSound(&sampleFile3);
        } else{
            AudioMixer_queueSound(&sampleFile6);
        }
        printf("Hit Z\n");
        threadData->hitZ = 0;
      }
      sleepForMs((60/threadData->tempo/2)*1000);
    }
    AudioMixer_cleanup();
    AudioMixer_freeWaveFileData(&sampleFile1);
    AudioMixer_freeWaveFileData(&sampleFile2);
    AudioMixer_freeWaveFileData(&sampleFile3);
    AudioMixer_freeWaveFileData(&sampleFile4);
    AudioMixer_freeWaveFileData(&sampleFile5);
    AudioMixer_freeWaveFileData(&sampleFile6);
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

void startProgram(threadController* threadArgument){
    //set mode to default
    threadArgument->mode = 1;
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

    //Start acceleromter monitoring threads
    pthread_create(&tid, &attr, monitorAccelerometerX, threadArgument);
    threadArgument->threadIDs[0] = tid;

    pthread_create(&tid, &attr, monitorAccelerometerY, threadArgument);
    threadArgument->threadIDs[1] = tid;

    pthread_create(&tid, &attr, monitorAccelerometerZ, threadArgument);
    threadArgument->threadIDs[2] = tid;

    //Start data printing thread
    pthread_create(&tid, &attr, printData, threadArgument);
    threadArgument->threadIDs[3] = tid;

    //play sound thread
    pthread_create(&tid, &attr, playSound, threadArgument);
    threadArgument->threadIDs[4] = tid;

    //monitorJoystick thread
    pthread_create(&tid, &attr, monitorJoystick, threadArgument);
    threadArgument->threadIDs[5] = tid;

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