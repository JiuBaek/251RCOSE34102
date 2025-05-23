
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NAME_LEN 16
#define MAX_PROCESSES 100
#define MAX_TIME 1000
int gantt_chart[MAX_TIME];
int gantt_time = 0;

// ------------------ 구조체 정의 ------------------

//process 구조체
typedef struct {
    int pid;                  
    int arrival_time;         
    int cpu_burst_time;       
    int io_burst_time;        
    int* io_request_times; //언제 I/O 가 실행될 것 인가 (다중 시도 중, 1~3회)
    int io_request_len;
    int io_remaining_time;
    int priority;    

    //for simulation
    int remaining_time;
    int waiting_time; // eval에 사용
    int turnaround_time; // eval에 사용

    int is_waiting_io; //지금 io 진행중인가
} Process;
Process processes[MAX_PROCESSES];

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
    int nextRear = (q->rear + 1) % q->capacity;
    if (nextRear != q->front) {
        q->data[q->rear] = p;
        q->rear = nextRear;
    }
    else {
        printf("Queue is full, cannot enqueue P%d\n", p->pid);
    }
}

Process* dequeue(Queue* q) {
    if (isQueueEmpty(q)) return NULL;
    Process* p = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    return p;
}

// --------------------- config --------------------------

SystemConfig* Config(int max_processes) {
    SystemConfig* config = (SystemConfig*)malloc(sizeof(SystemConfig));
    config->readyQueue = createQueue(max_processes);
    config->waitingQueue = createQueue(max_processes);

    return config;
}

void DestroyConfig(SystemConfig* cfg) {
    destroyQueue(cfg->readyQueue);
    destroyQueue(cfg->waitingQueue);
    free(cfg);
}

// ------------------ Process create and print ------------------

Process* Create_Process(int n) {

    Process* plist = (Process*)malloc(sizeof(Process) * n);
    srand(time(NULL));

    for (int i = 0; i < n; i++) {
        plist[i].pid = i + 1;
        plist[i].arrival_time = rand() % 10;
        plist[i].cpu_burst_time = 3 + rand() % 18; // 3 ~ 20
        plist[i].io_burst_time = 1 + rand() % 5;
        plist[i].priority = 1 + rand() % 5;

        //io 정하기
        plist[i].io_remaining_time = plist[i].io_burst_time;
        int io_count = 1 + rand() % 3;  // 1~3회
        plist[i].io_request_len = io_count;
        plist[i].io_request_times = malloc(sizeof(int) * io_count);

        int max_time = plist[i].cpu_burst_time - 1;
        int io_burst = 3;
        plist[i].io_burst_time = io_burst;

        int prev_time = 0;

        int j;
        for (j = 0; j < io_count; j++) {
            int earliest = prev_time + io_burst;
            if (earliest >= max_time) break; 

            int t = earliest + rand() % (max_time - earliest + 1);
            plist[i].io_request_times[j] = t;
            prev_time = t;
        }

        plist[i].io_request_len = j;
        
        //오름차순
        for (int a = 0; a < io_count - 1; a++) {
            for (int b = a + 1; b < io_count; b++) {
                if (plist[i].io_request_times[a] > plist[i].io_request_times[b]) {
                    int tmp = plist[i].io_request_times[a];
                    plist[i].io_request_times[a] = plist[i].io_request_times[b];
                    plist[i].io_request_times[b] = tmp;
                }
            }
        }

        //simulation
        plist[i].remaining_time = plist[i].cpu_burst_time;
        plist[i].waiting_time = 0;
        plist[i].turnaround_time = 0;
        plist[i].is_waiting_io = 0;
    }

    return plist;
}

Process* clone_process_list(Process* original, int n) {
    Process* copy = (Process*)malloc(sizeof(Process) * n);
    for (int i = 0; i < n; i++) {
        copy[i] = original[i];
    }
    return copy;
}

