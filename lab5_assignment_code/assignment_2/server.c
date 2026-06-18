#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 100
#define NP_SEND "./server_to_client"
#define NP_RECEIVE "./client_to_server"

int main(void) {
	char receive_msg[BUFFER_SIZE], send_msg[BUFFER_SIZE];
	int receive_fd, send_fd;
	int value;
	ssize_t bytes_read;

	/*---------------------------------------*/
	/* TODO 3 : make pipes and init fds      */
	/* (1) make NP_RECEIVE pipe              */
	/* (2) make NP_SEND pipe                 */
	/* (3) init receive_fd and send_fd       */

	//make client_to_server pipe
	// 이전 실행에서 남아 있을 수 있는 client_to_server pipe 파일을 삭제한다.
	unlink(NP_RECEIVE);
	// client가 보낸 정수 값을 받을 named pipe를 만든다.
	if (mkfifo(NP_RECEIVE, 0666) == -1) {
		perror("mkfifo client_to_server");
		return 1;
	}

	//make server_to_client pipe
	// 이전 실행에서 남아 있을 수 있는 server_to_client pipe 파일을 삭제한다.
	unlink(NP_SEND);
	// 서버가 계산 결과를 client에게 보낼 named pipe를 만든다.
	if (mkfifo(NP_SEND, 0666) == -1) {
		perror("mkfifo server_to_client");
		unlink(NP_RECEIVE);
		return 1;
	}

	// client_to_server pipe를 읽기 전용으로 열어서 client의 요청을 받을 준비를 한다.
	receive_fd = open(NP_RECEIVE, O_RDONLY);
	if (receive_fd == -1) {
		perror("open client_to_server");
		unlink(NP_RECEIVE);
		unlink(NP_SEND);
		return 1;
	}

	// server_to_client pipe를 쓰기 전용으로 열어서 계산 결과를 보낼 준비를 한다.
	send_fd = open(NP_SEND, O_WRONLY);
	if (send_fd == -1) {
		perror("open server_to_client");
		close(receive_fd);
		unlink(NP_RECEIVE);
		unlink(NP_SEND);
		return 1;
	}

	/* TODO 3 : END                          */
	/*---------------------------------------*/

	while (1) {
		/*---------------------------------------*/
		/* TODO 4 : read msg                     */

		// client_to_server pipe에서 client가 보낸 정수 값을 읽는다.
		memset(receive_msg, 0, BUFFER_SIZE);
		bytes_read = read(receive_fd, receive_msg, BUFFER_SIZE - 1);
		if (bytes_read <= 0) {
			break;
		}

		/* TODO 4 : END                          */
		/*---------------------------------------*/

		printf("server : receive %s\n", receive_msg);

		// 문자열로 받은 값을 정수로 변환한다.
		value = atoi(receive_msg);

		// 받은 정수를 제곱한 뒤 문자열로 저장한다.
		sprintf(send_msg, "%d", value*value);
		printf("\nserver : send %s\n", send_msg);

		/*---------------------------------------*/
		/* TODO 5 : write msg                    */

		// 계산한 제곱 결과를 server_to_client pipe로 client에게 보낸다.
		if (write(send_fd, send_msg, strlen(send_msg) + 1) == -1) {
			perror("write");
			break;
		}

		/* TODO 5 : END                          */
		/*---------------------------------------*/
	}

	// 사용이 끝난 파일 디스크립터를 닫고 named pipe 파일을 삭제한다.
	close(receive_fd);
	close(send_fd);
	unlink(NP_RECEIVE);
	unlink(NP_SEND);

	return 0;
}
