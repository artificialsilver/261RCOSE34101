.RECIPEPREFIX := >

CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# 빌드할 실행 파일 이름과 필요한 object 파일 목록이다.
TARGET = scheduler
OBJS = main.o workload.o queue.o scheduler.o evaluation.o

$(TARGET): $(OBJS)
>$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 각 object 파일을 만들 때 필요한 소스 파일과 헤더 의존성을 적는다.
main.o: main.c types.h workload.h scheduler.h
>$(CC) $(CFLAGS) -c main.c

workload.o: workload.c workload.h types.h
>$(CC) $(CFLAGS) -c workload.c

queue.o: queue.c queue.h types.h
>$(CC) $(CFLAGS) -c queue.c

scheduler.o: scheduler.c scheduler.h workload.h queue.h evaluation.h types.h
>$(CC) $(CFLAGS) -c scheduler.c

evaluation.o: evaluation.c evaluation.h types.h
>$(CC) $(CFLAGS) -c evaluation.c

# 컴파일 결과물을 삭제해 깨끗한 상태로 다시 빌드할 수 있게 한다.
clean:
>rm -f *.o $(TARGET)
