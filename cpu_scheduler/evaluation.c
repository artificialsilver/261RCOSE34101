#include <stdio.h>
#include <math.h>
#include "evaluation.h"

/* 프로세스 상태 enum을 출력용 문자열로 바꾼다. */
static const char *state_to_string(ProcessState state) {
    switch (state) {
        case STATE_NEW: return "NEW";
        case STATE_READY: return "READY";
        case STATE_RUNNING: return "RUNNING";
        case STATE_WAITING: return "WAITING";
        case STATE_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

void init_result(SimulationResult *result) {
    /* timeline 전체를 idle 값으로 채워 새 결과 저장 공간을 준비한다. */
    for (int i = 0; i < MAX_TIME; i++) {
        result->timeline[i] = -1;
    }

    result->end_time = 0;

    result->avg_waiting_time = 0.0;
    result->avg_turnaround_time = 0.0;
    result->avg_response_time = 0.0;
    result->avg_completion_time = 0.0;

    result->max_waiting_time = 0;
    result->min_waiting_time = 0;
    result->var_waiting_time = 0.0;

    result->max_turnaround_time = 0;
    result->min_turnaround_time = 0;
    result->var_turnaround_time = 0.0;

    result->max_response_time = 0;
    result->min_response_time = 0;
    result->var_response_time = 0.0;

    result->max_completion_time = 0;
    result->min_completion_time = 0;
    result->var_completion_time = 0.0;

    result->cpu_time = 0;
    result->io_time = 0;
    result->idle_time = 0;
    result->total_time = 0;
    result->context_switch_count = 0;
    result->cpu_utilization = 0.0;
    result->throughput = 0.0;
}

/* 정수 지표 배열의 분산을 계산해 알고리즘 결과의 편차를 보여준다. */
static double variance_int(int values[], int count, double avg) {
    if (count <= 0) {
        return 0.0;
    }

    double total = 0.0;
    for (int i = 0; i < count; i++) {
        double diff = values[i] - avg;
        total += diff * diff;
    }
    return total / count;
}

static void calculate_runtime_stats(PCB p[], int process_count, SimulationResult *result) {
    int last_pid = -1;

    result->cpu_time = 0;
    result->idle_time = 0;
    result->context_switch_count = 0;

    for (int t = 0; t < result->end_time; t++) {
        int pid = result->timeline[t];

        /* timeline 값이 -1이면 CPU가 쉰 시간으로 집계한다. */
        if (pid == -1) {
            result->idle_time++;
            continue;
        }

        result->cpu_time++;
        /* 이전 실행 pid와 달라진 경우를 context switch로 센다. */
        if (last_pid != -1 && last_pid != pid) {
            result->context_switch_count++;
        }
        last_pid = pid;
    }

    result->total_time = result->end_time;
    result->io_time = 0;
    for (int i = 0; i < process_count; i++) {
        /* I/O 시간은 각 프로세스에 등록된 I/O burst의 합으로 계산한다. */
        result->io_time += p[i].total_io_time;
    }

    if (result->total_time > 0) {
        result->cpu_utilization = ((double)result->cpu_time / result->total_time) * 100.0;
        result->throughput = ((double)process_count / result->total_time) * 100.0;
    }
}

void calculate_metrics(PCB p[], int process_count, SimulationResult *result) {
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;
    double total_completion = 0.0;

    int waiting_values[MAX_PROCESS];
    int turnaround_values[MAX_PROCESS];
    int response_values[MAX_PROCESS];
    int completion_values[MAX_PROCESS];
    int valid_count = 0;

    for (int i = 0; i < process_count; i++) {
        /* 완료되지 않은 프로세스는 지표 계산 대상에서 제외한다. */
        if (p[i].finish_time < 0) {
            continue;
        }

        /* turnaround time은 도착부터 완료까지 걸린 전체 시간이다. */
        p[i].turnaround_time = p[i].finish_time - p[i].arrival_time;
        /* waiting time은 전체 체류 시간에서 CPU와 I/O 사용 시간을 뺀 값이다. */
        p[i].waiting_time = p[i].turnaround_time - p[i].cpu_burst - p[i].total_io_time;

        if (p[i].first_run_time >= 0) {
            /* response time은 도착 후 처음 CPU를 배정받기까지의 시간이다. */
            p[i].response_time = p[i].first_run_time - p[i].arrival_time;
        }

        waiting_values[valid_count] = p[i].waiting_time;
        turnaround_values[valid_count] = p[i].turnaround_time;
        response_values[valid_count] = p[i].response_time;
        completion_values[valid_count] = p[i].finish_time;
        valid_count++;

        total_waiting += p[i].waiting_time;
        total_turnaround += p[i].turnaround_time;
        total_response += p[i].response_time;
        total_completion += p[i].finish_time;
    }

    if (valid_count > 0) {
        result->avg_waiting_time = total_waiting / valid_count;
        result->avg_turnaround_time = total_turnaround / valid_count;
        result->avg_response_time = total_response / valid_count;
        result->avg_completion_time = total_completion / valid_count;

        result->min_waiting_time = result->max_waiting_time = waiting_values[0];
        result->min_turnaround_time = result->max_turnaround_time = turnaround_values[0];
        result->min_response_time = result->max_response_time = response_values[0];
        result->min_completion_time = result->max_completion_time = completion_values[0];

        /* 평균과 별개로 각 지표의 최솟값과 최댓값을 찾는다. */
        for (int i = 1; i < valid_count; i++) {
            if (waiting_values[i] < result->min_waiting_time) result->min_waiting_time = waiting_values[i];
            if (waiting_values[i] > result->max_waiting_time) result->max_waiting_time = waiting_values[i];

            if (turnaround_values[i] < result->min_turnaround_time) result->min_turnaround_time = turnaround_values[i];
            if (turnaround_values[i] > result->max_turnaround_time) result->max_turnaround_time = turnaround_values[i];

            if (response_values[i] < result->min_response_time) result->min_response_time = response_values[i];
            if (response_values[i] > result->max_response_time) result->max_response_time = response_values[i];

            if (completion_values[i] < result->min_completion_time) result->min_completion_time = completion_values[i];
            if (completion_values[i] > result->max_completion_time) result->max_completion_time = completion_values[i];
        }

        result->var_waiting_time = variance_int(waiting_values, valid_count, result->avg_waiting_time);
        result->var_turnaround_time = variance_int(turnaround_values, valid_count, result->avg_turnaround_time);
        result->var_response_time = variance_int(response_values, valid_count, result->avg_response_time);
        result->var_completion_time = variance_int(completion_values, valid_count, result->avg_completion_time);
    }

    calculate_runtime_stats(p, process_count, result);
}

void print_gantt_chart(SimulationResult *result) {
    printf("\nGantt chart\n");
    printf("========================================\n\n");

    if (result->end_time == 0) {
        printf("(empty)\n");
        return;
    }

    int starts[MAX_TIME];
    int pids[MAX_TIME];
    int segment_count = 0;

    /* 연속해서 같은 pid가 실행된 시간은 하나의 Gantt chart 구간으로 묶는다. */
    for (int t = 0; t < result->end_time; t++) {
        if (t == 0 || result->timeline[t] != result->timeline[t - 1]) {
            starts[segment_count] = t;
            pids[segment_count] = result->timeline[t];
            segment_count++;
        }
    }

    for (int i = 0; i < segment_count; i++) {
        if (pids[i] == -1) {
            printf("|  idle  ");
        } else {
            printf("|   P%-3d ", pids[i]);
        }
    }
    printf("|\n");

    for (int i = 0; i < segment_count; i++) {
        printf("%-9d", starts[i]);
    }
    printf("%-9d fin\n", result->end_time);
}

void print_metrics(PCB p[], int process_count, SimulationResult *result) {
    printf("\nProcess table\n");
    printf("========================================\n");
    printf("PID\tState\t\tArrival\tBurst\tPriority\tIO count\tCompletion\tTurnaround\tWaiting\t\tResponse\n");
    printf("--------------------------------------------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < process_count; i++) {
        printf("%d\t%-12s\t%d\t%d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",
               p[i].pid,
               state_to_string(p[i].state),
               p[i].arrival_time,
               p[i].cpu_burst,
               p[i].priority,
               p[i].io_count,
               p[i].finish_time,
               p[i].turnaround_time,
               p[i].waiting_time,
               p[i].response_time);
    }

    printf("\nAverage waiting time: %.2f\tMax: %d, Min:%d, Variance: %.2f\n",
           result->avg_waiting_time,
           result->max_waiting_time,
           result->min_waiting_time,
           result->var_waiting_time);

    printf("Average turnaround time: %.2f\tMax: %d, Min:%d, Variance: %.2f\n",
           result->avg_turnaround_time,
           result->max_turnaround_time,
           result->min_turnaround_time,
           result->var_turnaround_time);

    printf("Average response time: %.2f\tMax: %d, Min: %d, Variance: %.2f\n",
           result->avg_response_time,
           result->max_response_time,
           result->min_response_time,
           result->var_response_time);

    printf("Average completion time: %.2f\tMax: %d, Min:%d, Variance: %.2f\n",
           result->avg_completion_time,
           result->max_completion_time,
           result->min_completion_time,
           result->var_completion_time);

    printf("CPU Utilization: %.2f%%\n", result->cpu_utilization);
    printf("Throughput: %.2f process per 100s\n", result->throughput);
    printf("CPU time: %d\n", result->cpu_time);
    printf("IO time: %d\n", result->io_time);
    printf("Idle time: %d\n", result->idle_time);
    printf("Total time: %d\n", result->total_time);
    printf("Context switch: %d times\n", result->context_switch_count);
    printf("\n========================================\n");
}

void compare_results(SimulationResult results[], int algorithm_count) {
    /* 시뮬레이션 실행 순서와 같은 순서로 알고리즘 이름을 둔다. */
    const char *names[] = {
        "FCFS",
        "SJF",
        "Preemptive SJF",
        "Priority",
        "Preemptive Priority",
        "Round Robin",
        "Priority Aging"
    };

    int best_wt = 0;
    int best_tat = 0;
    int best_rt = 0;

    printf("\n================ Algorithm Comparison ================\n");
    printf("%-22s %-12s %-12s %-12s %-12s %-12s\n",
           "Algorithm", "Avg WT", "Avg TAT", "Avg RT", "CPU Util", "CS");

    for (int i = 0; i < algorithm_count; i++) {
        char cpu_util_text[16];
        snprintf(cpu_util_text, sizeof(cpu_util_text), "%.2f%%", results[i].cpu_utilization);

        printf("%-22s %-12.2f %-12.2f %-12.2f %-12s %-12d\n",
               names[i],
               results[i].avg_waiting_time,
               results[i].avg_turnaround_time,
               results[i].avg_response_time,
               cpu_util_text,
               results[i].context_switch_count);

        /* 평균 시간이 가장 작은 알고리즘을 지표별로 기록한다. */
        if (results[i].avg_waiting_time < results[best_wt].avg_waiting_time) {
            best_wt = i;
        }

        if (results[i].avg_turnaround_time < results[best_tat].avg_turnaround_time) {
            best_tat = i;
        }

        if (results[i].avg_response_time < results[best_rt].avg_response_time) {
            best_rt = i;
        }
    }

    printf("\nBest Average Waiting Time    : %s (%.2f)\n",
           names[best_wt], results[best_wt].avg_waiting_time);

    printf("Best Average Turnaround Time : %s (%.2f)\n",
           names[best_tat], results[best_tat].avg_turnaround_time);

    printf("Best Average Response Time   : %s (%.2f)\n",
           names[best_rt], results[best_rt].avg_response_time);
}
