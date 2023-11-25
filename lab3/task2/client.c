#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/wait.h>

typedef long long ll;

#define META_LEN 5
#define MSGMAX 1024
#define MSGKEY 320
#define PRIORS 11

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

typedef enum
{
	CMD_SHUTDOWN = 1,
	CMD_TAKE,
	CMD_PUT,
	CMD_MOVE,
	CMD_PRINT,
	CMD_EXIT
} server_cmd;

typedef enum
{
	CMD_CODE_OK,
	CMD_CODE_ERR,
	CMD_CODE_INTERNAL_ERR,
	CMD_CODE_LOST,
	CMD_CODE_WON,
	CMD_CODE_SHUTDOWN
} server_cmd_code;

typedef enum
{
	LEFT,
	RIGHT
} obj_side;

typedef enum
{
	BOAT,
	WOLF,
	GOAT,
	CABBAGE,
	NONE
} objects;

status_codes read_line(char** str);
void print_cmd_code(server_cmd_code scmd_code);
void print_riddle_state(const char* str);

int main()
{
	pid_t pid;
	while (getpid() <= PRIORS)
	{
		switch (pid = fork())
		{
			case -1:
				print_error(FORK_ERROR);
				return FORK_ERROR;
			case 0:
				break;
			default:
				waitpid(pid, NULL, 0);
		}
	}
	// msg format: 4 bytes for pid, 1 byte for cmd code, N bytes for main data
	int fd = msgget(MSGKEY, 0666);
	
	if (fd < 0)
	{
		print_error(MSQ_OPENING_ERROR);
		return MSQ_OPENING_ERROR;
	}
	
	pid = getpid();
	msg_t msg;
	memcpy(msg.mtext, &pid, 4);
	
	printf("Cmds: take put move print exit shutdown\n");
	status_codes err_code = OK;
	int run_flag = 1;
	while (!err_code && run_flag)
	{
		// READ CMD
		char* cmd = NULL;
		printf("Cmd: ");
		err_code = read_line(&cmd);
		
		msg.mtype = 5;
		server_cmd scmd;
		status_codes cmd_code = OK;
		// --- COMMAND TAKE ---
		if (!err_code && !strcmp(cmd, "take"))
		{
			msg.mtext[4] = CMD_TAKE;
			msg.mtext[6] = '\0';
			char* obj = NULL;
			printf("Enter object (wolf/goat/cabbage) to take: ");
			cmd_code = read_line(&obj);
			if (!cmd_code)
			{
				if (!strcmp(obj, "wolf"))
				{
					msg.mtext[5] = WOLF;
				}
				else if (!strcmp(obj, "goat"))
				{
					msg.mtext[5] = GOAT;
				}
				else if (!strcmp(obj, "cabbage"))
				{
					msg.mtext[5] = CABBAGE;
				}
				else
				{
					cmd_code = INVALID_INPUT;
				}
			}
			free(obj);
		}
		// --- COMMAND PUT ---
		else if (!err_code && !strcmp(cmd, "put"))
		{
			msg.mtext[4] = CMD_PUT;
			msg.mtext[5] = '\0';
		}
		// --- COMMAND MOVE ---
		else if (!err_code && !strcmp(cmd, "move"))
		{
			msg.mtext[4] = CMD_MOVE;
			msg.mtext[5] = '\0';
		}
		// --- COMMAND PRINT ---
		else if (!err_code && !strcmp(cmd, "print"))
		{
			msg.mtext[4] = CMD_PRINT;
			msg.mtext[5] = '\0';
		}
		// --- COMMAND EXIT ---
		else if (!err_code && !strcmp(cmd, "exit"))
		{
			msg.mtext[4] = CMD_EXIT;
			msg.mtext[6] = '\0';
		}
		// --- COMMAND SHUTDOWN ---
		else if (!err_code && !strcmp(cmd, "shutdown"))
		{
			msg.mtext[4] = CMD_SHUTDOWN;
			msg.mtext[6] = '\0';
			msg.mtype = 1;
		}
		// --- INVALID COMMAND ---
		else
		{
			cmd_code = INVALID_INPUT;
		}
		if (cmd_code)
		{
			print_error(cmd_code);
			printf("\n");
			continue;
		}
		
		scmd = msg.mtext[4];
		free(cmd);
		if (!err_code)
		{
			if (msgsnd(fd, &msg, 7, 0))
			{
				err_code = MSQ_SND_ERROR;
			}
			if (scmd == CMD_EXIT)
			{
				return err_code;
			}
		}
		
		msg_t rcv_msg;
		if (!err_code)
		{
			int rcv_cnt = msgrcv(fd, &rcv_msg, MSGMAX, pid, MSG_NOERROR);
			if (rcv_cnt == -1)
			{
				err_code = MSQ_RCV_ERROR;
			}
		}
		if (!err_code)
		{
			server_cmd_code scmd_code = rcv_msg.mtext[0];
			print_cmd_code(scmd_code);
			if (scmd_code == CMD_CODE_WON)
			{
				print_riddle_state(rcv_msg.mtext);
				return OK;
			}
			if (scmd_code == CMD_CODE_LOST)
			{
				print_riddle_state(rcv_msg.mtext);
				return OK;
			}
			if (scmd_code == CMD_CODE_SHUTDOWN || scmd_code == CMD_CODE_INTERNAL_ERR)
			{
				if (scmd_code == CMD_CODE_SHUTDOWN && scmd == CMD_SHUTDOWN)
				{
					return OK;
				}
				return MSQ_CLOSED;
			}
			if (scmd == CMD_PRINT)
			{
				print_riddle_state(rcv_msg.mtext);
			}
		}
		
		if (err_code)
		{
			print_error(err_code);
			return err_code;
		}
		printf("\n");
	}
}

