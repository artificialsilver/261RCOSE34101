#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

void* magic_box(void* arg) {
    int input = (int)(intptr_t)arg;
    printf("multiplying %d by 6...\n", input);
    pthread_exit((void*)(intptr_t)(input * 6));
}

int main(int argc, char* argv[]){
    pthread_t tid;
    void* new_number;

    printf("Hey magic box, multiply 10 by 6\n");

    // <1/>, <2/>: 스레드 생성
    int status = pthread_create(&tid, NULL, magic_box, (void*)(intptr_t)10);
    if(status != 0){
        printf("error");
        return -1;
    }

    // <3/>, <4/>: 스레드 종료 대기 및 반환값 수신
    pthread_join(tid, &new_number);

    printf("the new number is %d\n", (int)(intptr_t)new_number);

    return 0;
}