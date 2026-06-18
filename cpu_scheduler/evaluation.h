#ifndef EVALUATION_H
#define EVALUATION_H

#include "types.h"

/* 시뮬레이션 결과 초기화, 성능 지표 계산, 출력 기능의 선언이다. */
void init_result(SimulationResult *result);
void calculate_metrics(PCB p[], int process_count, SimulationResult *result);
void print_gantt_chart(SimulationResult *result);
void print_metrics(PCB p[], int process_count, SimulationResult *result);
void compare_results(SimulationResult results[], int algorithm_count);

#endif
