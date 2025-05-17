
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NAME_LEN 16
#define MAX_PROCESSES 100
Process processes[MAX_PROCESSES];

// ------------------ 구조체 정의 ------------------

//process 구조체
typedef struct {
    int pid;                  
    int arrival_time;         
    int cpu_burst_time;       
    int io_burst_time;        
    int io_request_time;      
    int priority;    

    //for simulation
    int remaining_time;
    int waiting_time;
    int turnaround_time;
    int is_completed;

    int is_waiting_io;
    int io_remaining_time;
} Process;

//queue 구조체
typedef struct {
    Process** data;
    int front;
    int rear;
    int capacity;
} Queue;

//system config 구조체
typedef struct {
    Queue* readyQueue;
    Queue* waitingQueue;
} SystemConfig;

// ------------------ Queue 함수 ------------------

Queue* createQueue(int capacity) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->data = (Process**)malloc(sizeof(Process*) * capacity);
    q->front = q->rear = 0;
    q->capacity = capacity;
    return q;
}

void destroyQueue(Queue* q) {
    free(q->data);
    free(q);
}

int isQueueEmpty(Queue* q) {
    return q->front == q->rear;
}

void enqueue(Queue* q, Process* p) {
    if (q->rear < q->capacity) {
        q->data[q->rear++] = p;
    }
}

Process* dequeue(Queue* q) {
    if (isQueueEmpty(q)) return NULL;
    return q->data[q->front++];
}

// --------------------- config --------------------------

SystemConfig* Config(int max_processes) {
    SystemConfig* config = (SystemConfig*)malloc(sizeof(SystemConfig));
    config->readyQueue = createQueue(max_processes);
    config->waitingQueue = createQueue(max_processes);
    return config;
}

// ------------------ Process create and print ------------------

Process* Create_Process(int n) {

    Process* plist = (Process*)malloc(sizeof(Process) * n);
    srand(time(NULL));

    for (int i = 0; i < n; i++) {
        processes[i].pid = i + 1;
        processes[i].arrival_time = rand() % 10; // 1~ 10
        processes[i].cpu_burst_time = 1 + rand() % 20; // 1~20
        processes[i].io_burst_time = 1 + rand() % 5; // 1 ~ 5
        processes[i].io_request_time = rand() % processes[i].cpu_burst_time;
        processes[i].priority = 1 + rand() % 5;

        processes[i].remaining_time = processes[i].cpu_burst_time;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].is_completed = 0;
        processes[i].is_waiting_io = 0;
        processes[i].io_remaining_time = 0;
    }

    return plist;
}

Process* clone_process_list(Process* original, int n) {
    Process* copy = (Process*)malloc(sizeof(Process) * n);
    for (int i = 0; i < n; i++) {
        copy[i] = original[i];  // 구조체 복사 (얕은 복사로 충분)
    }
    return copy;
}

void Print_Processes(Process* plist, int n) {
    printf("PID\tArrive\tCPU\tI/O\tI/O@ \tPrio\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n",
            plist[i].pid,
            plist[i].arrival_time,
            plist[i].cpu_burst_time,
            plist[i].io_burst_time,
            plist[i].io_request_time,
            plist[i].priority);
    }
}

//------------------- Evaluation fuction -----------------------
void Evaluation(Process* plist, int n, const char* name) {
    float total_wt = 0.0f, total_tt = 0.0f;

    for (int i = 0; i < n; i++) {
        total_wt += plist[i].waiting_time;
        total_tt += plist[i].turnaround_time;
    }

    float avg_wt = total_wt / n;
    float avg_tt = total_tt / n;

    printf("\n Evaluation for [%s]\n", name);
    printf("Average Waiting Time: %.2f\n", avg_wt);
    printf("Average Turnaround Time: %.2f\n", avg_tt);
}

// ------------------ FCFS ----------------------------

