#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h> // Needed for pid_t
#include <sys/wait.h>  // Needed for waitpid (optional, but good practice sometimes)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Needed for getpid, getppid, fork, execlp, sleep
#include <string.h> // Needed for sprintf
#include <time.h>
#include "../../common/hs.h"


static shared_memory *shm_ptr_global = NULL;

// --- Signal Handler ---
void cleanup(int sig)
{
    if (shm_ptr_global != NULL && shm_ptr_global != (void *)-1)
    {
        shmdt(shm_ptr_global); // Only detach
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));       // Seed random generator
    signal(SIGINT, cleanup); // Register signal handler
    
    // Validate argument count
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <shm_id>:%s \n", argv[0], argv[1]);
        exit(1);
    }

    // Get Shared Memory ID
    int shm_id = atoi(argv[1]);
    if (shm_id <= 0)
    {
        fprintf(stderr, "Invalid shared memory ID: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
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

    // --- Get PIDs ---
    pid_t dp2_pid = getpid();
    pid_t dp1_pid = getppid();
    printf("DP-2 PID: %d, Parent (DP-1) PID: %d\n", dp2_pid, dp1_pid); // Debugging print

    // --- Prepare arguments for DC ---
    char shm_id_str[20];
    char dp1_pid_str[20];
    char dp2_pid_str[20];

    sprintf(shm_id_str, "%d", shm_id);
    sprintf(dp1_pid_str, "%d", dp1_pid);
    sprintf(dp2_pid_str, "%d", dp2_pid);

    // --- Fork and Launch DC ---
    pid_t dc_pid = fork();

    if (dc_pid < 0)
    {
        perror("fork failed");
        if (shm_ptr_global != NULL && shm_ptr_global != (void *)-1)
        {
            shmdt(shm_ptr_global); // Detach before error exit
        }
        exit(1); // Exit with error status
    }
    else if (dc_pid == 0)
    {
        // Execute the DC program
        // Arguments for DC's main: argv[0]=program name, argv[1]=shmID, argv[2]=DP1_PID, argv[3]=DP2_PID
        int exec_ret = execl("../../dc/bin/dc", "dc", shm_id_str, dp1_pid_str, dp2_pid_str, (char *)NULL);

        // If execl returns, it means an error occurred
        if (exec_ret == -1)
        {
            perror("execl failed");
            // Detach before exiting on error
            if (shm_ptr_global != NULL && shm_ptr_global != (void *)-1)
            {
                shmdt(shm_ptr_global); // Detach before error exit
            }
            exit(1); // Exit with error status
        }
    }
    else
    {
        printf("DP-2: Successfully launched DC process with PID: %d\n", dc_pid); // Debugging print

        // Attach shared memory
        shm_ptr_global = (shared_memory *)shmat(shm_id, NULL, 0);
        if (shm_ptr_global == (void *)-1)
        {
            perror("shmat failed");
            exit(1);
        }
        printf("DP-2: Attached to shared memory at address: %p\n", (void *)shm_ptr_global); // Debugging print

        while (1)
        {

            // --- Acquire Semaphore ---
            // Wait until the semaphore is available (value > 0) and decrement it.
            if (semop(sem_id, &lock, 1) == -1)
            {
                perror("DP-2 semop failed");
                break;
            }

            int availableSpace = (shm_ptr_global->read_index - shm_ptr_global->write_index - 1 + BUFFER) % BUFFER;
            if (availableSpace >= 1)
            {
                char random_char = 'A' + (rand() % 20);
                shm_ptr_global->buffer[shm_ptr_global->write_index] = random_char;
                // Update write index circularly.
                shm_ptr_global->write_index = (shm_ptr_global->write_index + 1) % BUFFER;
                printf("DP-2 wrote 1 character.\n"); // Debugging
            }

            // Unlock semaphore
            if (semop(sem_id, &unlock, 1) == -1)
            {
                perror("DP-2 semop failed");
                break;
            }

            usleep(50000);// 1/20 second
        }
    }

    return 0;
}
