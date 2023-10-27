#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
	fork();
	printf("Hi\n");
	fork();
	printf("Hi\n");
}