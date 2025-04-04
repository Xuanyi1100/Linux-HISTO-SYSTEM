#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>
#include "../../common/hs.h" // Assuming this exists and is needed
#include <sys/time.h>


static shared_memory *shm_ptr_global = NULL;
pid_t dp1_pid;
pid_t dp2_pid;

// --- Signal Handler ---
void timeToQuit(int sig)
{
    // sends SIGINT to DPs,
    kill(dp1_pid, SIGINT);
    kill(dp2_pid, SIGINT);

    // reads remaining data,
    // displays final histogram,

    //  exits with "Shazam!!".
    printf("Shazam!!\n");
    exit(0);
}
void wakeup_handler(int signum) {
    printf("Wake-up call! (SIGALRM received)\n");
    alarm(2);
    
}

int main(int argc, char *argv[])
{
    signal(SIGINT, timeToQuit);
    signal(SIGALRM, wakeup_handler);

    printf("DC starts\n");

    if (argc != 4)
    {
        // Print error to standard error
        fprintf(stderr, "DC Error: Incorrect number of arguments.\n");
        fprintf(stderr, "Usage: %s <shared_memory_id> <dp1_pid> <dp2_pid>\n", argv[0]);
        // Exit indicating failure
        exit(EXIT_FAILURE);
    }

    int shm_id = atoi(argv[1]);
    dp1_pid = (pid_t)atoi(argv[2]); // Cast result of atoi to pid_t
    dp2_pid = (pid_t)atoi(argv[3]); // Cast result of atoi to pid_t

    // Attach shared memory
    shm_ptr_global = (shared_memory *)shmat(shm_id, NULL, 0);
    if (shm_ptr_global == (void *)-1)
    {
        perror("shmat failed");
        exit(1);
    }
    printf("DC: Attached to shared memory at address: %p\n", (void *)shm_ptr_global); // Debugging print

    // --- Get Semaphore  ---
    key_t sem_key = ftok(SEM_KEY_PATH, SEM_KEY_ID);
    if (sem_key == -1)
    {
        perror("ftok failed in DP-2");
        exit(EXIT_FAILURE);
    }
    int sem_id = semget(sem_key, 1, 0);
    if (sem_id == -1)
    {
        perror("semget failed in DP-2");
        exit(1);
    }

    // Set the first alarm
    alarm(2);

    while (1)
    {
        pause();  // Sleep until a signal is received
        // --- Acquire Semaphore ---
        if (semop(sem_id, &lock, 1) == -1)
        {
            perror("DC semop failed");
            break;
        }

        // read 40 charaters
        for(int i = 0, i < 40, i++)
        {
            char readChar = shm_ptr_global->buffer[shm_ptr_global->read_index]
            
            shm_ptr_global->read_index = (shm_ptr_global->read_index + 1) % BUFFER;

        }

        // make sure not overtake write index

        // Display every 10 seconds with symbols:
        // - for ones place,
        // + for tens place,
        // * for hundreds place

        // Unlock semaphore
        if (semop(sem_id, &unlock, 1) == -1)
        {
            perror("DC semop failed");
            break;
        }

        sleep(10);
    }

    return 0;
}
