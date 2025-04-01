

//store the sharedMemoryID, DP-1’s processID and DP-2’s processID from the command line arguments


// then attach itself to the shared memory



//SIGINT in DC triggers cleanup: sends SIGINT to DPs, reads remaining data, displays final histogram, and exits with "Shazam!!".
//DPs detach from shared memory and exit on SIGINT

//Display every 10 seconds with symbols: - for ones place, + for tens place, * for hundreds place