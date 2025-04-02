#ifndef HISTO_SYSTEM_H
#define HISTO_SYSTEM_H

#include <sys/types.h>

// Shared memory structure
typedef struct {
    char buffer[256];
    int write_index;
    int read_index;
} shared_memory;

// Semaphore key parameters (same for all processes)
#define SEM_KEY_PATH "/tmp"     // Same file/dir for ftok()
#define SEM_KEY_ID 'M'       // Unique project ID for semaphore

#endif