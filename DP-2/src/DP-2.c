



// get PID of itself and DP-1


// launch DC through the use of a fork() call, 
// pass it’s PID, DP-1 PID , sharedMemoryID  as command line arguments to DC


//Only after forking and launching the DC application will DP-2 attach itself to the block of shared memory


// 1 char then sleep 1/20 second, remember to check space left enough for 1



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Print received arguments
    printf("DP-2 started!\n");
    printf("Arguments received:\n");
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    // Validate argument count
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <shm_id>:%s \n", argv[0],argv[1]);
        exit(1);
    }



    return 0;
}
