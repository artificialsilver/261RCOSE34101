#include <stdio.h>
#include "scheduler.h"
#include "workload.h"
#include "queue.h"
#include "evaluation.h"

#define DEFAULT_AGING_INTERVAL 5

/* 전체 시뮬레이션을 계속 돌릴지 판단하는 완료 검사 함수이다. */
static int all_completed(PCB p[], int n) {
    for (int i = 0; i < n; i++) {
        if (!p[i].completed) {
            return 0;
        }
    }
    return 1;
}

/* 현재 시간 기준으로 프로세스가 READY/RUNNING 후보가 될 수 있는지 확인한다. */
static int is_available(PCB *p, int current_time) {
    return !p->completed &&
           p->remaining_time > 0 &&
           p->arrival_time <= current_time &&
           (p->blocked_until < 0 || p->blocked_until <= current_time);
}

/* 남은 CPU 시간에서 역산해 지금까지 사용한 CPU 시간을 구한다. */
static int consumed_cpu_time(PCB *p) {
    return p->cpu_burst - p->remaining_time;
}

/* 스케줄링 비교에 사용할 다음 CPU 실행 구간의 남은 길이를 계산한다. */
static int next_cpu_segment_remaining(PCB *p) {
    int consumed = consumed_cpu_time(p);

    if (p->next_io < p->io_count) {
        int until_io = p->io_start[p->next_io] - consumed;
        if (until_io > 0 && until_io < p->remaining_time) {
            return until_io;
        }
    }

    return p->remaining_time;
}

/* I/O blocking 시간이 지난 프로세스를 다시 준비 상태로 바꾼다. */
static void refresh_waiting_processes(PCB p[], int n, int current_time) {
    for (int i = 0; i < n; i++) {
        if (p[i].state == STATE_WAITING &&
            p[i].blocked_until <= current_time &&
            !p[i].completed) {
            p[i].state = STATE_READY;
        }
    }
}

static void enqueue_ready_processes(IntQueue *q,
                                    PCB p[],
                                    int n,
                                    int current_time,
                                    int in_ready[],
                                    int running_index) {
    refresh_waiting_processes(p, n, current_time);

    for (int i = 0; i < n; i++) {
        /* 실행 중인 프로세스는 큐에 다시 넣지 않는다. */
        if (i == running_index) {
            continue;
        }

        /* 큐 안에 없는 실행 가능 프로세스만 새로 등록한다. */
        if (is_available(&p[i], current_time) && !in_ready[i]) {
            queue_push(q, i);
            in_ready[i] = 1;
            p[i].state = STATE_READY;
        }
    }
}

/* SJF에서 CPU가 비었을 때 실행할 가장 짧은 CPU 구간의 프로세스를 찾는다. */
static int select_sjf(PCB p[], int n, int current_time) {
    int selected = -1;
    int shortest_burst = INF;

    refresh_waiting_processes(p, n, current_time);

    for (int i = 0; i < n; i++) {
        if (is_available(&p[i], current_time)) {
            int burst = next_cpu_segment_remaining(&p[i]);

            if (burst < shortest_burst) {
                shortest_burst = burst;
                selected = i;
            } else if (burst == shortest_burst) {
                /* 같은 길이면 도착 시간과 pid로 일관된 선택 순서를 만든다. */
                if (selected == -1 ||
                    p[i].arrival_time < p[selected].arrival_time ||
                    (p[i].arrival_time == p[selected].arrival_time &&
                     p[i].pid < p[selected].pid)) {
                    selected = i;
                }
            }
        }
    }

    return selected;
}

/* SRTF에서 매 시간 단위마다 남은 CPU 구간이 가장 짧은 프로세스를 고른다. */
static int select_srtf(PCB p[], int n, int current_time) {
    int selected = -1;
    int shortest_remaining = INF;

    refresh_waiting_processes(p, n, current_time);

    for (int i = 0; i < n; i++) {
        if (is_available(&p[i], current_time)) {
            int remaining = next_cpu_segment_remaining(&p[i]);

            if (remaining < shortest_remaining) {
                shortest_remaining = remaining;
                selected = i;
            } else if (remaining == shortest_remaining) {
                if (selected == -1 ||
                    p[i].arrival_time < p[selected].arrival_time ||
                    (p[i].arrival_time == p[selected].arrival_time &&
                     p[i].pid < p[selected].pid)) {
                    selected = i;
                }
            }
        }
    }

    return selected;
}

