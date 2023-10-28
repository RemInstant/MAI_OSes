#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define PH_COUNT 5

typedef struct
{
	pthread_t* philosophers;
	pthread_mutex_t* forks;
	sem_t* sem_p;
	int id;
} philosopher_data;

void* run_philosopher(void* data)
{
	philosopher_data ph_data = *((philosopher_data*) data);
	int ph_id = ph_data.id;
	pthread_mutex_t* fork_l = &ph_data.forks[ph_id];
	pthread_mutex_t* fork_r = &ph_data.forks[(ph_id + 1) % PH_COUNT];
	sem_t* sem_p = ph_data.sem_p;
	
	while(1)
	{
		printf("Философ %d размышляет\n", ph_id);
		sem_wait(sem_p);
		pthread_mutex_lock(fork_l);
		printf("Философ %d берёт левую вилку (%d)\n", ph_id, ph_id);
		pthread_mutex_lock(fork_r);
		printf("Философ %d берёт правую вилку (%d)\n", ph_id, (ph_id + 1) % PH_COUNT);
		printf("Философ %d ест\n", ph_id);
		printf("Философ %d кладёт левую вилку\n", ph_id);
		printf("Философ %d кладёт правую вилку\n", ph_id);
		pthread_mutex_unlock(fork_l);
		pthread_mutex_unlock(fork_r);
		sem_post(sem_p);
	}
	return NULL;
}

int main()
{
	philosopher_data philosophers_info[PH_COUNT];
	pthread_t philosophers[PH_COUNT];
	pthread_mutex_t forks[PH_COUNT];
	
	sem_t sem;
	sem_init(&sem, 0, PH_COUNT - 1);
	
	// mutex threads initialization
	for (int i = 0; i < PH_COUNT; ++i)
	{
		pthread_mutex_init(&forks[i], NULL);
		printf("%d - %p\n", i, (void*) &(forks[i]));
	}
	for (int i = 0; i < PH_COUNT; ++i)
	{
		philosophers_info[i].id = i;
		philosophers_info[i].philosophers = philosophers;
		philosophers_info[i].forks = forks;
		philosophers_info[i].sem_p = &sem;
		pthread_create(&philosophers[i], NULL, run_philosopher, (void*) &(philosophers_info[i]));
	}
	// wait until all philosophers eat
	for (int i = 0; i < PH_COUNT; ++i)
	{
		pthread_join(philosophers[i], NULL);
	}
	// destroy mutex threads
	for (int i = 0; i < PH_COUNT; ++i)
	{
		pthread_mutex_destroy(&forks[i]);
	}
}