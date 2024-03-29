//--------------------------------------
//   tcpselect_server.c (I/O multiplexing)
//   client : tcp_client.c
//   compile : gcc -o tcpselect_server tcpselect_server.c
//--------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int echo_message(int csock);
fd_set rd_set; 				// 읽기용 소켓 기술자 집합
fd_set act_set;				// 활성화된 소켓 기술자 집합
int max_fd=0;               // 조사 대상인 최대기술자의 번호

/*
    int select(
    	int nfds,		// max file descriptor + 1
	fd_set *readfds,	// set of input descriptors
	fd_set *readfds,	// set of output descriptors
	fd_set *readfds,	// set of error descriptors
	struct timeval *timeout // waiting time of select
    )

    struct timeval(
    	long tv_sec;	//seconds
	long tv_usec;	//microseconds
    )
*/

int main(int argc, char *argv[])
{
    int server_sock;
    int client_sock;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    unsigned short server_port;
    unsigned int caddr_len;
    int fd;
    int flag = 1;

    if (argc != 2){
        printf("Usage:  %s <Port>\n", argv[0]);
        exit(1);
    }    

    printf("press Enter Key to quit the program ....\n");
    server_port = atoi(argv[1]);

    if ((server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("socket() failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_port);

    if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(1);
    }

    if (listen(server_sock, 5) < 0)
    {
        perror("listen() failed");
        exit(1);
    }

    printf("select_server start !!\n");
    // act_set의 모든 비트를 0으로 초기화
    FD_ZERO(&act_set);
    // act_set 중에서 server_sock에 해당하는 비트를 1로 setting
    FD_SET(server_sock, &act_set);	// 소켓기술자 값을 1로 세팅 - 클라이언트로부터 연결요청 감지
    FD_SET(0, &act_set); 		// 0을 1로 세팅 - 키보드 입력
    max_fd=server_sock;

    while(flag)
    {
        memcpy(&rd_set, &act_set, sizeof(rd_set));
	// select() : input/output process without blocking mode
        // 최대기술자 +1, 읽기용, 쓰기용, 에러용, 타임아웃
        if(select(max_fd+1, &rd_set, NULL, NULL, 0) < 0)
        {
            perror("select() failed");
            exit(1);
        }

	    // FD_ISSET : rd_set중에서 server_sock에 해당하는 비트가 1이면 양수값 return
        // 어떤 입력 신호가 감지된 후 그 원인을 조사하는 매크로
        if(FD_ISSET(server_sock, &rd_set))	// 연결 요청이 들어오면
        {
            caddr_len = sizeof(client_addr);
            client_sock=accept(server_sock, (struct sockaddr *)&client_addr, &caddr_len);
            
            if(client_sock < 0)
            {
               perror("accept() failed");
               exit(1);
            }
            
            // inet_ntoa() : Binary IPv4 주소를 십진법으로 변경
            printf("Echo request from %s\n", inet_ntoa(client_addr.sin_addr));
            FD_SET(client_sock, &act_set);	// 소켓기술자 값을 1로 세팅
            
            if(client_sock > max_fd){
                max_fd = client_sock;
            }
        }

        if(FD_ISSET(0, &rd_set)) 		// 키보드에서 입력이 들어오면
        {
            printf("Program terminated...\n");
            getchar();
            flag=0;
            exit(0);
        }

        for(fd=0; fd<=max_fd; fd++)
        {
            // 입력 신호 감지 후 원인 파악 위해 max_fd까지 각 rd_set값 조사
            if((fd != server_sock) && FD_ISSET(fd, &rd_set)){
                (void) echo_message(fd);		// 메시지 송수신 루틴 호출
            }
        }
    } // end of While
}

int echo_message(int csock)
{
    int msg_size;
    char msg_buf[BUFSIZ+1];
    msg_size = read(csock, msg_buf, BUFSIZ);  	// 메시지 수신
    
    if(msg_size <= 0)
    {
        close(csock);
        // act_set 중에서 csock에 해당하는 비트를 0으로 지움
        FD_CLR(csock, &act_set); 		// 기술자 값을 0으로 지움
        return(0);
    }

    printf("receive message from client => %s\n", msg_buf);
    strcat(msg_buf, " is returned message.");
    if(write(csock, msg_buf, strlen(msg_buf)+1) == -1) 	// 메시지 송신
    {
        perror("write() failed");
        exit(1);
    }
    return(0);
}
