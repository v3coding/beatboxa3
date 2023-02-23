#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
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


void* monitorAccelerometer(void* args){
    printf("Monitor Accelerometer Thread Started\n");
    threadController* threadData = (threadController*) args;

    unsigned char buff[50];
    memset(buff,0,sizeof(buff));

    while(threadData->programRunning){
       // threadData->Ax = readI2cReg(threadData->i2cFileDesc,AxL);
       // threadData->Ay = readI2cReg(threadData->i2cFileDesc,AyL);
       // threadData->Ax = readI2cReg(threadData->i2cFileDesc,AzL);
       
       *buff = readI2cReg(threadData->i2cFileDesc,READADDR);
       int16_t x = (buff[0] << 8) | (buff[15]);
       int16_t y = (buff[16] << 8) | (buff[32]);
       int16_t z = (buff[32] << 8) | (buff[49]);
       printf("x = %d, y = %d, z = %d\n",x,y,z);
        sleep(1.0);
    }
    pthread_exit(0);
}

void* printData(void* args){
    printf("Print Data Thread Started\n");
    threadController* threadData = (threadController*) args;

    while(threadData->programRunning){
      //  printf("Ax = %u  Ay = %u  Az = %u\n", threadData->Ax, threadData->Ay, threadData->Az);
        sleep(1);
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

    //Wait for threads to gracefully return
    waitForProgramEnd(threadArgument);
}

void waitForProgramEnd(threadController* threadArgument){

    //Wait for accelerometer thread to join gracefully
    pthread_join(threadArgument->threadIDs[0],NULL);

    //Wait for printing thread to join gracefully
    pthread_join(threadArgument->threadIDs[1],NULL);
    
}