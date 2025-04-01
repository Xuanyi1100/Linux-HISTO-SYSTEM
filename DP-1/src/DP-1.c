#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>  // For semaphores
#include <errno.h>    // For error handling

typedef struct {
    char buffer[256];  // Circular buffer for characters 'A'-'T'
    int write_index;   // Current write position (4 bytes)
    int read_index;    // Current read position (4 bytes)
} shared_memory;

union semun {
    int val;               // Value for SETVAL
    struct semid_ds *buf;  // Buffer for IPC_STAT, IPC_SET
    unsigned short *array; // Array for GETALL, SETALL
};

int create_semaphore() {
    key_t sem_key = ftok(".", 'M');  // Different key from shared memory
    int sem_id = semget(sem_key, 1, IPC_CREAT | 0666);  // 1 semaphore
    if (sem_id == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize semaphore value to 1 (unlocked)
    union semun arg;
    arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        perror("semctl (SETVAL) failed");
        exit(1);
    }

    printf("Semaphore created and initialized (ID: %d)\n", sem_id);
    return sem_id;
}

int create_or_attach_shm() {
    key_t key = ftok(".", 'S');
    if (key == -1) {
        perror("ftok failed");
        exit(1);
    }

    // Check if shared memory exists
    int shm_id = shmget(key, sizeof(shared_memory), 0666);
    if (shm_id != -1) {
        printf("Attached to existing shared memory (ID: %d)\n", shm_id);
        shared_memory *shm = (shared_memory *)shmat(shm_id, NULL, 0);
        if (shm == (void *)-1) {
            perror("shmat failed");
            exit(1);
        }
        return shm_id;
    }

    // Create new shared memory
    shm_id = shmget(key, sizeof(shared_memory), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget (creation) failed");
        exit(1);
    }

    // Initialize
    shared_memory *shm = (shared_memory *)shmat(shm_id, NULL, 0);
    shm->write_index = 0;
    shm->read_index = 0;
    memset(shm->buffer, 0, 256);
    printf("Created new shared memory (ID: %d)\n", shm_id);
    return shm_id;
}

void detach_shared_memory(shared_memory *shm) {
    if (shmdt(shm) == -1) {
        perror("shmdt failed");
    }
}

void delete_shared_memory(int shm_id) {
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl (IPC_RMID) failed");
    }
}

// dp_1 start dp_2 by fork(), command liine arg is shareMemoryID
  
// the write index needs to wrap around when reach 256
// make sure never never write past the read index ( used by DC )
// if DP is about to overtake to the read-index, go into sleep mode

// after DP-2 attched to the sm, usiing semaphore to write to the sharedMemory
// 20 chars then sleep 2 seconds, remember to check space left enough for 20 or not

int main() {
    // Create shared memory
    int shm_id = create_or_attach_shm();
    // attaching shared memory to get the pointer
    shared_memory *shm = (shared_memory *)shmat(shm_id, NULL, 0);

    // Create semaphore
    int sem_id = create_semaphore();

    printf("Shared memory created. shm_id = %d\n", shm_id);
    printf("Buffer starts at %p, write_index = %d, read_index = %d\n", 
           shm->buffer, shm->write_index, shm->read_index);

    
    // Fork DP-2 and pass both shm_id and sem_id
    pid_t pid = fork();
    if (pid == 0) {
        // Child process (DP-2)
        char shm_str[20];
        sprintf(shm_str, "%d", shm_id);
        execl("../../DP-2/bin/DP-2", "DP-2", shm_str, NULL);
        perror("execl failed");
        exit(1);
    } else if (pid > 0) {
        // Parent (DP-1 continues)
        printf("DP-2 launched with PID %d\n", pid);
    } else {
        perror("fork failed");
        exit(1);
    }

    // Cleanup 
    /*
    semctl(sem_id, 0, IPC_RMID);  // Delete semaphore
    detach_shared_memory(shm);
    delete_shared_memory(shm_id);

    */
    return 0;
}