status_codes read_line(char** str)
{
	if (str == NULL)
	{
		return INVALID_INPUT;
	}
	ull iter = 0;
	ull size = 2;
	*str = (char*) malloc(sizeof(char) * size);
	if (*str == NULL)
	{
		return BAD_ALLOC;
	}
	char ch = getchar();
	while (ch != '\n' && ch != EOF)
	{
		if (iter > size - 2)
		{
			size *= 2;
			char* temp_word = realloc(*str, sizeof(char) * size);
			if (temp_word == NULL)
			{
				free(*str);
				return BAD_ALLOC;
			}
			*str = temp_word;
		}
		(*str)[iter++] = ch;
		ch = getchar();
	}
	(*str)[iter] = '\0';
	return OK;
}

void print_cmd_code(server_cmd_code scmd_code)
{
	switch (scmd_code)
	{
		case CMD_CODE_OK:
			printf("Command was successfully executed\n");
			break;
		case CMD_CODE_ERR:
			printf("Command cannot be executed\n");
			break;
		case CMD_CODE_INTERNAL_ERR:
			printf("Internal error occurred!\n");
			break;
		case CMD_CODE_LOST:
			printf("Wolf ate goat or goat ate cabbage. Lost!\n");
			break;
		case CMD_CODE_WON:
			printf("WON!\n");
			break;
		case CMD_CODE_SHUTDOWN:
			printf("Server was shutdowned!\n");
			break;
	}
}

void print_riddle_state(const char* str)
{
	if (str[5] != WOLF)
	{
		if (str[2])
		{
			printf("         |                |  Wolf\n");
		}
		else
		{
			printf("  Wolf   |                |\n");
		}
	}
	else
	{
		printf("         |                |\n");
	}
	
	if (str[3] || str[5] == GOAT)
	{
		printf("         |");
	}
	else
	{
		printf("  Goat   |");
	}
	
	char obj[10];
	if (str[5] == WOLF)
	{
		sprintf(obj, "(Wolf)");
	}
	else if (str[5] == GOAT)
	{
		sprintf(obj, "(Goat)");
	}
	else if (str[5] == CABBAGE)
	{
		sprintf(obj, "(Cabbage)");
	}
	else if (str[5] == NONE)
	{
		sprintf(obj, "(    )");
	}
	
	if (str[1])
	{
		printf(" %14s ", obj);
	}
	else
	{
		printf(" %-14s ", obj);
	}
	if (str[3] && str[5] != GOAT)
	{
		printf("|  Goat\n");
	}
	else
	{
		printf("|\n");
	}
	
	if (str[5] != CABBAGE)
	{
		if (str[4])
		{
			printf("         |                |  Cabbage\n");
		}
		else
		{
			printf(" Cabbage |                |\n");
		}
	}
	else
	{
		printf("         |                |\n");
	}
}