void FCFS(Process* plist, int n, SystemConfig* cfg) {

    printf("\n=== FCFS Scheduling ===\n");

    Process* running = NULL;

    int current_time = 0;
    int completed = 0;
    int run_time = 0;


    while (completed < n) {

        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time) {
                enqueue(cfg->readyQueue, &plist[i]);
            }
        }

        int waiting_size = cfg->waitingQueue->rear - cfg->waitingQueue->front;
        for (int i = 0; i < waiting_size; i++) {
            Process* p = dequeue(cfg->waitingQueue);
            p->io_remaining_time--;
            if (p->io_remaining_time <= 0) {
                p->is_waiting_io = 0;
                enqueue(cfg->readyQueue, p);
            }
            else {
                enqueue(cfg->waitingQueue, p);
            }
        }

        if (!running && !isQueueEmpty(cfg->readyQueue)) {
            running = dequeue(cfg->readyQueue);
            run_time = 0;
        }

        if (running) {
            running->remaining_time--;
            run_time++;

            if (run_time == running->io_request_time && running->io_burst_time > 0) {
                running->is_waiting_io = 1;
                running->io_remaining_time = running->io_burst_time;
                enqueue(cfg->waitingQueue, running);
                running = NULL;
            }
            else if (running->remaining_time <= 0) {
                running->turnaround_time = current_time + 1 - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                running->is_completed = 1;
                completed++;
                running = NULL;
            }
        }

        current_time++;
    }
}

// ------------------ SJF ------------------
void SJF(Process* plist, int n, SystemConfig* cfg) {
    int current_time = 0;
    int completed = 0;
    Process* running = NULL;

    printf("\n=== SJF (Non-Preemptive) Scheduling ===\n");

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && !plist[i].is_completed) {
                enqueue(cfg->readyQueue, &plist[i]);
            }
        }

        int waiting_size = cfg->waitingQueue->rear - cfg->waitingQueue->front;
        for (int i = 0; i < waiting_size; i++) {
            Process* p = dequeue(cfg->waitingQueue);
            p->io_remaining_time--;
            if (p->io_remaining_time <= 0) {
                p->is_waiting_io = 0;
                enqueue(cfg->readyQueue, p);
            }
            else {
                enqueue(cfg->waitingQueue, p);
            }
        }

        if (!running && !isQueueEmpty(cfg->readyQueue)) {
            // 최소 burst time 프로세스 선택
            int min_idx = cfg->readyQueue->front;
            for (int i = cfg->readyQueue->front + 1; i < cfg->readyQueue->rear; i++) {
                if (cfg->readyQueue->data[i]->cpu_burst_time < cfg->readyQueue->data[min_idx]->cpu_burst_time) {
                    min_idx = i;
                }
            }

            // 해당 프로세스 꺼냄
            Process* selected = cfg->readyQueue->data[min_idx];
            for (int i = min_idx; i < cfg->readyQueue->rear - 1; i++) {
                cfg->readyQueue->data[i] = cfg->readyQueue->data[i + 1];
            }
            cfg->readyQueue->rear--;

            running = selected;
        }

        if (running) {
            printf("Time %d: P%d is running\n", current_time, running->pid);
            running->remaining_time--;

            if (running->remaining_time == 0) {
                running->turnaround_time = current_time + 1 - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                running->is_completed = 1;
                completed++;
                running = NULL;
            }
        }
        else {
            printf("Time %d: CPU is idle\n", current_time);
        }

        current_time++;
    }
}

//------------------- Preemptive SJF ---------------------
void SJF_Preemptive(Process* plist, int n, SystemConfig* cfg) {
    int current_time = 0;
    int completed = 0;
    Process* running = NULL;

    printf("\n=== SJF (Preemptive) Scheduling ===\n");

    while (completed < n) {
        // 도착한 프로세스 Ready Queue에 추가
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && !plist[i].is_completed) {
                enqueue(cfg->readyQueue, &plist[i]);
            }
        }

        // I/O 처리
        int waiting_size = cfg->waitingQueue->rear - cfg->waitingQueue->front;
        for (int i = 0; i < waiting_size; i++) {
            Process* p = dequeue(cfg->waitingQueue);
            p->io_remaining_time--;
            if (p->io_remaining_time <= 0) {
                p->is_waiting_io = 0;
                enqueue(cfg->readyQueue, p);
            }
            else {
                enqueue(cfg->waitingQueue, p);
            }
        }

        // 가장 남은 시간이 짧은 프로세스 선택
        Process* shortest = NULL;
        int shortest_idx = -1;
        for (int i = cfg->readyQueue->front; i < cfg->readyQueue->rear; i++) {
            Process* p = cfg->readyQueue->data[i];
            if (!p->is_waiting_io && !p->is_completed) {
                if (!shortest || p->remaining_time < shortest->remaining_time) {
                    shortest = p;
                    shortest_idx = i;
                }
            }
        }

        if (shortest && (running == NULL || shortest->remaining_time < running->remaining_time)) {
            running = shortest;
        }

        if (running) {
            printf("Time %d: P%d is running (remaining: %d)\n", current_time, running->pid, running->remaining_time);
            running->remaining_time--;

            // I/O 요청
            if (running->remaining_time > 0 && running->cpu_burst_time - running->remaining_time == running->io_request_time) {
                running->is_waiting_io = 1;
                running->io_remaining_time = running->io_burst_time;
                enqueue(cfg->waitingQueue, running);
                running = NULL;
            }
            // 완료
            else if (running->remaining_time == 0) {
                running->turnaround_time = current_time + 1 - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                running->is_completed = 1;
                completed++;
                running = NULL;
            }
        }
        else {
            printf("Time %d: CPU is idle\n", current_time);
        }

        current_time++;
    }

}

