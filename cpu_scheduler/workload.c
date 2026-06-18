#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "workload.h"

/* 입력된 I/O 이벤트를 CPU 사용 시점 순서대로 정렬한다. */
static void sort_io_events(PCB *p) {
    for (int i = 0; i < p->io_count - 1; i++) {
        for (int j = i + 1; j < p->io_count; j++) {
            if (p->io_start[j] < p->io_start[i]) {
                int tmp_start = p->io_start[i];
                int tmp_burst = p->io_burst[i];
                p->io_start[i] = p->io_start[j];
                p->io_burst[i] = p->io_burst[j];
                p->io_start[j] = tmp_start;
                p->io_burst[j] = tmp_burst;
            }
        }
    }
}

/* PCB 하나의 기본값과 실행 중 관리되는 상태값을 초기화한다. */
static void init_process(PCB *p, int pid, int arrival, int burst, int priority) {
    p->pid = pid;
    p->arrival_time = arrival;
    p->cpu_burst = burst;
    p->remaining_time = burst;
    p->priority = priority;

    p->io_count = 0;
    for (int i = 0; i < MAX_IO_EVENTS; i++) {
        p->io_start[i] = -1;
        p->io_burst[i] = 0;
    }
    p->next_io = 0;
    p->blocked_until = -1;
    p->total_io_time = 0;

    p->first_run_time = -1;
    p->finish_time = -1;

    p->waiting_time = 0;
    p->turnaround_time = 0;
    p->response_time = 0;

    p->completed = 0;
    p->state = STATE_NEW;
}

/* 같은 CPU 사용 지점에 I/O 이벤트가 중복 등록됐는지 검사한다. */
static int exists_io_start(PCB *p, int start) {
    for (int i = 0; i < p->io_count; i++) {
        if (p->io_start[i] == start) {
            return 1;
        }
    }
    return 0;
}

void create_random_workload(PCB process_list[], int *process_count) {
    /* 매 실행마다 다른 난수 workload가 나오도록 seed를 설정한다. */
    srand((unsigned int)time(NULL));

    printf("Input number of processes: ");
    if (scanf("%d", process_count) != 1) {
        *process_count = 0;
        return;
    }

    if (*process_count < 0) {
        *process_count = 0;
    }
    if (*process_count > MAX_PROCESS) {
        *process_count = MAX_PROCESS;
    }

    for (int i = 0; i < *process_count; i++) {
        /* 난수 workload의 도착 시간, CPU burst, 우선순위 범위를 정한다. */
        int arrival = rand() % 50;
        int burst = 5 + rand() % 16;
        int priority = 1 + rand() % 9;

        init_process(&process_list[i], i + 1, arrival, burst, priority);

        /* CPU burst 길이에 맞춰 가능한 I/O 이벤트 수를 제한한다. */
        int max_events = burst / 5;
        if (max_events > 3) {
            max_events = 3;
        }

        int io_count = 0;
        if (max_events > 0) {
            io_count = rand() % (max_events + 1);
        }

        process_list[i].io_count = io_count;
        for (int k = 0; k < io_count; k++) {
            int start;
            /* I/O는 CPU 실행 중간 시점에만 배치한다. */
            do {
                start = 1 + rand() % (burst - 1);
            } while (exists_io_start(&process_list[i], start));

            process_list[i].io_start[k] = start;
            process_list[i].io_burst[k] = 1 + rand() % 5;
            process_list[i].total_io_time += process_list[i].io_burst[k];
        }
        sort_io_events(&process_list[i]);
    }
}

void create_manual_workload(PCB process_list[], int *process_count) {
    printf("Input number of processes: ");
    if (scanf("%d", process_count) != 1) {
        *process_count = 0;
        return;
    }

    if (*process_count < 0) {
        *process_count = 0;
    }
    if (*process_count > MAX_PROCESS) {
        *process_count = MAX_PROCESS;
    }

    printf("\nInput format\n");
    printf("arrival_time cpu_burst priority io_count [io_start io_burst]...\n");
    printf("Example without I/O : 0 5 2 0\n");
    printf("Example with 2 I/O  : 0 12 3 2 4 3 9 2\n");
    printf("Meaning: after 4 CPU units, I/O 3; after 9 CPU units, I/O 2\n\n");

    for (int i = 0; i < *process_count; i++) {
        int arrival, burst, priority, io_count;

        printf("P%d: ", i + 1);
        if (scanf("%d %d %d %d", &arrival, &burst, &priority, &io_count) != 4) {
            arrival = 0;
            burst = 1;
            priority = 1;
            io_count = 0;
        }

        if (arrival < 0) {
            /* 음수 도착 시간은 시뮬레이션 가능한 값으로 보정한다. */
            arrival = 0;
        }
        if (burst <= 0) {
            burst = 1;
        }
        if (priority <= 0) {
            priority = 1;
        }

        init_process(&process_list[i], i + 1, arrival, burst, priority);

        if (io_count < 0) {
            io_count = 0;
        }
        if (io_count > MAX_IO_EVENTS) {
            io_count = MAX_IO_EVENTS;
        }

        process_list[i].io_count = 0;

        for (int k = 0; k < io_count; k++) {
            int start, io_burst;
            if (scanf("%d %d", &start, &io_burst) != 2) {
                start = 1;
                io_burst = 0;
            }

            if (start <= 0) {
                start = 1;
            }
            if (start >= burst) {
                start = burst - 1;
            }
            if (io_burst < 0) {
                io_burst = 0;
            }

            /* 중복된 I/O 시작 시점은 가능한 다음 시점으로 조정한다. */
            while (exists_io_start(&process_list[i], start) && start < burst - 1) {
                start++;
            }
            if (exists_io_start(&process_list[i], start)) {
                continue;
            }

            int accepted = process_list[i].io_count;
            process_list[i].io_start[accepted] = start;
            process_list[i].io_burst[accepted] = io_burst;
            process_list[i].total_io_time += io_burst;
            process_list[i].io_count++;
        }

        sort_io_events(&process_list[i]);
    }
}

void copy_workload(PCB src[], PCB dest[], int process_count) {
    for (int i = 0; i < process_count; i++) {
        /* 원본 입력은 유지하고 알고리즘 실행 중 생기는 상태만 초기화한다. */
        dest[i] = src[i];
        dest[i].remaining_time = src[i].cpu_burst;
        dest[i].next_io = 0;
        dest[i].blocked_until = -1;
        dest[i].first_run_time = -1;
        dest[i].finish_time = -1;
        dest[i].waiting_time = 0;
        dest[i].turnaround_time = 0;
        dest[i].response_time = 0;
        dest[i].completed = 0;
        dest[i].state = STATE_NEW;
    }
}

void print_workload(PCB process_list[], int process_count) {
    printf("\nProcess List\n");
    printf("PID\tAT\tBT\tPR\tIO_COUNT\tIO_EVENTS(start:burst)\n");

    for (int i = 0; i < process_count; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\t\t",
               process_list[i].pid,
               process_list[i].arrival_time,
               process_list[i].cpu_burst,
               process_list[i].priority,
               process_list[i].io_count);

        if (process_list[i].io_count == 0) {
            printf("-");
        } else {
            for (int k = 0; k < process_list[i].io_count; k++) {
                printf("%d:%d",
                       process_list[i].io_start[k],
                       process_list[i].io_burst[k]);
                if (k < process_list[i].io_count - 1) {
                    printf(", ");
                }
            }
        }
        printf("\n");
    }
}
