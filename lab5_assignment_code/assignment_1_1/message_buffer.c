#include "message_buffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int shm_fd = -1;
void *memory_segment = NULL;

int init_buffer(MessageBuffer **buffer) {
    /*---------------------------------------*/
    /* TODO 1 : init buffer                  */
    /* (1) create POSIX shared memory        */
    /*     using shm_open()                  */
    /* (2) set size using ftruncate()        */
    /* (3) map memory using mmap()           */
    /* (4) initialize MessageBuffer fields   */

    /* 호출자가 넘긴 포인터가 유효한지 먼저 확인한다. */
    if (buffer == NULL) {
        return -1;
    }
    *buffer = NULL;

    /* 이름이 SHM_NAME인 POSIX 공유 메모리 객체를 생성하거나 연다. */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        printf("shm_open error!\n\n");
        return -1;
    }

    /* 공유 메모리의 크기를 MessageBuffer 구조체 하나만큼으로 설정한다. */
    if (ftruncate(shm_fd, sizeof(MessageBuffer)) == -1) {
        printf("ftruncate error!\n\n");
        close(shm_fd);
        shm_fd = -1;
        return -1;
    }

    /* 공유 메모리를 현재 프로세스 주소 공간에 읽기/쓰기 가능하게 매핑한다. */
    memory_segment = mmap(NULL, sizeof(MessageBuffer),
                          PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) {
        printf("mmap error!\n\n");
        close(shm_fd);
        shm_fd = -1;
        memory_segment = NULL;
        return -1;
    }

    /* 매핑된 메모리 주소를 MessageBuffer 포인터로 사용할 수 있게 연결한다. */
    *buffer = (MessageBuffer *)memory_segment;

    /* 단일 슬롯 버퍼는 처음에 비어 있는 상태로 초기화한다. */
    (*buffer)->message.sender_id = 0;
    (*buffer)->message.data = 0;
    (*buffer)->account_id = 0;
    (*buffer)->is_empty = 1;

    /* TODO 1 : END                          */
    /*---------------------------------------*/

    printf("init buffer\n");
    return 0;
}

int attach_buffer(MessageBuffer **buffer) {
    /*---------------------------------------*/
    /* TODO 2 : attach buffer                */
    /* do not consider "no buffer situation" */
    /* (1) open POSIX shared memory          */
    /*     using shm_open()                  */
    /* (2) map memory using mmap()           */
    /* (3) assign mapped address to *buffer  */

    /* 호출자가 넘긴 포인터가 유효한지 먼저 확인한다. */
    if (buffer == NULL) {
        return -1;
    }
    *buffer = NULL;

    /* 이미 생성된 공유 메모리 객체를 연다. */
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        printf("shm_open error!\n\n");
        return -1;
    }

    /* 열린 공유 메모리를 현재 프로세스 주소 공간에 붙인다. */
    memory_segment = mmap(NULL, sizeof(MessageBuffer),
                          PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) {
        printf("mmap error!\n\n");
        close(shm_fd);
        shm_fd = -1;
        memory_segment = NULL;
        return -1;
    }

    /* 매핑된 주소를 통해 다른 프로세스와 같은 MessageBuffer를 공유한다. */
    *buffer = (MessageBuffer *)memory_segment;

    /* TODO 2 : END                          */
    /*---------------------------------------*/

    printf("attach buffer\n");
    printf("\n");
    return 0;
}

int detach_buffer(void) {
    /* 현재 프로세스 주소 공간에서 공유 메모리 매핑을 해제한다. */
    if (munmap(memory_segment, sizeof(MessageBuffer)) == -1) {
        printf("munmap error!\n\n");
        return -1;
    }

    /* 공유 메모리 파일 디스크립터를 닫는다. */
    if (shm_fd != -1) close(shm_fd);

    printf("detach buffer\n\n");
    return 0;
}

int destroy_buffer(void) {
    /* 시스템에 남아 있는 공유 메모리 객체 이름을 제거한다. */
    if(shm_unlink(SHM_NAME) == -1) {
        printf("shm_unlink error!\n\n");
        return -1;
    }

    printf("destroy shared_memory\n\n");
    return 0;
}

int produce(MessageBuffer **buffer, int sender_id, int data, int account_id) {

    /*---------------------------------------*/
    /* TODO 3 : produce message              */
    /* write sender_id, data, and account_id */
    /* to the shared single-slot buffer      */

    /* 공유 버퍼가 제대로 연결되어 있는지 확인한다. */
    if (buffer == NULL || *buffer == NULL) {
        return -1;
    }

    /* 단일 슬롯 버퍼가 이미 차 있으면 새 메시지를 쓸 수 없다. */
    if ((*buffer)->is_empty == 0) {
        return -1;
    }

    /* 비어 있는 슬롯에 생산자가 보낸 메시지 내용을 기록한다. */
    (*buffer)->message.sender_id = sender_id;
    (*buffer)->message.data = data;
    (*buffer)->account_id = account_id;

    /* 메시지를 모두 쓴 뒤 버퍼가 차 있는 상태로 표시한다. */
    (*buffer)->is_empty = 0;
	
    /* TODO 3 : END                          */
    /*---------------------------------------*/

    printf("produce message\n");

    return 0;
}

int consume(MessageBuffer **buffer, Message **message) {

    /*---------------------------------------*/
    /* TODO 4 : consume message              */
    /* read the message from shared memory   */
    /* and update the buffer state           */

    /* 공유 버퍼와 결과를 받을 message 포인터가 유효한지 확인한다. */
    if (buffer == NULL || *buffer == NULL || message == NULL) {
        return -1;
    }

    /* 단일 슬롯 버퍼가 비어 있으면 읽을 메시지가 없다. */
    if ((*buffer)->is_empty == 1) {
        return -1;
    }

    /* 공유 메모리 안에 저장된 메시지의 주소를 소비자에게 넘겨준다. */
    *message = &((*buffer)->message);

    /* 이 과제에서는 세마포어를 쓰지 않으므로 슬롯 상태만 비어 있음으로 바꾼다. */
    (*buffer)->is_empty = 1;

    /* TODO 4 : END                          */
    /*---------------------------------------*/
    return 0;
}
