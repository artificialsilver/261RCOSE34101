#ifndef WORKLOAD_H
#define WORKLOAD_H

#include "types.h"

/* 프로세스 workload를 만들고, 복사하고, 화면에 출력하는 함수들의 선언이다. */
void create_random_workload(PCB process_list[], int *process_count);
void create_manual_workload(PCB process_list[], int *process_count);
void copy_workload(PCB src[], PCB dest[], int process_count);
void print_workload(PCB process_list[], int process_count);

#endif
