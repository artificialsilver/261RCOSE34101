#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdint.h>
#define NUM_TASKS 3
#define SPREADING 2

static _Atomic int cnt_task = NUM_TASKS;
// hint1: 바쁜 대기를 제거하기 위해 조건 변수와 뮤텍스 선언
pthread_mutex_t lock;
pthread_cond_t cond;

void spread_words() {
    sleep(SPREADING);
    printf("[subordinate] spreading words...\n");
    
    pthread_mutex_lock(&lock);
    cnt_task--;
    if (cnt_task == 0) {
        // hint2: cnt_task가 0이 되면 대기 중인 메인 스레드를 깨움
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&lock);
}

void* subordinate(void* arg) {
    sleep(SPREADING);
    printf("[subordinate] as you wish\n");
    for(int i = 0; i < 3; i++) {
        spread_words();
    }
    // hint3: 스레드 종료
    pthread_exit(NULL);
}

void* king(void* arg) {
    pthread_t tid;
    int status;
    printf("spread the words ");

    // hint4: 부하 스레드 생성 및 시작
    status = pthread_create(&tid, NULL, subordinate, NULL);
    if (status != 0) {
        pthread_exit(NULL);
    }

    pthread_detach(tid);

    printf("that I am king!\n");
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    pthread_t tid;
    int status;
    
    // hint5: 사용하기 전 뮤텍스와 조건 변수 초기화
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    status = pthread_create(&tid, NULL, king, NULL);
    if (status != 0) {
        printf("error");
        return -1;
    }
    pthread_join(tid, NULL);

    // hint6: 뮤텍스와 조건 변수를 사용하여 바쁜 대기 제거
    pthread_mutex_lock(&lock);
    while (cnt_task > 0) {
        pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    // hint7: 사용이 끝난 뮤텍스와 조건 변수 정리
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    printf("The words have been spread...\n");
    return 0;
}