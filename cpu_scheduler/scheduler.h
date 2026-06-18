#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

/* 각 스케줄링 알고리즘의 시뮬레이션 진입점을 선언한다. */
void simulate_fcfs(PCB original[], int process_count, SimulationResult *result);
void simulate_sjf(PCB original[], int process_count, SimulationResult *result);
void simulate_preemptive_sjf(PCB original[], int process_count, SimulationResult *result);

void simulate_priority(PCB original[], int process_count, SimulationResult *result);
void simulate_preemptive_priority(PCB original[], int process_count, SimulationResult *result);
void simulate_priority_aging(PCB original[], int process_count, int aging_interval, SimulationResult *result);

void simulate_round_robin(PCB original[], int process_count, int time_quantum, SimulationResult *result);

#endif