//------------------- Nonpreemptive Priority ------------------
void Priority_NonPreemptive(Process* plist, int n, SystemConfig* cfg) {
    int current_time = 0;
    int completed = 0;
    Process* running = NULL;

    printf("\n=== Priority (Non-Preemptive) Scheduling ===\n");

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && !plist[i].is_completed) {
                enqueue(cfg->readyQueue, &plist[i]);
            }
        }

        // I/O 처리
        int waiting_size = cfg->waitingQueue->rear - cfg->waitingQueue->front;
        for (int i = 0; i < waiting_size; i++) {
            Process* p = dequeue(cfg->waitingQueue);
            p->io_remaining_time--;
            if (p->io_remaining_time <= 0) {
                p->is_waiting_io = 0;
                enqueue(cfg->readyQueue, p);
            }
            else {
                enqueue(cfg->waitingQueue, p);
            }
        }

        if (!running && !isQueueEmpty(cfg->readyQueue)) {
            // 가장 높은 priority 선택 (숫자가 작을수록 우선)
            int min_idx = cfg->readyQueue->front;
            for (int i = cfg->readyQueue->front + 1; i < cfg->readyQueue->rear; i++) {
                if (cfg->readyQueue->data[i]->priority < cfg->readyQueue->data[min_idx]->priority) {
                    min_idx = i;
                }
            }

            running = cfg->readyQueue->data[min_idx];
            for (int i = min_idx; i < cfg->readyQueue->rear - 1; i++) {
                cfg->readyQueue->data[i] = cfg->readyQueue->data[i + 1];
            }
            cfg->readyQueue->rear--;
        }

        if (running) {
            printf("Time %d: P%d is running\n", current_time, running->pid);
            running->remaining_time--;

            if (running->remaining_time > 0 &&
                running->cpu_burst_time - running->remaining_time == running->io_request_time) {
                running->is_waiting_io = 1;
                running->io_remaining_time = running->io_burst_time;
                enqueue(cfg->waitingQueue, running);
                running = NULL;
            }
            else if (running->remaining_time == 0) {
                running->turnaround_time = current_time + 1 - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                running->is_completed = 1;
                completed++;
                running = NULL;
            }
        }
        else {
            printf("Time %d: CPU is idle\n", current_time);
        }

        current_time++;
    }
}

//------------------- Preemptive Priority -----------------
void Priority_Preemptive(Process* plist, int n, SystemConfig* cfg) {
    int current_time = 0;
    int completed = 0;
    Process* running = NULL;

    printf("\n=== Priority (Preemptive) Scheduling ===\n");

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && !plist[i].is_completed) {
                enqueue(cfg->readyQueue, &plist[i]);
            }
        }

        // I/O 처리
        int waiting_size = cfg->waitingQueue->rear - cfg->waitingQueue->front;
        for (int i = 0; i < waiting_size; i++) {
            Process* p = dequeue(cfg->waitingQueue);
            p->io_remaining_time--;
            if (p->io_remaining_time <= 0) {
                p->is_waiting_io = 0;
                enqueue(cfg->readyQueue, p);
            }
            else {
                enqueue(cfg->waitingQueue, p);
            }
        }

        // 가장 높은 priority (숫자 작을수록 높음) 선택
        Process* top = NULL;
        for (int i = cfg->readyQueue->front; i < cfg->readyQueue->rear; i++) {
            Process* p = cfg->readyQueue->data[i];
            if (!p->is_completed && !p->is_waiting_io) {
                if (!top || p->priority < top->priority) {
                    top = p;
                }
            }
        }

        if (top && (running == NULL || top->priority < running->priority)) {
            running = top;  // 선점 발생 가능
        }

        if (running) {
            printf("Time %d: P%d is running (priority: %d)\n", current_time, running->pid, running->priority);
            running->remaining_time--;

            if (running->remaining_time > 0 &&
                running->cpu_burst_time - running->remaining_time == running->io_request_time) {
                running->is_waiting_io = 1;
                running->io_remaining_time = running->io_burst_time;
                enqueue(cfg->waitingQueue, running);
                running = NULL;
            }
            else if (running->remaining_time == 0) {
                running->turnaround_time = current_time + 1 - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                running->is_completed = 1;
                completed++;
                running = NULL;
            }
        }
        else {
            printf("Time %d: CPU is idle\n", current_time);
        }

        current_time++;
    }
}

