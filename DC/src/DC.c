

//store the sharedMemoryID, DP-1’s processID and DP-2’s processID from the command line arguments


// then attach itself to the shared memory



//SIGINT in DC triggers cleanup: sends SIGINT to DPs, reads remaining data, displays final histogram, and exits with "Shazam!!".
//DPs detach from shared memory and exit on SIGINT

//Display every 10 seconds with symbols: - for ones place, + for tens place, * for hundreds place#include <stdio.h>
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

int main(int argc, char *argv[]) {

for (int i = 0; i < argc; i++) {
        // Print each argument received by main
        printf("  argv[%d]: %s\n", i, argv[i]);
    }

    if (argc != 4) {
        // Print error to standard error
        fprintf(stderr, "DC Error: Incorrect number of arguments.\n");
        fprintf(stderr, "Usage: %s <shared_memory_id> <dp1_pid> <dp2_pid>\n", argv[0]);
        // Exit indicating failure
        exit(EXIT_FAILURE);
    }

    int shm_id = atoi(argv[1]);
    pid_t dp1_pid = (pid_t)atoi(argv[2]); // Cast result of atoi to pid_t
    pid_t dp2_pid = (pid_t)atoi(argv[3]); // Cast result of atoi to pid_t

     return 0; 
}
