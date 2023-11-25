#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

typedef long long ll;

#define META_LEN 5
#define MSGMAX 1024
#define MSGKEY 310
#define PRIORS 11
#define CMD_CODE_PARSE 1
#define CMD_CODE_SHUTDOWN 2

typedef long long ll;
typedef unsigned long long ull;

typedef enum status_codes
{
	OK,
	INVALID_ARG,
	INVALID_INPUT,
	INVALID_FLAG,
	INVALID_NUMBER,
	FILE_OPENING_ERROR,
	FILE_CONTENT_ERROR,
	OVERFLOW,
	BAD_ALLOC,
	FORK_ERROR,
	MSQ_OPENING_ERROR,
	MSQ_CLOSING_ERROR,
	MSQ_SND_ERROR,
	MSQ_RCV_ERROR,
	MSQ_CLOSED,
	MSQ_INVALID_MSG
} status_codes;

void print_error(status_codes code)
{
	switch (code)
	{
		case OK:
			return;
		case INVALID_ARG:
			printf("Invalid function argument\n");
			return;
		case INVALID_INPUT:
			printf("Invalid input\n");
			return;
		case INVALID_FLAG:
			printf("Invalid flag\n");
			return;
		case INVALID_NUMBER:
			printf("Invalid number\n");
			return;
		case FILE_OPENING_ERROR:
			printf("File cannot be opened\n");
			return;
		case FILE_CONTENT_ERROR:
			printf("Invalid content of file\n");
			return;
		case OVERFLOW:
			printf("An overflow occurred\n");
			return;
		case BAD_ALLOC:
			printf("Memory lack error occurred\n");
			return;
		case FORK_ERROR:
			printf("Unable to fork\n");
			return;
		case MSQ_OPENING_ERROR:
			printf("Unable to open a message queue\n");
			return;
		case MSQ_CLOSING_ERROR:
			printf("Unable to close a message queue\n");
			return;
		case MSQ_SND_ERROR:
			printf("Unable to send a message in a queue\n");
			return;
		case MSQ_RCV_ERROR:
			printf("Unable to reveive a message from a queue\n");
			return;
		case MSQ_CLOSED:
			printf("Message queue was forcibly closed\n");
			return;
		case MSQ_INVALID_MSG:
			printf("Message received from a queue is invalid\n");
			return;
		default:
			printf("Unexpected error occurred\n");
			return;
	}
}

typedef struct
{
	long mtype;
	char mtext[MSGMAX];
} msg_t;

status_codes shutdown(int fd)
{
	msg_t msg;
	int rcv_cnt;
	while ((rcv_cnt = msgrcv(fd, &msg, MSGMAX, -PRIORS, IPC_NOWAIT)) != -1)
	{
		if (rcv_cnt >= 5)
		{
			pid_t client_id;
			memcpy(&client_id, msg.mtext, 4 * sizeof(char));
			msg.mtype = client_id;
			msg.mtext[0] = CMD_CODE_SHUTDOWN;
			msg.mtext[1] = '\0';
			if (msgsnd(fd, &msg, 2, 0))
			{
				print_error(MSQ_SND_ERROR);
				printf("To %d\n", client_id);
			}
		}
	}
	printf("SHUT DOWN\n");
	if (msgctl(fd, IPC_RMID, 0))
	{
		return MSQ_CLOSING_ERROR;
	}
	return OK;
}

int main()
{
	msg_t msg;
	int fd = msgget(MSGKEY, IPC_CREAT | 0666);
	if (fd < 0)
	{
		print_error(MSQ_OPENING_ERROR);
		return MSQ_OPENING_ERROR;
	}
	
	while (1)
	{
		int rcv_cnt = msgrcv(fd, &msg, MSGMAX, -PRIORS, MSG_NOERROR);
		if (rcv_cnt == -1)
		{
			print_error(MSQ_RCV_ERROR);
			status_codes code = shutdown(fd);
			if (code)
			{
				print_error(code);
			}
			return MSQ_RCV_ERROR;
		}
		if (rcv_cnt < 5)
		{
			print_error(MSQ_INVALID_MSG);
			printf("type=%ld text=%s\n\n", msg.mtype, msg.mtext);
			continue;
		}
		
		pid_t client_id;
		memcpy(&client_id, msg.mtext, 4 * sizeof(char));
		printf("RECEIVED %d BYTES FROM %d\n", rcv_cnt, client_id);
		
		if (msg.mtext[4] == CMD_CODE_SHUTDOWN)
		{
			status_codes code = shutdown(fd);
			if (code)
			{
				print_error(code);
			}
			return code;
		}
		
		char res[1001];
		int delim_flag = 0;
		int res_ind = 0;
		char* ptr = msg.mtext + 5;
		while (*ptr)
		{
			if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
			{
				delim_flag = 1;
			}
			else
			{
				if (delim_flag)
				{
					delim_flag = 0;
					res[res_ind++] = ' ';
				}
				res[res_ind++] = *ptr;
			}
			++ptr;
		}
		res[res_ind] = '\0';
		
		msg.mtype = client_id;
		msg.mtext[0] = CMD_CODE_PARSE;
		sprintf(msg.mtext+1, "%s", res);
		
		if (msgsnd(fd, &msg, strlen(msg.mtext)+1, 0))
		{
			print_error(MSQ_SND_ERROR);
			printf("To %d\n", client_id);
		}
		sleep(1);
	}
}
