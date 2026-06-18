#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdint.h>
#define NUM_TASKS 3
#define SPREADING 2

static _Atomic int cnt_task = NUM_TASKS;

void spread_words() {
    sleep(SPREADING);
    printf("[subordinate] spreading words...\n");
    cnt_task--;
}

void* subordinate(void* arg) {
    sleep(SPREADING);
    printf("[subordinate] as you wish\n");
    for(int i = 0; i < 3; i++) {
        spread_words();
    }
    // hint1: 스레드 종료
    pthread_exit(NULL); 
}

void* king(void* arg) {
    pthread_t tid;
    int status;
    printf("spread the words ");

    // hint2: 부하 스레드 생성 및 시작
    status = pthread_create(&tid, NULL, subordinate, NULL);
    
    pthread_detach(tid);

    printf("that I am king!\n");
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    pthread_t tid;
    int status;

    status = pthread_create(&tid, NULL, king, NULL);
    if (status != 0) {
        printf("error");
        return -1;
    }
    pthread_join(tid, NULL);

    // hint3: busy waiting을 이용한 부하 스레드 종료 대기
    while(cnt_task > 0) {
        usleep(1000);
    }

    printf("The words have been spread...\n");
    return 0;
}