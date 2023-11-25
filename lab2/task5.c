#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

typedef enum
{
	NONE,
	MAN,
	WOMAN
} sex;

typedef struct
{
	int woman_cnt;
	int man_cnt;
	int bathers_cnt;
	int max_bathers_cnt;
	sex bather;
	int max_bath_time;
	pthread_cond_t woman_try_enter;
	pthread_cond_t man_try_enter;
	pthread_mutex_t mutex;
} bathroom_data;

void woman_wants_to_enter(bathroom_data* data);
void man_wants_to_enter(bathroom_data* data);
void woman_leaves(bathroom_data* data);
void man_leaves(bathroom_data* data);
void* process_woman(void* data);
void* process_man(void* data);

int main(int argc, char** argv)
{	
	if (argc == 1)
	{
		printf("Usage: cmd_path <N>\n");
		printf("N - max bathers number\n");
		return 0;
	}
	
	char* ptr;
	int max_n = strtoll(argv[1], &ptr, 10);
	if (*ptr != '\0')
	{
		printf("Invalid input\n");
		return 1;
	}
	if (errno == ERANGE)
	{
		printf("Overflow\n");
		return 2;
	}
	
	bathroom_data data;
	data.bathers_cnt = 0;
	data.bather = NONE;
	data.max_bathers_cnt = max_n;
	
	printf("Enter the number of women: ");
	scanf("%d", &data.woman_cnt);
	printf("Enter the number of men: ");
	scanf("%d", &data.man_cnt);
	printf("Enter the max time of bathing (> 0): ");
	scanf("%d", &data.max_bath_time);
	
	if (data.max_bath_time <= 0)
	{
		printf("Invalid input\n");
		return 1;
	}
	
	int code1 = pthread_cond_init(&data.woman_try_enter, NULL);
	int code2 = pthread_cond_init(&data.man_try_enter, NULL);
	int code3 = pthread_mutex_init(&data.mutex, NULL);
	if (code1 || code2 || code3)
	{
		pthread_cond_destroy(&data.woman_try_enter);
		pthread_cond_destroy(&data.man_try_enter);
		pthread_mutex_destroy(&data.mutex);
		printf("An error occurred\n");
		return 2;
	}
	
	srand(time(NULL));
	pthread_t humans[data.woman_cnt + data.man_cnt];
	for (int i = 0; i < data.woman_cnt + data.man_cnt; ++i)
	{
		if (i < data.woman_cnt)
		{
			pthread_create(&(humans[i]), NULL, process_woman, &data);
		}
		else
		{
			pthread_create(&(humans[i]), NULL, process_man, &data);
		}
	}
	
	for (int i = 0; i < data.woman_cnt + data.man_cnt; ++i)
	{
		pthread_join(humans[i], NULL);
	}
	
	pthread_cond_destroy(&data.woman_try_enter);
	pthread_cond_destroy(&data.man_try_enter);
	pthread_mutex_destroy(&data.mutex);
}

void woman_wants_to_enter(bathroom_data* data)
{
	// lock mutex to check the condition
	pthread_mutex_lock(&data->mutex);
	while (data->bathers_cnt == data->max_bathers_cnt || data->bather == MAN)
	{
		// unlocks the mutex and wait the conditions
		// when cond is fullfilled locks the mutex and continue performance
		pthread_cond_wait(&data->woman_try_enter, &data->mutex);
	}
	data->bather = WOMAN;
	data->bathers_cnt++;
	printf("Woman entered (%d)\n", data->bathers_cnt);
	pthread_mutex_unlock(&data->mutex);
}

void man_wants_to_enter(bathroom_data* data)
{
	pthread_mutex_lock(&data->mutex);
	while (data->bathers_cnt == data->max_bathers_cnt || data->bather == WOMAN)
	{
		pthread_cond_wait(&data->man_try_enter, &data->mutex);
	}
	data->bather = MAN;
	data->bathers_cnt++;
	printf("Man entered (%d)\n", data->bathers_cnt);
	pthread_mutex_unlock(&data->mutex);
}

void woman_leaves(bathroom_data* data)
{
	pthread_mutex_lock(&data->mutex);
	data->bathers_cnt--;
	if (data->bathers_cnt == 0)
	{
		data->bather = NONE;
	}
	printf("Woman left (%d)\n", data->bathers_cnt);
	pthread_mutex_unlock(&data->mutex);
	pthread_cond_broadcast(&data->woman_try_enter);
	pthread_cond_broadcast(&data->man_try_enter);
}

void man_leaves(bathroom_data* data)
{
	pthread_mutex_lock(&data->mutex);
	data->bathers_cnt--;
	if (data->bathers_cnt == 0)
	{
		data->bather = NONE;
	}
	printf("Man left (%d)\n", data->bathers_cnt);
	pthread_mutex_unlock(&data->mutex);
	pthread_cond_broadcast(&data->woman_try_enter);
	pthread_cond_broadcast(&data->man_try_enter);
}

void* process_woman(void* data)
{
	int max_time = ((bathroom_data*) data)->max_bath_time;
	sleep(rand() % 16);
	woman_wants_to_enter((bathroom_data*) data);
	sleep(rand() % max_time + 1);
	woman_leaves((bathroom_data*) data);
	return NULL;
}

void* process_man(void* data)
{
	int max_time = ((bathroom_data*) data)->max_bath_time;
	sleep(rand() % 16);
	man_wants_to_enter((bathroom_data*) data);
	sleep(rand() % max_time + 1);
	man_leaves((bathroom_data*) data);
	return NULL;
}