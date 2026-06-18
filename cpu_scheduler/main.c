#include <stdio.h>
#include "types.h"
#include "workload.h"
#include "scheduler.h"
#include "evaluation.h"

int main() {
    /* 사용자가 만든 workload와 각 알고리즘의 결과를 저장하는 공간이다. */
    PCB process_list[MAX_PROCESS];
    SimulationResult results[7];

    int process_count = 0;
    int time_quantum = 2;
    int use_aging = 0;
    int aging_interval = 5;
    int input_mode;

    printf("CPU Scheduling Simulator\n");
    printf("1. Random workload\n");
    printf("2. Manual workload\n");
    printf("Select: ");
    if (scanf("%d", &input_mode) != 1) {
        input_mode = 2;
    }

    if (input_mode == 1) {
        create_random_workload(process_list, &process_count);
    } else {
        create_manual_workload(process_list, &process_count);
    }

    printf("Input time quantum for RR: ");
    if (scanf("%d", &time_quantum) != 1 || time_quantum <= 0) {
        time_quantum = 1;
    }

    printf("Run Priority Aging as an additional function? (1: yes, 0: no): ");
    if (scanf("%d", &use_aging) != 1) {
        use_aging = 0;
    }

    if (use_aging) {
        printf("Input aging interval: ");
        if (scanf("%d", &aging_interval) != 1 || aging_interval <= 0) {
            aging_interval = 5;
        }
    }

    print_workload(process_list, process_count);

    /* 같은 입력 workload를 여러 스케줄링 알고리즘에 차례대로 적용한다. */
    printf("\n================ FCFS ================\n");
    simulate_fcfs(process_list, process_count, &results[0]);

    printf("\n================ SJF =================\n");
    simulate_sjf(process_list, process_count, &results[1]);

    printf("\n=========== Preemptive SJF ===========\n");
    simulate_preemptive_sjf(process_list, process_count, &results[2]);

    printf("\n============== Priority ==============\n");
    simulate_priority(process_list, process_count, &results[3]);

    printf("\n======== Preemptive Priority =========\n");
    simulate_preemptive_priority(process_list, process_count, &results[4]);

    printf("\n=========== Round Robin ==============\n");
    simulate_round_robin(process_list, process_count, time_quantum, &results[5]);

    if (use_aging) {
        printf("\n=========== Priority Aging ===========\n");
        simulate_priority_aging(process_list, process_count, aging_interval, &results[6]);
    }

    compare_results(results, use_aging ? 7 : 6);

    return 0;
}