/* priority 숫자가 낮은 프로세스를 더 높은 우선순위로 선택한다. */
static int select_priority(PCB p[], int n, int current_time, int current_process) {
    int selected = -1;
    int highest_priority = INF;

    refresh_waiting_processes(p, n, current_time);

    for (int i = 0; i < n; i++) {
        if (is_available(&p[i], current_time)) {
            if (p[i].priority < highest_priority) {
                highest_priority = p[i].priority;
                selected = i;
            } else if (p[i].priority == highest_priority) {
                if (selected == -1 ||
                    p[i].arrival_time < p[selected].arrival_time ||
                    (p[i].arrival_time == p[selected].arrival_time &&
                     p[i].pid < p[selected].pid)) {
                    selected = i;
                }
            }
        }
    }

    if (current_process >= 0 &&
        is_available(&p[current_process], current_time) &&
        p[current_process].priority == highest_priority) {
        /* 같은 우선순위라면 기존 실행 프로세스를 유지해 불필요한 전환을 줄인다. */
        return current_process;
    }

    return selected;
}

/* 기다린 시간에 따라 aging이 반영된 임시 우선순위를 계산한다. */
static int aged_priority(PCB *p, int waiting_age, int aging_interval) {
    int priority = p->priority - (waiting_age / aging_interval);

    if (priority < 1) {
        /* 우선순위 값은 1보다 작아지지 않도록 제한한다. */
        priority = 1;
    }

    return priority;
}

static int select_priority_aging(PCB p[],
                                 int n,
                                 int current_time,
                                 int current_process,
                                 int waiting_age[],
                                 int aging_interval) {
    int selected = -1;
    int highest_priority = INF;

    refresh_waiting_processes(p, n, current_time);

    for (int i = 0; i < n; i++) {
        if (is_available(&p[i], current_time)) {
            int priority = aged_priority(&p[i], waiting_age[i], aging_interval);

            if (priority < highest_priority) {
                highest_priority = priority;
                selected = i;
            } else if (priority == highest_priority) {
                /* aging 결과가 같으면 더 오래 기다린 프로세스를 먼저 고른다. */
                if (selected == -1 ||
                    waiting_age[i] > waiting_age[selected] ||
                    (waiting_age[i] == waiting_age[selected] &&
                     p[i].arrival_time < p[selected].arrival_time) ||
                    (waiting_age[i] == waiting_age[selected] &&
                     p[i].arrival_time == p[selected].arrival_time &&
                     p[i].pid < p[selected].pid)) {
                    selected = i;
                }
            }
        }
    }

    if (current_process >= 0 &&
        is_available(&p[current_process], current_time) &&
        aged_priority(&p[current_process], waiting_age[current_process], aging_interval) == highest_priority) {
        return current_process;
    }

    return selected;
}

/* aging용 대기 시간을 갱신해 다음 선택에 반영할 값을 준비한다. */
static void update_aging_counters(PCB p[], int n, int current_time, int running_index, int waiting_age[]) {
    for (int i = 0; i < n; i++) {
        if (i == running_index) {
            waiting_age[i] = 0;
        } else if (is_available(&p[i], current_time)) {
            waiting_age[i]++;
        }
    }
}

/* 선택된 프로세스를 한 시간 단위만큼 실행하고 timeline에 기록한다. */
static void run_one_time_unit(PCB p[], int index, int *current_time, SimulationResult *result) {
    if (p[index].first_run_time == -1) {
        /* 최초 실행 시각은 response time 계산 자료로 남긴다. */
        p[index].first_run_time = *current_time;
    }

    p[index].state = STATE_RUNNING;
    result->timeline[*current_time] = p[index].pid;
    p[index].remaining_time--;
    (*current_time)++;
}

/* 한 시간 실행 뒤 프로세스가 계속 실행 가능한지, 종료됐는지, I/O로 빠졌는지 판단한다. */
static int handle_after_cpu_unit(PCB p[], int index, int current_time) {
    int executed_time = consumed_cpu_time(&p[index]);

    if (p[index].remaining_time == 0) {
        /* CPU burst를 모두 사용한 경우 완료 정보를 확정한다. */
        p[index].completed = 1;
        p[index].finish_time = current_time;
        p[index].state = STATE_TERMINATED;
        return 1;
    }

    if (p[index].next_io < p[index].io_count &&
        p[index].io_start[p[index].next_io] == executed_time) {

        /* 다음 I/O 지점에 도달하면 해당 시간만큼 WAITING 상태로 보낸다. */
        int io_burst = p[index].io_burst[p[index].next_io];
        p[index].blocked_until = current_time + io_burst;
        p[index].next_io++;
        p[index].state = STATE_WAITING;
        return 2;
    }

    return 0;
}