void Print_Processes(Process* plist, int n) {

    printf("PID\tArrival\tCPU\tIO\tIO_Req_Times\tPriority\n");

    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t",
            plist[i].pid,
            plist[i].arrival_time,
            plist[i].cpu_burst_time,
            plist[i].io_burst_time);

        printf("[");
        for (int j = 0; j < plist[i].io_request_len; j++) {
            printf("%d", plist[i].io_request_times[j]);
            if (j < plist[i].io_request_len - 1) printf(", ");
        }
        printf("]");

        printf("\t%d\n", plist[i].priority);
    }
}

//--------------------- Gantt Chart --------------------------
void PrintGanttChart(int* chart, int time) {
    printf("\nGantt Chart:\n");

    for (int i = 0; i < time; i++) {
        if (chart[i] == 0)
            printf("|Idle ");
        else
            printf("| P%d ", chart[i]);
    }
    printf("|\n");

    for (int i = 0; i <= time; i++) {
        printf("%5d", i);
    }
    printf("\n");
}

//------------------- Evaluation fuction -----------------------
void Evaluation(Process* plist, int n, const char* name) {
    if (n == 0) return;

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

//------------------ I/O --------------------------- FCFS, 
int HandleIORequest(Process** running_ptr, SystemConfig* cfg, int current_time) {
    Process* p = *running_ptr;

    int executed = p->cpu_burst_time - p->remaining_time;

    // IO 요청이 남아 있는지 확인
    if (p->io_request_len > 0 && p->io_request_times[0] == executed) {

        p->is_waiting_io = 1;
        p->io_remaining_time = p->io_burst_time;

        enqueue(cfg->waitingQueue, p);
        *running_ptr = NULL;

        for (int i = 1; i < p->io_request_len; i++) {
            p->io_request_times[i - 1] = p->io_request_times[i];
        }
        p->io_request_len--;

        if (p->io_request_len == 0) {
            free(p->io_request_times);
            p->io_request_times = NULL;
        }

        return 1;
    }

    return 0;
}

void ProcessIO(SystemConfig* cfg, int current_time) {
    int queue_size = (cfg->waitingQueue->rear - cfg->waitingQueue->front + cfg->waitingQueue->capacity) % cfg->waitingQueue->capacity;
    
    for (int i = 0; i < queue_size; i++) {
        Process* p = dequeue(cfg->waitingQueue);

        if (p) {
            p->io_remaining_time--;
            printf("Time %d: P%d is in I/O  \n", current_time, p->pid);

            if (p->io_remaining_time <= 0) {
                printf("Time %d: P%d completed I/O and re-entered Ready Queue\n", current_time, p->pid);
                p->is_waiting_io = 0;
                enqueue(cfg->readyQueue, p);
            } else {
                enqueue(cfg->waitingQueue, p);
            }
        }
    }
}


// ------------------ FCFS ----------------------------

void FCFS(Process* plist, int n, SystemConfig* cfg) {

    printf("\n FCFS \n");

    Process* running = NULL;

    int current_time = 0;
    int completed = 0;
    int run_time = 0;


    while (completed < n) { // n은 plist 프로세스 개수

        // 도착한 순서대로 readyqueue에 정렬
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && plist[i].remaining_time > 0) {
                //printf("Time %d: Enqueuing P%d\n", current_time, plist[i].pid);
                enqueue(cfg->readyQueue, &plist[i]);
            }
        }

        // I/O 처리
        ProcessIO(cfg, current_time);

        //실행할 프로세스 선택 (레디큐에서 꺼내옴)
        if ((running == NULL) && !isQueueEmpty(cfg->readyQueue)) {
            running = dequeue(cfg->readyQueue);
        }

        // 실행중인 프로세스 처리
        if (running) {
            //printf("Time %d: P%d is running\n", current_time, running->pid);
            running->remaining_time--;
            gantt_chart[current_time] = running->pid;

            if (!HandleIORequest(&running, cfg, current_time)) {
                if (running && running->remaining_time <= 0) {
                    running->turnaround_time = current_time + 1 - running->arrival_time;
                    running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                    completed++;
                    //printf("Time %d: P%d completed\n", current_time, running->pid);
                    running = NULL;
                }
            }
        }
        else {
            //printf("Time %d: CPU is idle\n", current_time)
                gantt_chart[current_time] = 0;
        }

        current_time++;
    }
    gantt_time = current_time;

    PrintGanttChart(gantt_chart, gantt_time);
    return;
}

