#ifndef HISTO_SYSTEM_H
#define HISTO_SYSTEM_H

#include <sys/types.h>

// Shared memory structure
typedef struct
{
    char buffer[256];
    int write_index;
    int read_index;
} shared_memory;

// Semaphore key parameters (same for all processes)
#define SEM_KEY_PATH "/tmp" // Same file/dir for ftok()
#define SEM_KEY_ID 'M'      // Unique project ID for semaphore

#endif

struct sembuf lock = {0, -1, SEM_UNDO};
struct sembuf unlock = {0, 1, SEM_UNDO};

union semun
{
    int val;               // Value for SETVAL
    struct semid_ds *buf;  // Buffer for IPC_STAT, IPC_SET
    unsigned short *array; // Array for GETALL, SETALL
};