void simulate_fcfs(PCB original[], int process_count, SimulationResult *result) {
    PCB p[MAX_PROCESS];
    IntQueue ready_queue;
    int in_ready[MAX_PROCESS] = {0};

    copy_workload(original, p, process_count);
    init_result(result);
    queue_init(&ready_queue);

    int current_time = 0;
    int current_process = -1;

    while (!all_completed(p, process_count) && current_time < MAX_TIME) {
        /* 현재 시간에 준비된 프로세스를 FCFS 큐에 반영한다. */
        enqueue_ready_processes(&ready_queue, p, process_count, current_time, in_ready, current_process);

        if (current_process == -1 && !queue_empty(&ready_queue)) {
            /* FCFS는 먼저 큐에 들어온 프로세스부터 CPU를 배정한다. */
            current_process = queue_pop(&ready_queue);
            in_ready[current_process] = 0;
            p[current_process].state = STATE_RUNNING;
        }

        if (current_process == -1) {
            /* 준비된 프로세스가 없을 때는 idle 시간으로 남긴다. */
            result->timeline[current_time] = -1;
            current_time++;
            continue;
        }

        run_one_time_unit(p, current_process, &current_time, result);
        int status = handle_after_cpu_unit(p, current_process, current_time);

        if (status == 1 || status == 2) {
            current_process = -1;
        }
    }

    result->end_time = current_time;
    calculate_metrics(p, process_count, result);
    print_gantt_chart(result);
    print_metrics(p, process_count, result);
}

void simulate_sjf(PCB original[], int process_count, SimulationResult *result) {
    PCB p[MAX_PROCESS];

    copy_workload(original, p, process_count);
    init_result(result);

    int current_time = 0;
    int current_process = -1;

    while (!all_completed(p, process_count) && current_time < MAX_TIME) {
        if (current_process == -1) {
            /* 비선점 SJF는 실행 중인 프로세스를 중간에 빼앗지 않는다. */
            current_process = select_sjf(p, process_count, current_time);

            if (current_process == -1) {
                result->timeline[current_time] = -1;
                current_time++;
                continue;
            }
        }

        run_one_time_unit(p, current_process, &current_time, result);
        int status = handle_after_cpu_unit(p, current_process, current_time);

        if (status == 1 || status == 2) {
            current_process = -1;
        }
    }

    result->end_time = current_time;
    calculate_metrics(p, process_count, result);
    print_gantt_chart(result);
    print_metrics(p, process_count, result);
}

void simulate_preemptive_sjf(PCB original[], int process_count, SimulationResult *result) {
    PCB p[MAX_PROCESS];

    copy_workload(original, p, process_count);
    init_result(result);

    int current_time = 0;

    while (!all_completed(p, process_count) && current_time < MAX_TIME) {
        /* 선점 SJF는 매 시각마다 최단 잔여 구간을 다시 비교한다. */
        int current_process = select_srtf(p, process_count, current_time);

        if (current_process == -1) {
            result->timeline[current_time] = -1;
            current_time++;
            continue;
        }

        run_one_time_unit(p, current_process, &current_time, result);
        int status = handle_after_cpu_unit(p, current_process, current_time);

        if (status == 0) {
            /* 아직 끝나지 않았다면 다음 선택 후보가 되도록 상태를 되돌린다. */
            p[current_process].state = STATE_READY;
        }
    }

    result->end_time = current_time;
    calculate_metrics(p, process_count, result);
    print_gantt_chart(result);
    print_metrics(p, process_count, result);
}

void simulate_priority(PCB original[], int process_count, SimulationResult *result) {
    PCB p[MAX_PROCESS];

    copy_workload(original, p, process_count);
    init_result(result);

    int current_time = 0;
    int current_process = -1;

    while (!all_completed(p, process_count) && current_time < MAX_TIME) {
        if (current_process == -1) {
            /* 비선점 priority는 CPU가 빈 순간에만 우선순위 선택을 수행한다. */
            current_process = select_priority(p, process_count, current_time, -1);

            if (current_process == -1) {
                result->timeline[current_time] = -1;
                current_time++;
                continue;
            }
        }

        run_one_time_unit(p, current_process, &current_time, result);
        int status = handle_after_cpu_unit(p, current_process, current_time);

        if (status == 1 || status == 2) {
            current_process = -1;
        }
    }

    result->end_time = current_time;
    calculate_metrics(p, process_count, result);
    print_gantt_chart(result);
    print_metrics(p, process_count, result);
}

