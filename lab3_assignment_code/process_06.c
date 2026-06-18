#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    pid_t pid, pid2;
    int status;

    printf("counting to 5!\n");
    pid = fork();

    if(</1>) {
        pid2 = fork();

        if(</2>) {
            printf("1!\n");
            waitpid(</3>);
            printf("3!\n");
            waitpid(</4>);
            printf("5!\n");
        }
        else if (</5>) {
            sleep(3);
            printf("4!\n");
            exit(3);
        }
        else {
            return -1;
        }
    }
    else if (</6>) {
        sleep(1);
        printf("2!\n");
        exit(2);
    }
    else {
        return -1;
    }
    return 0;
}

/*
expected output

counting to 5!
1!
2!
3!
4!
5!

*/