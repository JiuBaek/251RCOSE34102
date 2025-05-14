
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NAME_LEN 16
#define MAX_PROCESSES 20

typedef struct {
    int pid;                  
    int arrival_time;         
    int cpu_burst_time;       
    int io_burst_time;        
    int io_request_time;      
    int priority;    
} Process;

Process processes[MAX_PROCESSES];

void Create_Process(int n) {
    srand(time(NULL));

    for (int i = 0; i < n; i++) {
        processes[i].pid = i + 1;
        processes[i].arrival_time = rand() % 10;
        processes[i].cpu_burst_time = 3 + rand() % 8; 
        processes[i].io_burst_time = rand() % 5;
        processes[i].io_request_time = rand() % processes[i].cpu_burst_time;
        processes[i].priority = 1 + rand() % 5;
    }
}