// ------------------ SJF (Non-Preemptive) ------------------
void SJF(Process* plist, int n, SystemConfig* cfg) {
    int current_time = 0;
    int completed = 0;
    Process* running = NULL;

    printf("\n SJF (Non-Preemptive) \n");

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && plist[i].remaining_time > 0) {
                enqueue(cfg->readyQueue, &plist[i]);
               //printf("Time %d: Enqueuing P%d\n", current_time, plist[i].pid);
            }
        }

        // I/O 처리
        ProcessIO(cfg, current_time);

        if (!running && !isQueueEmpty(cfg->readyQueue)) {
            // 최소 burst time 프로세스 선택
            int min_idx = cfg->readyQueue->front;
            for (int i = cfg->readyQueue->front + 1; i < cfg->readyQueue->rear; i++) {
                if (cfg->readyQueue->data[i]->cpu_burst_time < cfg->readyQueue->data[min_idx]->cpu_burst_time) {
                    min_idx = i;
                }
            }

            Process* selected = cfg->readyQueue->data[min_idx];
            for (int i = min_idx; i < cfg->readyQueue->rear - 1; i++) {
                cfg->readyQueue->data[i] = cfg->readyQueue->data[i + 1];
            }
            cfg->readyQueue->rear--;

            running = selected;
        }

        if (running) {
            //printf("Time %d: P%d is running\n", current_time, running->pid);
            running->remaining_time--;
            gantt_chart[current_time] = running->pid;

            int executed = running->cpu_burst_time - running->remaining_time;

            if (!HandleIORequest(&running, cfg, current_time)) {
                if (running && running->remaining_time <= 0) {
                    running->turnaround_time = current_time + 1 - running->arrival_time;
                    running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                    completed++;
                    //printf("Time %d: P%d completed\n", current_time, running->pid);
                    running = NULL;
                }
            }
        }
        else {
            //printf("Time %d: CPU is idle\n", current_time);
            gantt_chart[current_time] = 0;
        }

        current_time++;
    }
    gantt_time = current_time;
    PrintGanttChart(gantt_chart, gantt_time);
}

//------------------- Preemptive SJF ---------------------
void SJF_Preemptive(Process* plist, int n, SystemConfig* cfg) {
    int current_time = 0;
    int completed = 0;
    Process* running = NULL;

    printf("\n SJF (Preemptive) \n");

    while (completed < n) {
        // 도착한 프로세스 Ready Queue에 추가
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && plist[i].remaining_time > 0) {
                enqueue(cfg->readyQueue, &plist[i]);
               // printf("Time %d: Enqueuing P%d\n", current_time, plist[i].pid);
            }
        }

        // I/O 처리
        ProcessIO(cfg, current_time);

        // 가장 남은 시간이 짧은 프로세스 선택
        Process* shortest = NULL;
        int shortest_idx = -1;
        for (int i = cfg->readyQueue->front; i < cfg->readyQueue->rear; i++) {
            Process* p = cfg->readyQueue->data[i];
            if (!p->is_waiting_io && p->remaining_time > 0) {
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
            //printf("Time %d: P%d is running (remaining: %d)\n", current_time, running->pid, running->remaining_time);
            running->remaining_time--;
            gantt_chart[current_time] = running->pid;

            if (!HandleIORequest(&running, cfg, current_time)) {
                if (running && running->remaining_time <= 0) {
                    running->turnaround_time = current_time + 1 - running->arrival_time;
                    running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                    completed++;
                    //printf("Time %d: P%d completed\n", current_time, running->pid);
                    running = NULL;
                }
            }
        } else {
            //printf("Time %d: CPU is idle\n", current_time);
            gantt_chart[current_time] = 0;
        }

        current_time++;
    }
    gantt_time = current_time;
    PrintGanttChart(gantt_chart, gantt_time);

}

