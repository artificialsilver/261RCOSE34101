#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 100
#define NP_RECEIVE "./server_to_client"
#define NP_SEND "./client_to_server"

static int open_fifo(const char *path, int flags) {
	int fd;

	// 서버가 named pipe를 만들기 전에 client가 먼저 실행될 수 있으므로 잠시 반복해서 open을 시도한다.
	for (int i = 0; i < 50; i++) {
		fd = open(path, flags);
		if (fd != -1) {
			return fd;
		}
		usleep(100 * 1000);
	}

	return -1;
}

int main(void) {
	char receive_msg[BUFFER_SIZE], send_msg[BUFFER_SIZE];
	int receive_fd, send_fd;
	/*---------------------------------------*/
	/* TODO 1 : init receive_fd and send_fd  */

	// client_to_server pipe를 쓰기 전용으로 열어서 서버에게 값을 보낼 준비를 한다.
	send_fd = open_fifo(NP_SEND, O_WRONLY);
	if (send_fd == -1) {
		perror("open client_to_server");
		return 1;
	}

	// server_to_client pipe를 읽기 전용으로 열어서 서버의 계산 결과를 받을 준비를 한다.
	receive_fd = open_fifo(NP_RECEIVE, O_RDONLY);
	if (receive_fd == -1) {
		perror("open server_to_client");
		close(send_fd);
		return 1;
	}

	/* TODO 1 : END                          */
	/*---------------------------------------*/

	for (int i=1; i<10; i++) {
		printf("client : send %d\n", i);
		sprintf(send_msg, "%d", i);
		/*---------------------------------------*/
		/* TODO 2 : send msg and receive msg     */

		// 정수 값을 문자열 형태로 client_to_server pipe에 보낸다.
		if (write(send_fd, send_msg, strlen(send_msg) + 1) == -1) {
			perror("write");
			break;
		}

		// server_to_client pipe에서 서버가 보낸 제곱 결과를 읽는다.
		memset(receive_msg, 0, BUFFER_SIZE);
		if (read(receive_fd, receive_msg, BUFFER_SIZE - 1) <= 0) {
			perror("read");
			break;
		}

		/* TODO 2 : END                          */
		/*---------------------------------------*/

		printf("client : receive %s\n\n", receive_msg);

		usleep(500*1000);
	}

	// 사용이 끝난 named pipe 파일 디스크립터를 닫는다.
	close(receive_fd);
	close(send_fd);

	return 0;
}
