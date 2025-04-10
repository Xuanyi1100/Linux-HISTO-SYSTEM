/**
 * @file DP-1.c
 * @brief Data Producer 1 (DP-1) for histogram system
 * 
 * Date: 2025-04-05
 * Sp_05
 * Group member: Deyi, Zhizheng
 * Main producer process that creates shared memory and semaphores.
 * Generates random characters ('A'-'T') in batches and writes them
 * to shared memory. Forks DP-2 process.
 * 
 * Usage: ./DP-1
 */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h> // For semaphores
#include <errno.h>   // For error handling
#include <unistd.h>
#include <signal.h>
#include "../../common/hs.h"
#include <time.h> // For srand

// --- Globals for Signal Handler ---
static int sem_id_global = -1;
static int shm_id_global = -1;
static shared_memory *shm_ptr_global = NULL;

// --- Signal Handler ---
/**
 * @brief SIGINT handler for cleanup
 * @param sig Signal number
 * 
 * Detaches shared memory before exiting
 */
void cleanup(int sig)
{
    if (shm_ptr_global != NULL && shm_ptr_global != (void *)-1)
    {
        shmdt(shm_ptr_global); // Only detach
    }
    exit(0);
}

/**
 * @brief Creates or gets semaphore
 * @return int Semaphore ID
 * 
 * Creates new semaphore with 0666 permissions if not exists,
 * otherwise gets existing one. Initializes to unlocked state (1).
 */
int create_semaphore()
{
    key_t sem_key = ftok(SEM_KEY_PATH, SEM_KEY_ID);
    int sem_id = semget(sem_key, 1, 0666);
    if (sem_id == -1)
    {
        if (errno == ENOENT)
        { // Doesn't exist, create it
            sem_id = semget(sem_key, 1, IPC_CREAT | 0666);
            if (sem_id == -1)
            {
                perror("semget failed");
                exit(1);
            }
        }
        else
        {
            perror("semget failed");
            exit(1);
        }
    }

    // Initialize semaphore value to 1 (unlocked)
    union semun arg;
    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1)
    {
        perror("semctl (SETVAL) failed");
        exit(1);
    }
    return sem_id;
}

/**
 * @brief Creates or gets shared memory segment
 * @return int Shared memory ID
 */
int create_or_get_shm()
{
    key_t key = ftok(".", 'S');
    if (key == -1)
    {
        perror("ftok failed");
        exit(1);
    }

    // Check if shared memory exists
    int shm_id = shmget(key, sizeof(shared_memory), 0666);
    if (shm_id != -1)
    {
        return shm_id;
    }

    // Create new shared memory
    shm_id = shmget(key, sizeof(shared_memory), IPC_CREAT | 0666);
    if (shm_id == -1)
    {
        perror("shmget (creation) failed");
        exit(1);
    }

    return shm_id;
}

/**
 * @brief Writes characters to shared memory buffer
 * 
 * @param chars_to_write Number of characters to generate
 * @param shm Pointer to shared memory structure
 * 
 * Generates random characters 'A'-'T' and writes to buffer
 */
void writeChars(int chars_to_write, shared_memory *shm)
{
    for (int i = 0; i < chars_to_write; i++)
    {
        char random_char = 'A' + (rand() % 20);
        shm->buffer[shm->write_index] = random_char;
        // Update write index circularly.
        shm->write_index = (shm->write_index + 1) % BUFFER;
    }
}

/**
 * @brief Main DP-1 program
 * 
 * Initializes shared memory and semaphores, forks DP-2,
 * and runs main production loop with 2-second intervals
 */
int main()
{
    srand(time(NULL));       // Seed random generator
    signal(SIGINT, cleanup); // Register signal handler

    // Create shared memory
    shm_id_global = create_or_get_shm();
    // attaching shared memory to get the pointer
    shm_ptr_global = (shared_memory *)shmat(shm_id_global, NULL, 0);
    if (shm_ptr_global == (void *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    // protentally incorrect when sm exsits
    shm_ptr_global->write_index = 0;
    shm_ptr_global->read_index = 0;
    memset(shm_ptr_global->buffer, 0, BUFFER);

    // Create semaphore
    sem_id_global = create_semaphore();

    // Fork DP-2 and pass both shm_id and sem_id
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process (DP-2)
        char shm_str[20];
        sprintf(shm_str, "%d", shm_id_global);
        execl("../../DP-2/bin/DP-2", "DP-2", shm_str, NULL);
        perror("execl failed");
        // Detach before exiting on error
        if (shm_ptr_global != NULL && shm_ptr_global != (void *)-1)
        {
            shmdt(shm_ptr_global); // Detach before error exit
        }
        exit(1); // Exit with error status
    }
    else if (pid > 0)
    {      
        // DP-1 main loop
        while (1)
        {

            // --- Acquire Semaphore ---
            // Wait until the semaphore is available (value > 0) and decrement it.
            if (semop(sem_id_global, &lock, 1) == -1)
            {
                perror("DP-1 semop failed");
                break;
            }

            // when r is 10 w is 8, we can't write at 9 because then w will be 10, it's called overtake
            int availableSpace = (shm_ptr_global->read_index - shm_ptr_global->write_index - 1 + BUFFER) % BUFFER;
            int charsToWrite = availableSpace >= 20 ? 20 : availableSpace;
            writeChars(charsToWrite, shm_ptr_global);

            // Unlock semaphore
            if (semop(sem_id_global, &unlock, 1) == -1)
            {
                perror("DP-1 semop failed");
                break;
            }

            sleep(2);
        }
    }
    else
    {
        perror("fork failed");
        if (shm_ptr_global != NULL && shm_ptr_global != (void *)-1)
        {
            shmdt(shm_ptr_global); // Detach before error exit
        }
        exit(1); // Exit with error status
    }

    return 0;
}