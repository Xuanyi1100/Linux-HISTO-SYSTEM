/**
 * @file DC.c
 * @brief Data Collector (DC) for histogram system
* Date: 2025-04-05
 * Sp_05
 * Group member: Deyi, Zhizheng
 * This program collects character data from shared memory, maintains a histogram,
 * and displays it periodically. It coordinates with DP-1 and DP-2 processes using
 * semaphores and shared memory. Handles SIGINT for clean termination.
 * 
 * Usage: ./dc <shared_memory_id> <dp1_pid> <dp2_pid>
 */
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

volatile sig_atomic_t terminate_flag = 0; // Set by SIGINT handler
volatile sig_atomic_t display_flag = 0;   // Flag for 10-second display
int letter_counts[26] = {0};
const int display_interval = 5;        // Display every 5 * 2 seconds = 10 seconds
int alarm_counter = 0; 

/**
 * @brief Displays histogram of character frequencies
 * 
 * Clears the screen and prints a histogram showing frequency distribution
 * of characters 'A'-'T' using stars, pluses and minuses for hundreds, tens
 * and units respectively.
 */
void display_histogram()
{
    system("clear");  // Clear screen
    for (int i = 0; i < 20; i++) { // Only 'A' through 'T'
        char letter = 'A' + i;
        long long count = letter_counts[i];
        long long hundreds = count / 100;
        long long tens = (count % 100) / 10;
        long long ones = count % 10;

        printf("%c-%03lld ", letter, count); // Print letter and count

        for(int j = 0; j < hundreds; ++j) printf("*");
        for(int j = 0; j < tens; ++j) printf("+");
        for(int j = 0; j < ones; ++j) printf("-");
        printf("\n");
    }
    fflush(stdout); // Ensure output is immediately visible
}
// --- Signal Handler ---
/**
 * @brief SIGINT handler for clean termination
 * 
 * @param sig Signal number (should be SIGINT)
 * 
 * Sends SIGINT to DP processes and sets termination flag
 */
void timeToQuit(int sig)
{
    // this handler should only set the flag and call kill. It must not do anything else
    kill(dp1_pid, SIGINT);
    kill(dp2_pid, SIGINT);
    terminate_flag = 1;

}
/**
 * @brief SIGALRM handler for periodic wakeups
 * 
 * @param signum Signal number (should be SIGALRM)
 * 
 * Manages display timing by counting 2-second intervals
 */
void wakeup_handler(int signum) {

    alarm(2);
    // display mamagement
    alarm_counter++;
    if (alarm_counter >= display_interval) {
        display_flag = 1;
        alarm_counter = 0; // Reset counter
    }
    
}
/**
 * @brief Main DC program
 * 
 * @param argc Argument count
 * @param argv Arguments: [program_name, shm_id, dp1_pid, dp2_pid]
 * @return int Exit status
 * 
 * Sets up signal handlers, attaches to shared memory, manages semaphores,
 * and runs main collection loop with periodic display updates.
 */
int main(int argc, char *argv[])
{
    signal(SIGINT, timeToQuit);
    signal(SIGALRM, wakeup_handler);

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

    while (!terminate_flag)
    {
        pause();  // Sleep until a signal is received
        // --- Acquire Semaphore ---
        if (semop(sem_id, &lock, 1) == -1)
        {
            perror("DC semop failed");
            break;
        }

        // Determine how many characters are available to read
        int available_to_read = (shm_ptr_global->write_index - shm_ptr_global->read_index + BUFFER) % BUFFER;

        // read all that can be read
        for(int i = 0; i < available_to_read; i++)
        {
            char c = shm_ptr_global->buffer[shm_ptr_global->read_index];
            if (c >= 'A' && c <= 'T') {
                letter_counts[c - 'A']++;
            }            
            shm_ptr_global->read_index = (shm_ptr_global->read_index + 1) % BUFFER;
        }

        // Unlock semaphore
        if (semop(sem_id, &unlock, 1) == -1)
        {
            perror("DC semop failed");
            break;
        }

        if(display_flag)
        {
            display_histogram();
            display_flag = 0;
        }

    }

    // --- Acquire Semaphore ---
        if (semop(sem_id, &lock, 1) == 0)
        {
           // Determine how many characters are available to read
        int available_to_read = (shm_ptr_global->write_index - shm_ptr_global->read_index + BUFFER) % BUFFER;

        // read all that can be read
        for(int i = 0; i < available_to_read; i++)
        {
            char c = shm_ptr_global->buffer[shm_ptr_global->read_index];
            if (c >= 'A' && c <= 'T') {
                letter_counts[c - 'A']++;
            }            
            shm_ptr_global->read_index = (shm_ptr_global->read_index + 1) % BUFFER;
        }
        
        // Release lock (optional, we are removing it anyway)
         semop(sem_id, &unlock, 1);
        }

        


    // displays final histogram
    display_histogram();

    // Cleanup IPC
    shmdt(shm_ptr_global);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    //  exits with "Shazam!!".
    printf("Shazam!!\n");
    

    return 0;
}
