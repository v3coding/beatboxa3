#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
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

void* monitorAccelerometer(void* args);

void* printData(void* args);