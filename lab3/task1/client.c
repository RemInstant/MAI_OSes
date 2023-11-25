#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

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
	MSQ_SND_ERROR,
	MSQ_RCV_ERROR,
	MSQ_CLOSED
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
		case MSQ_SND_ERROR:
			printf("Unable to send a message in a queue\n");
			return;
		case MSQ_RCV_ERROR:
			printf("Unable to reveive a message from a queue\n");
			return;
		case MSQ_CLOSED:
			printf("Message queue was forcibly closed\n");
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

status_codes validate_input(int argc, char** argv);
status_codes handle_data_sending(int fd, const char* inputs_path, int* msg_cnt);
status_codes handle_data_receiving(int fd, const char* output_path, int msg_cnt);
status_codes handle_shut_down(int fd);

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Usage: cmd_path <flag>\n");
		printf("Flags:\n");
		printf("-f <Inputs> <Output>   -   Process data on the server. Input files are defined in the Inputs,\n");
		printf("                       -   result is stored into the Output\n");
		printf("-k                     -   Shut down the server\n");
		return OK;
	}
	
	status_codes code = validate_input(argc, argv);
	if (code)
	{
		print_error(code);
		return code;
	}
	
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
	// cmd codes: 1 - parse string; 2 - shut down
	int fd = msgget(MSGKEY, 0666);
	
	if (fd < 0)
	{
		print_error(MSQ_OPENING_ERROR);
		return MSQ_OPENING_ERROR;
	}
	
	char flag = argv[1][1];
	switch (flag)
	{
		case 'f':
		{
			int msg_cnt = 0;
			code = handle_data_sending(fd, argv[2], &msg_cnt);
			if (code)
			{
				print_error(code);
				printf("Only %d lines was sent\n", msg_cnt);
			}
			code = handle_data_receiving(fd, argv[3], msg_cnt);
			break;
		}
		case 'k':
		{
			code = handle_shut_down(fd);
			break;
		}
	}
	if (code)
	{
		print_error(code);
	}
	return code;
}

status_codes validate_input(int argc, char** argv)
{
	if (argv == NULL)
	{
		return INVALID_ARG;
	}
	
	char fl = argv[1][1];
	if (argv[1][0] != '-' || argv[1][2] != '\0' || (fl != 'f' && fl != 'k'))
	{
		return INVALID_FLAG;
	}
	if (fl == 'f' && argc != 4)
	{
		return INVALID_INPUT;
	}
	if (fl == 'k' && argc != 2)
	{
		return INVALID_INPUT;
	}
	return OK;
}

status_codes fread_line(FILE* file, char** str)
{
	if (file == NULL || str == NULL)
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
	char ch = getc(file);
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
		ch = getc(file);
	}
	(*str)[iter] = '\0';
	return OK;
}

status_codes handle_data_sending(int fd, const char* inputs_path, int* msg_cnt)
{
	if (inputs_path == NULL || msg_cnt == NULL)
	{
		return INVALID_ARG;
	}
	
	FILE* inputs = fopen(inputs_path, "r");
	if (inputs == NULL)
	{
		return FILE_OPENING_ERROR;
	}
	
	msg_t msg;
	pid_t pid = getpid();
	memcpy(msg.mtext, &pid, 4);
	msg.mtext[4] = CMD_CODE_PARSE;
	
	*msg_cnt = 0;
	int run_flag = 1;
	char* file_path = NULL;
	status_codes err_code = fread_line(inputs, &file_path);
	if (!err_code && file_path[0] == '\0')
	{
		run_flag = 0;
	}
	while (!err_code && run_flag)
	{
		FILE* file = fopen(file_path, "r");
		free(file_path);
		file_path = NULL;
		if (!err_code && file == NULL)
		{
			err_code = FILE_OPENING_ERROR;
		}
		
		char raw_prior;
		int prior;
		char raw_text[MSGMAX - META_LEN];
		char text[MSGMAX - META_LEN];
		
		while (!err_code && !feof(file))
		{
			// read raw prior and text
			int empty_flag = 0;
			if (fscanf(file, "prior=%c text=%1000[^\n]\n", &raw_prior, &raw_text) != 2)
			{
				if (ftell(file) == 0)
				{
					empty_flag = 1;
				}
				else
				{
					err_code = FILE_CONTENT_ERROR;
				}
			}
			// parse raw prior
			if (!err_code && !empty_flag)
			{
				if (raw_prior < '0' || raw_prior > '9')
				{
					err_code = FILE_CONTENT_ERROR;
				}
				prior = raw_prior - '0';
			}
			// parse raw text
			if (!err_code && !empty_flag)
			{
				int slash_flag = 0;
				char* raw_ptr = raw_text;
				char* ptr = text;
				if (*raw_ptr != '\"')
				{
					err_code = FILE_CONTENT_ERROR;
				}
				++raw_ptr;
				while(!err_code && *raw_ptr)
				{
					if (slash_flag)
					{
						if (*raw_ptr == '\\' || *raw_ptr == '\"')
						{
							*ptr = *raw_ptr;
							++ptr;
							slash_flag = 0;
						}
						else
						{
							err_code = FILE_CONTENT_ERROR;
						}
					}
					else if (*raw_ptr != '\\')
					{
						*ptr = *raw_ptr;
						++ptr;
					}
					else
					{
						slash_flag = 1;
					}
					++raw_ptr;
				}
				--ptr;
				if (*ptr != '\"')
				{
					err_code = FILE_CONTENT_ERROR;
				
				}
				*ptr = '\0';
			}
			// send data
			if (!err_code && !empty_flag)
			{
				msg.mtype = prior + 2; // prior=1 is for shutdown
				sprintf(msg.mtext + META_LEN, "%s", text);
				if (msgsnd(fd, &msg, strlen(text)+6, 0))
				{
					err_code = MSQ_SND_ERROR;
				}
			}
			if (!err_code && !empty_flag)
			{
				++(*msg_cnt);
			}
		}
		err_code = err_code ? err_code : fread_line(inputs, &file_path);
		if (!err_code && file_path[0] == '\0')
		{
			run_flag = 0;
			free(file_path);
		}
	}
	fclose(inputs);
	return err_code;
}

status_codes handle_data_receiving(int fd, const char* output_path, int msg_cnt)
{
	if (output_path == NULL)
	{
		return INVALID_ARG;
	}
	
	FILE* output = fopen(output_path, "w");
	if (output == NULL)
	{
		return FILE_OPENING_ERROR;
	}
	
	status_codes err_code = OK;
	msg_t msg;
	int pid = getpid();
	while (!err_code && msg_cnt--)
	{
		int rcv_cnt = msgrcv(fd, &msg, MSGMAX, pid, MSG_NOERROR);
		if (rcv_cnt == -1)
		{
			err_code = MSQ_RCV_ERROR;
		}
		if (!err_code && msg.mtext[0] == CMD_CODE_SHUTDOWN)
		{
			err_code = MSQ_CLOSED;
		}
		if (!err_code)
		{
			fprintf(output, "%s\n", msg.mtext + 1);
		}
	}
	fclose(output);
	return err_code;
}

status_codes handle_shut_down(int fd)
{	
	pid_t pid = getpid();
	msg_t msg;
	msg.mtype = 1; // max prior for the shut down command
	memcpy(msg.mtext, &pid, 4);
	msg.mtext[4] = CMD_CODE_SHUTDOWN;
	msg.mtext[5] = '\0';
	if (msgsnd(fd, &msg, 6, 0))
	{
		return MSQ_SND_ERROR;
	}
	return OK;
}