//------------------- Nonpreemptive Priority ------------------
void Priority_NonPreemptive(Process* plist, int n, SystemConfig* cfg) {
    printf("\n Priority (Non-Preemptive) \n");

    Process* running = NULL;

    int current_time = 0;
    int completed = 0;


    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && plist[i].remaining_time > 0) {
                enqueue(cfg->readyQueue, &plist[i]);
                //printf("Time %d: Enqueuing P%d\n", current_time, plist[i].pid);
            }
        }

        // I/O 처리
        ProcessIO(cfg, current_time);

        //highest priority 숫자 낮을 수록 우선순위 높음
        if (!running && !isQueueEmpty(cfg->readyQueue)) {
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
            //printf("Time %d: P%d is running\n", current_time, running->pid);
            running->remaining_time--;
            gantt_chart[current_time] = running->pid;

            if (!HandleIORequest(&running, cfg, current_time)) {
                if (running && running->remaining_time <= 0) {
                    running->turnaround_time = current_time + 1 - running->arrival_time;
                    running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                    completed++;
                    //printf("Time %d: P%d completed\n", current_time, running->pid);
                    running = NULL;
                }
            }
        } else {
            //printf("Time %d: CPU is idle\n", current_time);
            gantt_chart[current_time] = 0;
        }

        current_time++;
    }
    gantt_time = current_time;
    PrintGanttChart(gantt_chart, gantt_time);
}

//------------------- Preemptive Priority -----------------
void Priority_Preemptive(Process* plist, int n, SystemConfig* cfg) {
    printf("\n Priority (Preemptive) \n");

    Process* running = NULL;

    int current_time = 0;
    int completed = 0;


    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && plist[i].remaining_time > 0) {
                enqueue(cfg->readyQueue, &plist[i]);
                //printf("Time %d: Enqueuing P%d\n", current_time, plist[i].pid);
            }
        }

        // I/O 처리
        ProcessIO(cfg, current_time);


        // highest priority 숫자 낮을 수록 우선순위 높음
        Process* top = NULL;
        int top_idx = -1;
        for (int i = cfg->readyQueue->front; i < cfg->readyQueue->rear; i++) {
            Process* p = cfg->readyQueue->data[i];
            if (p->remaining_time > 0 && !p->is_waiting_io) {
                if (!top || p->priority < top->priority) {
                    top = p;
                    top_idx = i;
                }
            }
        }


        if (top && (running == NULL || top->priority < running->priority)) {

            if (running && top->priority < running->priority) {
                enqueue(cfg->readyQueue, running);
                //printf("Time %d: P%d preempted by P%d\n", current_time, running->pid, top->pid);
            }

            // ReadyQueue에서 top 제거
            for (int i = top_idx; i < cfg->readyQueue->rear - 1; i++) {
                cfg->readyQueue->data[i] = cfg->readyQueue->data[i + 1];
            }
            cfg->readyQueue->rear--;

            running = top;
        }

        if (running) {
            //printf("Time %d: P%d is running (priority: %d)\n", current_time, running->pid, running->priority);
            running->remaining_time--;
            gantt_chart[current_time] = running->pid;

            if (!HandleIORequest(&running, cfg, current_time)) {
                if (running && running->remaining_time <= 0) {
                    running->turnaround_time = current_time + 1 - running->arrival_time;
                    running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                    completed++;
                    //printf("Time %d: P%d completed\n", current_time, running->pid);
                    running = NULL;
                }
            }
        }
        else {
            //printf("Time %d: CPU is idle\n", current_time);
            gantt_chart[current_time] = 0;
        }

        current_time++;
    }
    gantt_time = current_time;
    PrintGanttChart(gantt_chart, gantt_time);
}

