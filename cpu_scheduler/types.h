#ifndef TYPES_H
#define TYPES_H

#define MAX_PROCESS 50
#define MAX_TIME 1000
#define MAX_IO_EVENTS 10
#define INF 987654321

/* 시뮬레이터가 프로세스의 현재 실행 단계를 표시할 때 쓰는 상태값이다. */
typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_WAITING,
    STATE_TERMINATED
} ProcessState;

/* 한 프로세스의 입력 정보, 실행 상태, 결과 지표를 함께 보관하는 구조체이다. */
typedef struct {
    int pid;

    /* 스케줄링 알고리즘이 선택 기준으로 사용하는 기본 속성이다. */
    int arrival_time;
    int cpu_burst;
    int remaining_time;
    int priority;

    /* 프로세스가 CPU 실행 중간에 수행할 I/O 이벤트 정보를 저장한다. */
    int io_count;
    int io_start[MAX_IO_EVENTS];
    int io_burst[MAX_IO_EVENTS];
    int next_io;
    int blocked_until;
    int total_io_time;

    /* 완료 후 성능 지표를 계산하기 위해 필요한 시각 정보이다. */
    int first_run_time;
    int finish_time;

    int waiting_time;
    int turnaround_time;
    int response_time;

    int completed;
    ProcessState state;
} PCB;

/* 하나의 스케줄링 알고리즘을 실행한 결과와 통계값을 담는 구조체이다. */
typedef struct {
    /* 각 시간에 CPU를 사용한 프로세스 pid를 기록하고, -1은 idle을 뜻한다. */
    int timeline[MAX_TIME];
    int end_time;

    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
    double avg_completion_time;

    int max_waiting_time;
    int min_waiting_time;
    double var_waiting_time;

    int max_turnaround_time;
    int min_turnaround_time;
    double var_turnaround_time;

    int max_response_time;
    int min_response_time;
    double var_response_time;

    int max_completion_time;
    int min_completion_time;
    double var_completion_time;

    int cpu_time;
    int io_time;
    int idle_time;
    int total_time;
    int context_switch_count;
    double cpu_utilization;
    double throughput;
} SimulationResult;

/* 스케줄링 알고리즘을 enum으로 구분해야 할 때 사용할 식별자이다. */
typedef enum {
    ALG_FCFS,
    ALG_SJF,
    ALG_PREEMPTIVE_SJF,
    ALG_PRIORITY,
    ALG_PREEMPTIVE_PRIORITY,
    ALG_RR
} SchedulingAlgorithm;

#endif
