#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
	pid_t pid;            // the identifier of the process
	pid_t parent_pid;     // pid of the parent
	pid_t group_pid;      // the identifierd of the process group
	uid_t real_uid;       // the user who owns the process
	uid_t effective_uid;  // the user on behalf of whom the process runs
	gid_t real_gid;       // the group of the real user
	gid_t effective_gid;  // same as euid
	
	pid = getpid();
	parent_pid = getppid();
	group_pid = getpgrp();
	real_uid = getuid();
	effective_uid = geteuid();
	real_gid = getgid();
	effective_gid = getegid();
	
	printf("Идентификатор процесса: %d\n", pid);
	printf("Идентификатор родительского процесса: %d\n", parent_pid);
	printf("Идентификатор группы процесса: %d\n", group_pid);
	printf("Реальный идентификатор пользователя: %d\n", real_uid);
	printf("Эффективный идентификатор пользователя: %d\n", effective_uid);
	printf("Реальный идентификатор группы пользователя: %d\n", real_gid);
	printf("Эффективный идентификатор группы пользователя: %d\n", effective_gid);
}