//----------------- Round Robin ---------------------
void RoundRobin(Process* plist, int n, SystemConfig* cfg, int time_quantum) {
    int current_time = 0;
    int completed = 0;
    Process* running = NULL;
    int time_slice = 0;

    printf("\n=== Round Robin Scheduling (TQ = %d) ===\n", time_quantum);

    while (completed < n) {
        // 프로세스 도착
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && !plist[i].is_completed) {
                enqueue(cfg->readyQueue, &plist[i]);
            }
        }

        // I/O 처리
        int waiting_size = cfg->waitingQueue->rear - cfg->waitingQueue->front;
        for (int i = 0; i < waiting_size; i++) {
            Process* p = dequeue(cfg->waitingQueue);
            p->io_remaining_time--;
            if (p->io_remaining_time <= 0) {
                p->is_waiting_io = 0;
                enqueue(cfg->readyQueue, p);
            }
            else {
                enqueue(cfg->waitingQueue, p);
            }
        }

        if (!running && !isQueueEmpty(cfg->readyQueue)) {
            running = dequeue(cfg->readyQueue);
            time_slice = 0;
        }

        if (running) {
            printf("Time %d: P%d is running (remaining: %d)\n", current_time, running->pid, running->remaining_time);
            running->remaining_time--;
            time_slice++;

            // I/O 요청 도달
            if (running->cpu_burst_time - running->remaining_time == running->io_request_time &&
                running->io_burst_time > 0) {
                running->is_waiting_io = 1;
                running->io_remaining_time = running->io_burst_time;
                enqueue(cfg->waitingQueue, running);
                running = NULL;
            }
            // 프로세스 완료
            else if (running->remaining_time == 0) {
                running->turnaround_time = current_time + 1 - running->arrival_time;
                running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                running->is_completed = 1;
                completed++;
                running = NULL;
            }
            // 타임 슬라이스 만료 → 뒤로 보내기
            else if (time_slice == time_quantum) {
                enqueue(cfg->readyQueue, running);
                running = NULL;
            }
        }
        else {
            printf("Time %d: CPU is idle\n", current_time);
        }

        current_time++;
    }
}


// ------------------ main ------------------
int main() {
    int n = 5;
    Process* original = Create_Process(n);
    Print_Processes(original, n);

    // FCFS
    Process* plist1 = clone_process_list(original, n);
    SystemConfig* cfg1 = Config(n);
    FCFS(plist1, n, cfg1);
    Evaluation(plist1, n, "FCFS");
    DestroyConfig(cfg1);
    free(plist1);

    // SJF (Non-preemptive)
    Process* plist2 = clone_process_list(original, n);
    SystemConfig* cfg2 = Config(n);
    SJF(plist2, n, cfg2);
    Evaluation(plist2, n, "SJF (Non-Preemptive)");
    DestroyConfig(cfg2);
    free(plist2);

    // SJF (Preemptive)
    Process* plist3 = clone_process_list(original, n);
    SystemConfig* cfg3 = Config(n);
    SJF_Preemptive(plist3, n, cfg3);
    Evaluation(plist3, n, "SJF (Preemptive)");
    DestroyConfig(cfg3);
    free(plist3);

    // Non-Preemptive Priority
    Process* plist4 = clone_process_list(original, n);
    SystemConfig* cfg4 = Config(n);
    Priority_NonPreemptive(plist4, n, cfg4);
    Evaluation(plist4, n, "Priority (Non-Preemptive)");
    DestroyConfig(cfg4);
    free(plist4);

    // Preemptive Priority
    Process* plist5 = clone_process_list(original, n);
    SystemConfig* cfg5 = Config(n);
    Priority_Preemptive(plist5, n, cfg5);
    Evaluation(plist5, n, "Priority (Preemptive)");
    DestroyConfig(cfg5);
    free(plist5);

    // Round Robin (TQ = 3)
    Process* plist6 = clone_process_list(original, n);
    SystemConfig* cfg6 = Config(n);
    RoundRobin(plist6, n, cfg6, 3);
    Evaluation(plist6, n, "Round Robin (TQ = 3)");
    DestroyConfig(cfg6);
    free(plist6);


    free(original);
    return 0;
}

