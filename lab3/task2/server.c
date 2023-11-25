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

typedef struct client_data
{
	pid_t client_id;
	obj_side sides[4];
	objects boat_obj;
	struct client_data* next;
} client_data;

typedef struct
{
	client_data* head;
} Client_list;

status_codes add_client(Client_list* list, pid_t client_id);
status_codes remove_client(Client_list* list, pid_t client_id);
status_codes find_client(Client_list* list, pid_t client_id, client_data** data);
status_codes destroy_client_list(Client_list* list);
status_codes shutdown(int fd, pid_t closer_id);
status_codes put_data_to_msg(client_data data, msg_t* msg);

int main()
{
	msg_t msg;
	int fd = msgget(MSGKEY, IPC_CREAT | 0666);
	if (fd < 0)
	{
		print_error(MSQ_OPENING_ERROR);
		return MSQ_OPENING_ERROR;
	}
	
	Client_list list;
	list.head = NULL;
	
	while (1)
	{
		int rcv_cnt = msgrcv(fd, &msg, MSGMAX, -PRIORS, MSG_NOERROR);
		if (rcv_cnt == -1)
		{
			print_error(MSQ_RCV_ERROR);
			return MSQ_RCV_ERROR;
		}
		if (rcv_cnt != 7)
		{
			print_error(MSQ_INVALID_MSG);
			printf("type=%ld text=%s rcv_cnt=%d\n\n", msg.mtype, msg.mtext, rcv_cnt);
			continue;
		}
		
		pid_t client_id;
		server_cmd cmd = msg.mtext[4];
		int res_len = 0;
		memcpy(&client_id, msg.mtext, 4 * sizeof(char));
		printf("RECEIVED %d BYTES FROM %d\n", rcv_cnt, client_id);
		
		client_data* data = NULL;
		status_codes err_code = find_client(&list, client_id, &data);
		if (!err_code && data == NULL)
		{
			err_code = add_client(&list, client_id);
			if (!err_code)
			{
				data = list.head;
			}
		}
		switch (cmd)
		{
			case CMD_TAKE:
			{
				if (!err_code)
				{
					objects obj = msg.mtext[5];
					if (data->boat_obj == NONE && data->sides[obj] == data->sides[BOAT])
					{
						data->boat_obj = obj;
						msg.mtext[0] = CMD_CODE_OK;
					}
					else
					{
						msg.mtext[0] = CMD_CODE_ERR;
					}
					msg.mtext[1] = '\0';
					res_len = 2;
				}
				break;
			}
			case CMD_PUT:
			{
				if (!err_code)
				{
					if (data->boat_obj != NONE)
					{
						data->sides[data->boat_obj] = data->sides[BOAT];
						data->boat_obj = NONE;
						msg.mtext[0] = CMD_CODE_OK;
					}
					else
					{
						msg.mtext[0] = CMD_CODE_ERR;
					}
					msg.mtext[1] = '\0';
					res_len = 2;
				}
				break;
			}
			case CMD_MOVE:
			{
				if (!err_code)
				{
					data->sides[BOAT] = data->sides[BOAT] == LEFT ? RIGHT : LEFT;
					if (data->boat_obj != NONE)
					{
						data->sides[data->boat_obj] = data->sides[BOAT];
					}
					msg.mtext[0] = CMD_CODE_OK;
					msg.mtext[1] = '\0';
					res_len = 2;
				}
				break;
			}
			case CMD_PRINT:
			{
				if (!err_code)
				{
					msg.mtext[0] = CMD_CODE_OK;
					put_data_to_msg(*data, &msg);
					res_len = 7;
				}
				break;
			}
			case CMD_EXIT:
			{
				if (!err_code)
				{
					err_code = remove_client(&list, client_id);
				}
				break;
			}
			case CMD_SHUTDOWN:
			{
				destroy_client_list(&list);
				err_code = shutdown(fd, client_id);
				if (err_code)
				{
					print_error(err_code);
				}
				return err_code;
			}
			default:
			{
				err_code = MSQ_INVALID_MSG;
				break;
			}
		}
		
		if (!err_code && cmd != CMD_EXIT)
		{
			// --- CHECK LOSE POSITION ---
			if ((data->sides[CABBAGE] == data->sides[GOAT] && data->sides[GOAT] != data->sides[BOAT])
					||  (data->sides[WOLF] == data->sides[GOAT] && data->sides[GOAT] != data->sides[BOAT]))
			{
				msg.mtext[0] = CMD_CODE_LOST;
				put_data_to_msg(*data, &msg);
				res_len = 7;
				err_code = remove_client(&list, client_id);
			}
			// --- CHECK WIN POSITION ---
			else if (data->sides[WOLF] == data->sides[GOAT] && data->sides[GOAT] == data->sides[CABBAGE]
					&& data->sides[CABBAGE] == RIGHT && data->boat_obj == NONE)
			{
				msg.mtext[0] = CMD_CODE_WON;
				put_data_to_msg(*data, &msg);
				res_len = 7;
				err_code = remove_client(&list, client_id);
			}
		}
		
		if (err_code)
		{
			msg.mtext[0] = CMD_CODE_INTERNAL_ERR;
			msg.mtext[1] = '\0';
			res_len = 2;
		}
		
		msg.mtype = client_id;
		if (cmd != CMD_EXIT)
		{
			if (msgsnd(fd, &msg, res_len, 0))
			{
				print_error(MSQ_SND_ERROR);
				printf("To %d\n", client_id);
			}
		}
	}
}

