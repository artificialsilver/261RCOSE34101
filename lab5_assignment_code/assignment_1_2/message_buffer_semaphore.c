#include "message_buffer_semaphore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

int shm_fd = -1;
void *memory_segment = NULL;

sem_t *sem = SEM_FAILED;

void init_sem(void) {
    /*---------------------------------------*/
    /* TODO 1 : init semaphore               */
    /* create/open POSIX named semaphore     */
    /* using sem_open() with initial value 1 */

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        printf("init_sem sem_open error!\n");
        return;
    }

    /* TODO 1 : END                          */
    /*---------------------------------------*/
    printf("init semaphore : %s\n", SEM_NAME);
}

void destroy_sem(void) {
    /*---------------------------------------*/
    /* TODO 2 : destroy semaphore            */
    /* close using sem_close() and remove    */
    /* named semaphore using sem_unlink()    */

    if (sem != SEM_FAILED) {
        if (sem_close(sem) == -1) {
            printf("destroy_sem sem_close error!\n");
        }
        sem = SEM_FAILED;
    }

    if (sem_unlink(SEM_NAME) == -1) {
        printf("destroy_sem sem_unlink error!\n");
    }

    /* TODO 2 : END                          */
    /*---------------------------------------*/
}

void s_wait(void) {
    /* 세마포어가 아직 열려 있지 않으면 열린 세마포어를 가져온다. */
    if (sem == SEM_FAILED) {
        sem = sem_open(SEM_NAME, 0);
        if (sem == SEM_FAILED) {
            printf("<s_wait> sem_open error!\n");
            return;
        }
    }

    /* 임계 영역에 들어가기 전에 wait()를 호출하여 접근을 직렬화한다. */
    if (sem_wait(sem) == -1) {
        printf("<s_wait> sem_wait error!\n");
        return;
    }
}

void s_quit(void) {
    /* 세마포어가 아직 열려 있지 않으면 열린 세마포어를 가져온다. */
    if (sem == SEM_FAILED) {
        sem = sem_open(SEM_NAME, 0);
        if (sem == SEM_FAILED) {
            printf("<s_quit> sem_open error!\n");
            return;
        }
    }

    /* 임계 영역이 끝난 뒤 post()로 세마포어를 해제한다. */
    if (sem_post(sem) == -1) {
        printf("<s_quit> sem_post error!\n");
        return;
    }
}

/*---------------------------------------------*/
/* TODO 3 : use s_quit() and s_wait()          */
/* use POSIX shared memory and semaphore       */
/* in the functions below                      */

int init_buffer(MessageBuffer **buffer) {
    /*---------------------------------------*/
    /* TODO 3-1 : init buffer                */
    /* (1) create POSIX shared memory        */
    /*     using shm_open()                  */
    /* (2) set size using ftruncate()        */
    /* (3) map memory using mmap()           */
    /* (4) initialize MessageBuffer fields   */

    if (buffer == NULL) {
        return -1;
    }
    *buffer = NULL;

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        printf("shm_open error!\n\n");
        return -1;
    }

    if (ftruncate(shm_fd, sizeof(MessageBuffer)) == -1) {
        printf("ftruncate error!\n\n");
        close(shm_fd);
        shm_fd = -1;
        return -1;
    }

    memory_segment = mmap(NULL, sizeof(MessageBuffer),
                          PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) {
        printf("mmap error!\n\n");
        close(shm_fd);
        shm_fd = -1;
        memory_segment = NULL;
        return -1;
    }

    *buffer = (MessageBuffer *)memory_segment;

    /* 초기 버퍼는 빈 상태로 설정한다. */
    (*buffer)->message.sender_id = 0;
    (*buffer)->message.data = 0;
    (*buffer)->account_id = 0;
    (*buffer)->is_empty = 1;

    /* TODO 3-1 : END                        */
    /*---------------------------------------*/

    printf("init buffer\n");
    return 0;
}

int attach_buffer(MessageBuffer **buffer) {
    /*---------------------------------------*/
    /* TODO 3-2 : attach buffer              */
    /* do not consider "no buffer situation" */
    /* (1) open POSIX shared memory          */
    /*     using shm_open()                  */
    /* (2) map memory using mmap()           */
    /* (3) assign mapped address to *buffer  */

    if (buffer == NULL) {
        return -1;
    }
    *buffer = NULL;

    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        printf("shm_open error!\n\n");
        return -1;
    }

    memory_segment = mmap(NULL, sizeof(MessageBuffer),
                          PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) {
        printf("mmap error!\n\n");
        close(shm_fd);
        shm_fd = -1;
        memory_segment = NULL;
        return -1;
    }

    *buffer = (MessageBuffer *)memory_segment;

    /* TODO 3-2 : END                        */
    /*---------------------------------------*/

    printf("attach buffer\n");
    printf("\n");
    return 0;
}

int detach_buffer(void) {
    if (munmap(memory_segment, sizeof(MessageBuffer)) == -1) {
        printf("munmap error!\n\n");
        return -1;
    }

    if (shm_fd != -1) close(shm_fd);
    if (sem != SEM_FAILED) sem_close(sem);

    printf("detach buffer\n\n");
    return 0;
}

int destroy_buffer(void) {
    if(shm_unlink(SHM_NAME) == -1) {
        printf("shm_unlink error!\n\n");
        return -1;
    }

    printf("destroy shared_memory\n\n");
    return 0;
}

int produce(MessageBuffer **buffer, int sender_id, int data, int account_id) {

    /*---------------------------------------*/
    /* TODO 3-3 : produce message            */
    /* protect the shared buffer using       */
    /* s_wait() and s_quit()                 */
    /* write sender_id, data, and account_id */
    /* to the shared single-slot buffer      */

    if (buffer == NULL || *buffer == NULL) {
        return -1;
    }

    /* 생산자는 메시지를 쓰기 전에 세마포어로 임계 영역을 잠근다. */
    s_wait();

    if ((*buffer)->is_empty == 0) {
        s_quit();
        return -1;
    }

    /* 공유 버퍼에 데이터를 모두 쓴 후에 상태를 변경한다. */
    (*buffer)->message.sender_id = sender_id;
    (*buffer)->message.data = data;
    (*buffer)->account_id = account_id;
    (*buffer)->is_empty = 0;

    /* 작업을 마친 뒤 세마포어를 해제한다. */
    s_quit();

    /* TODO 3-3 : END                        */
    /*---------------------------------------*/

    printf("produce message\n");

    return 0;
}

int consume(MessageBuffer **buffer, Message **message) {

    /*---------------------------------------*/
    /* TODO 3-4 : consume message            */
    /* protect the shared buffer using       */
    /* s_wait() and s_quit()                 */
    /* read the message from shared memory   */
    /* and update the buffer state           */

    if (buffer == NULL || *buffer == NULL || message == NULL) {
        return -1;
    }

    /* 소비자는 읽기와 상태 변경 모두를 임계 영역으로 보호한다. */
    s_wait();

    if ((*buffer)->is_empty == 1) {
        s_quit();
        return -1;
    }

    /* 공유 메모리에서 메시지를 읽고 빈 상태로 되돌린다. */
    *message = &((*buffer)->message);
    (*buffer)->is_empty = 1;

    s_quit();

    /* TODO 3-4 : END                        */
    /*---------------------------------------*/
    return 0;
}

/* TODO 3 : END                                */
/*-------------------------------------------------------*/