void simulate_preemptive_priority(PCB original[], int process_count, SimulationResult *result) {
    PCB p[MAX_PROCESS];

    copy_workload(original, p, process_count);
    init_result(result);

    int current_time = 0;
    int current_process = -1;

    while (!all_completed(p, process_count) && current_time < MAX_TIME) {
        /* 선점 priority는 매 시각마다 더 높은 우선순위 프로세스가 있는지 확인한다. */
        current_process = select_priority(p, process_count, current_time, current_process);

        if (current_process == -1) {
            result->timeline[current_time] = -1;
            current_time++;
            continue;
        }

        run_one_time_unit(p, current_process, &current_time, result);
        int status = handle_after_cpu_unit(p, current_process, current_time);

        if (status == 0) {
            p[current_process].state = STATE_READY;
        } else {
            current_process = -1;
        }
    }

    result->end_time = current_time;
    calculate_metrics(p, process_count, result);
    print_gantt_chart(result);
    print_metrics(p, process_count, result);
}

void simulate_priority_aging(PCB original[], int process_count, int aging_interval, SimulationResult *result) {
    PCB p[MAX_PROCESS];
    int waiting_age[MAX_PROCESS] = {0};

    copy_workload(original, p, process_count);
    init_result(result);

    if (aging_interval <= 0) {
        /* aging 간격이 유효하지 않으면 기본값으로 보정한다. */
        aging_interval = DEFAULT_AGING_INTERVAL;
    }

    int current_time = 0;
    int current_process = -1;

    printf("Aging interval: every %d waiting time unit(s), effective priority improves by 1\n",
           aging_interval);

    while (!all_completed(p, process_count) && current_time < MAX_TIME) {
        /* 대기 시간이 반영된 우선순위로 실행 프로세스를 선택한다. */
        current_process = select_priority_aging(p,
                                                process_count,
                                                current_time,
                                                current_process,
                                                waiting_age,
                                                aging_interval);

        if (current_process == -1) {
            result->timeline[current_time] = -1;
            current_time++;
            /* CPU가 비어 있어도 READY 프로세스의 대기 시간은 증가시킨다. */
            update_aging_counters(p, process_count, current_time - 1, -1, waiting_age);
            continue;
        }

        run_one_time_unit(p, current_process, &current_time, result);
        int status = handle_after_cpu_unit(p, current_process, current_time);

        update_aging_counters(p, process_count, current_time - 1, current_process, waiting_age);

        if (status == 0) {
            p[current_process].state = STATE_READY;
        } else {
            current_process = -1;
        }
    }

    result->end_time = current_time;
    calculate_metrics(p, process_count, result);
    print_gantt_chart(result);
    print_metrics(p, process_count, result);
}

void simulate_round_robin(PCB original[], int process_count, int time_quantum, SimulationResult *result) {
    PCB p[MAX_PROCESS];
    IntQueue ready_queue;
    int in_ready[MAX_PROCESS] = {0};

    copy_workload(original, p, process_count);
    init_result(result);
    queue_init(&ready_queue);

    if (time_quantum <= 0) {
        /* 잘못된 quantum으로 무한 반복이 생기지 않도록 최소값을 둔다. */
        time_quantum = 1;
    }

    int current_time = 0;

    while (!all_completed(p, process_count) && current_time < MAX_TIME) {
        /* Round Robin 큐에 현재 들어올 수 있는 프로세스를 추가한다. */
        enqueue_ready_processes(&ready_queue, p, process_count, current_time, in_ready, -1);

        if (queue_empty(&ready_queue)) {
            result->timeline[current_time] = -1;
            current_time++;
            continue;
        }

        int current_process = queue_pop(&ready_queue);
        in_ready[current_process] = 0;
        p[current_process].state = STATE_RUNNING;

        int used_time = 0;
        int status = 0;

        /* 할당 시간을 다 쓰거나 상태 변화가 생길 때까지 현재 프로세스를 실행한다. */
        while (used_time < time_quantum &&
               p[current_process].remaining_time > 0 &&
               current_time < MAX_TIME) {

            run_one_time_unit(p, current_process, &current_time, result);
            used_time++;

            status = handle_after_cpu_unit(p, current_process, current_time);

            /* 실행 도중 준비된 다른 프로세스도 다음 순서를 위해 큐에 넣는다. */
            enqueue_ready_processes(&ready_queue, p, process_count, current_time, in_ready, current_process);

            if (status == 1 || status == 2) {
                break;
            }
        }

        if (status == 0 && !p[current_process].completed) {
            /* 시간 할당량만 끝난 프로세스는 큐의 맨 뒤로 보낸다. */
            p[current_process].state = STATE_READY;
            queue_push(&ready_queue, current_process);
            in_ready[current_process] = 1;
        }
    }

    result->end_time = current_time;
    calculate_metrics(p, process_count, result);
    print_gantt_chart(result);
    print_metrics(p, process_count, result);
}