//----------------- Round Robin ---------------------
void RoundRobin(Process* plist, int n, SystemConfig* cfg, int time_quantum) {
    printf("\n Round Robin Scheduling (TQ = %d) \n", time_quantum);

    Process* running = NULL;

    int current_time = 0;
    int completed = 0;
    int time_slice = 0;


    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (plist[i].arrival_time == current_time && plist[i].remaining_time > 0) {
                enqueue(cfg->readyQueue, &plist[i]);
                //printf("Time %d: Enqueuing P%d\n", current_time, plist[i].pid);
            }
        }

        // I/O 처리
        ProcessIO(cfg, current_time);

        if (!running && !isQueueEmpty(cfg->readyQueue)) {
            running = dequeue(cfg->readyQueue);
            time_slice = 0;
        }

        if (running) {
            //printf("Time %d: P%d is running (remaining: %d)\n", current_time, running->pid, running->remaining_time);
            running->remaining_time--;
            time_slice++;
            gantt_chart[current_time] = running->pid;

            if (!HandleIORequest(&running, cfg, current_time)) {
                if (running && running->remaining_time <= 0) {
                    running->turnaround_time = current_time + 1 - running->arrival_time;
                    running->waiting_time = running->turnaround_time - running->cpu_burst_time;
                    completed++;
                    //printf("Time %d: P%d completed\n", current_time, running->pid);
                    running = NULL;
                }
            }
        }
        else {
            //printf("Time %d: CPU is idle\n", current_time);
            gantt_chart[current_time] = 0;
        }

        current_time++;
    }
    gantt_time = current_time;
    PrintGanttChart(gantt_chart, gantt_time);
}


// ------------------ main ------------------
int main() {
    int n = 3;
    Process* original = Create_Process(n);
    Print_Processes(original, n);

    // FCFS
    Process* plist1 = clone_process_list(original, n);
    SystemConfig* cfg1 = Config(MAX_PROCESSES);
    FCFS(plist1, n, cfg1);
    DestroyConfig(cfg1);

    // SJF (Non-preemptive)
    Process* plist2 = clone_process_list(original, n);
    SystemConfig* cfg2 = Config(MAX_PROCESSES);
    SJF(plist2, n, cfg2);
    DestroyConfig(cfg2);

    // SJF (Preemptive)
    Process* plist3 = clone_process_list(original, n);
    SystemConfig* cfg3 = Config(MAX_PROCESSES);
    SJF_Preemptive(plist3, n, cfg3);
    DestroyConfig(cfg3);

    // Non-Preemptive Priority
    Process* plist4 = clone_process_list(original, n);
    SystemConfig* cfg4 = Config(MAX_PROCESSES);
    Priority_NonPreemptive(plist4, n, cfg4);
    DestroyConfig(cfg4);

    // Preemptive Priority
    Process* plist5 = clone_process_list(original, n);
    SystemConfig* cfg5 = Config(MAX_PROCESSES);
    Priority_Preemptive(plist5, n, cfg5);
    DestroyConfig(cfg5);

    // Round Robin (TQ = 3)
    Process* plist6 = clone_process_list(original, n);
    SystemConfig* cfg6 = Config(MAX_PROCESSES);
    RoundRobin(plist6, n, cfg6, 3);
    DestroyConfig(cfg6);

    Evaluation(plist1, n, "FCFS");
    Evaluation(plist2, n, "SJF (Non-Preemptive)");
    Evaluation(plist3, n, "SJF (Preemptive)");
    Evaluation(plist4, n, "Priority (Non-Preemptive)");
    Evaluation(plist5, n, "Priority (Preemptive)");
    Evaluation(plist6, n, "Round Robin (TQ = 3)");

    free(plist1);
    free(plist2);
    free(plist3);
    free(plist4);
    free(plist5);
    free(plist6);


    free(original);
    return 0;
}

