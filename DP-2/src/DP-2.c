

// 1 char then sleep 1/20 second, remember to check space left enough for 1

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

int main(int argc, char *argv[])
{
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
        // Fork failed
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (dc_pid == 0)
    {       
        // Execute the DC program
        // Arguments for DC's main: argv[0]=program name, argv[1]=shmID, argv[2]=DP1_PID, argv[3]=DP2_PID
        int exec_ret = execl("../../dc/bin/dc", "dc", shm_id_str, dp1_pid_str, dp2_pid_str, (char *)NULL);

        // If execl returns, it means an error occurred
        if (exec_ret == -1)
        {
            perror("execl failed to launch DC");
            // Use _exit in child after fork on error to avoid flushing parent's stdio buffers
            _exit(EXIT_FAILURE);
        }
    }
    else
    {
        printf("DP-2: Successfully launched DC process with PID: %d\n", dc_pid); // Debugging print

        // Attach shared memory
        shared_memory *shm = (shared_memory *)shmat(shm_id, NULL, 0);
        if (shm == (void *)-1)
        {
            perror("shmat failed");
            exit(1);
        }
        printf("DP-2: Attached to shared memory at address: %p\n", (void*)shm); // Debugging print


        while (1)
        {
        }
    }

        

        return 0;
    }