status_codes add_client(Client_list* list, pid_t client_id)
{
	if (list == NULL)
	{
		return INVALID_ARG;
	}
	client_data* data = (client_data*) malloc(sizeof(client_data));
	if (data == NULL)
	{
		return BAD_ALLOC;
	}
	data->client_id = client_id;
	data->sides[0] = LEFT;
	data->sides[1] = LEFT;
	data->sides[2] = LEFT;
	data->sides[3] = LEFT;
	data->boat_obj = NONE;
	data->next = list->head;
	list->head = data;
	return OK;
}

status_codes remove_client(Client_list* list, pid_t client_id)
{
	if (list == NULL)
	{
		return INVALID_ARG;
	}
	if (list->head == NULL)
	{
		return INVALID_INPUT;
	}
	if (list->head->client_id == client_id)
	{
		client_data* temp = list->head;
		list->head = list->head->next;
		free(temp);
		return OK;
	}
	client_data* cur = list->head;
	while (cur->next != NULL && cur->next->client_id != client_id)
	{
		cur = cur->next;
	}
	if (cur->next == NULL)
	{
		return INVALID_INPUT;
	}
	client_data* temp = cur->next;
	cur->next = cur->next->next;
	free(temp);
	return OK;
}

status_codes find_client(Client_list* list, pid_t client_id, client_data** data)
{
	if (list == NULL)
	{
		return INVALID_ARG;
	}
	if (list->head == NULL)
	{
		*data = NULL;
		return OK;
	}
	client_data* cur = list->head;
	while (cur->client_id != client_id)
	{
		cur = cur->next;
		if (cur == NULL)
		{
			*data = NULL;
			return OK;
		}
	}
	*data = cur;
	return OK;
}

status_codes destroy_client_list(Client_list* list)
{
	if (list == NULL)
	{
		return INVALID_ARG;
	}
	client_data* cur = list->head;
	while(cur != NULL)
	{
		client_data* temp = cur;
		cur = cur->next;
		free(temp);
	}
	return OK;
}

status_codes shutdown(int fd, pid_t closer_id)
{
	msg_t msg;
	msg.mtext[0] = CMD_CODE_SHUTDOWN;
	msg.mtext[1] = '\0';
	msg.mtype = closer_id;
	if (msgsnd(fd, &msg, 2, 0))
	{
		print_error(MSQ_SND_ERROR);
		printf("To %d\n", closer_id);
	}
	
	int rcv_cnt;
	while ((rcv_cnt = msgrcv(fd, &msg, MSGMAX, -PRIORS, IPC_NOWAIT)) != -1)
	{
		if (rcv_cnt == 5 || rcv_cnt == 6)
		{
			pid_t client_id;
			memcpy(&client_id, msg.mtext, 4 * sizeof(char));
			msg.mtype = client_id;
			if (msgsnd(fd, &msg, 2, 0))
			{
				print_error(MSQ_SND_ERROR);
				printf("To %d\n", client_id);
			}
		}
	}
	sleep(1);
	printf("SHUT DOWN\n");
	if (msgctl(fd, IPC_RMID, 0))
	{
		return MSQ_CLOSING_ERROR;
	}
	return OK;
}

status_codes put_data_to_msg(client_data data, msg_t* msg)
{
	if (msg == NULL)
	{
		return INVALID_ARG;
	}
	msg->mtext[1] = data.sides[0];
	msg->mtext[2] = data.sides[1];
	msg->mtext[3] = data.sides[2];
	msg->mtext[4] = data.sides[3];
	msg->mtext[5] = data.boat_obj;
	msg->mtext[6] = '\0';
	return OK;
}