//IOS, project 2: Author: Igor Lacko (xlackoi00)
//proj2.h, header file for proj2.c containing headers, structure definitions and function declarations
#ifndef PROJ_2_H
#define PROJ_2_H

//headers
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>

//skier structure
typedef struct{
    long int id;
    long int starting_stop;
} skier;

//skibus structure
typedef struct{
    long int current_stop;
    long int last_stop;
    long int n_passengers;
    long int capacity;
} skibus;

//bus stop structure
typedef struct{
    long int n_waiting;
    long int id;
    sem_t *queue;
    sem_t *bus_arrived;
} bus_stop;


//error handling function and argcheck
void CapacityCheck(long int *args);
void ErrorExit(const char *error_message);

//allocation and free functions
void SharedMemoryInit(long int *args);
void SharedMemoryFree(long int *args);
void ProcKill(long int *args);

//process functions
void Skier(long int *args, skier L);
void Skibus(long int *args);

#endif