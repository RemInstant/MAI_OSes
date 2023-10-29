#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

typedef long long ll;
typedef unsigned long long ull;

typedef enum
{
	OK,
	INVALID_INPUT,
	INVALID_FLAG,
	INVALID_NUMBER,
	FILE_OPENING_ERROR,
	OVERFLOW,
	BAD_ALLOC,
	NULL_POINTER_ERROR,
	PROCESS_CREATING_ERROR
} status_codes;

void print_error(status_codes code)
{
	switch (code)
	{
		case OK:
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
		case OVERFLOW:
			printf("An overflow occurred\n");
			return;
		case BAD_ALLOC:
			printf("Memory lack error occurred\n");
			return;
		case NULL_POINTER_ERROR:
			printf("Null pointer error occurred\n");
			return;
		case PROCESS_CREATING_ERROR:
			printf("An error occurred while creating a process\n");
			return;
		default:
			printf("Unexpected error occurred\n");
			return;
	}
}

status_codes handle_file(char* path, char* pattern, int* has_occ)
{
	if (path == NULL || pattern == NULL || has_occ == NULL)
	{
		return NULL_POINTER_ERROR;
	}
	
	FILE* input = fopen(path, "r");
	if (input == NULL)
	{
		return FILE_OPENING_ERROR;
	}
	
	ull buf_size = strlen(pattern);
	if (buf_size == 0)
	{
		return INVALID_INPUT;
	}
	
	char* buf = (char*) malloc(sizeof(char) * (buf_size + 1));
	if (buf == NULL)
	{
		fclose(input);
		return BAD_ALLOC;
	}
	
	char ch = ' ';
	buf[buf_size] = '\0';
	for (int i = 0; i < buf_size; ++i)
	{
		ch = getc(input);
		if (ch != EOF)
		{
			buf[i] = ch;
		}
		else
		{
			fclose(input);
			free(buf);
			*has_occ = 0;
			return OK;
		}
	}
	
	while (ch != EOF)
	{
		if (!(strcmp(buf, pattern)))
		{
			*has_occ = 1;
			return OK;
		}
		
		ch = getc(input);
		for (int i = 1; i < buf_size; ++i)
		{
			buf[i-1] = buf[i];
		}
		buf[buf_size-1] = ch;
	}
	
	fclose(input);
	free(buf);
	*has_occ = 0;
	return OK;
}

status_codes get_paths(char* main_path, ull* path_cnt, char*** paths)
{
	if (main_path == NULL || path_cnt == NULL || paths == NULL)
	{
		return NULL_POINTER_ERROR;
	}
	
	FILE* main_file = fopen(main_path, "r");
	if (main_file == NULL)
	{
		return FILE_OPENING_ERROR;
	}
	
	ull all_size = 2;
	ull all_cnt = 0;
	char** all_paths = (char**) malloc(sizeof(char*) * all_size);
	if (all_paths == NULL)
	{
		fclose(main_file);
		return BAD_ALLOC;
	}
	
	// Filling array of pathes
	status_codes error_flag = OK;
	char ch = ' ';
	while (ch != EOF && error_flag == OK)
	{
		ull path_size = 2;
		ull path_len = 0;
		char* path = (char*) malloc(sizeof(char) * path_size);
		if (path == NULL)
		{
			error_flag = BAD_ALLOC;
		}
		
		// Reading a path-line of main file
		ch = fgetc(main_file);
		while ((ch != '\n' && ch != EOF) && error_flag == OK)
		{
			if (path_len + 1 == path_size)
			{
				path_size *= 2;
				char* tmp = (char*) realloc(path, sizeof(char) * path_size);
				if (tmp == NULL)
				{
					error_flag = BAD_ALLOC;
				}
				else
				{
					path = tmp;
				}
			}
			if (error_flag == OK)
			{
				path[path_len++] = ch;
			}
			ch = fgetc(main_file);
		}
		
		// Storing a path-line
		if (error_flag == OK)
		{
			path[path_len] = '\0';
			
			if (all_cnt == all_size)
			{
				all_size *= 2;
				char** tmp = (char**) realloc(all_paths, sizeof(char*) * all_size);
				if (tmp == NULL)
				{
					error_flag = BAD_ALLOC;
				}
				else
				{
					all_paths = tmp;
				}
			}
			if (error_flag == OK)
			{
				all_paths[all_cnt++] = path;
			}
		}
	}
	
	if (error_flag != OK)
	{
		for (int i = 0; i < all_cnt; ++i)
		{
			if (all_paths[i] != NULL)
			{
				free(all_paths[i]);
			}
		}
		free(all_paths);
		fclose(main_file);
		return error_flag;
	}
	
	*path_cnt = all_cnt;
	*paths = all_paths;
	return OK;
}

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Usage: cmd_path <file> <pattern>\n");
	}
	if (argc != 3)
	{
		print_error(INVALID_INPUT);
		return INVALID_INPUT;
	}
	
	ull path_cnt;
	char** paths;
	status_codes paths_code = get_paths(argv[1], &path_cnt, &paths);
	if (paths_code != OK)
	{
		print_error(paths_code);
		return paths_code;
	}
	
	pid_t pid = 1;
	for(ull i = 0; i < path_cnt && pid != 0; ++i)
	{
		pid = fork();
		if (pid == -1)
		{
			print_error(PROCESS_CREATING_ERROR);
			return PROCESS_CREATING_ERROR;
		}
		else if (pid == 0)
		{
			int has_occ = 0;
			status_codes search_code = handle_file(paths[i], argv[2], &has_occ);
			if (search_code != OK)
			{
				printf("While processing the file %llu occurred an error:\n", i + 1);
				print_error(search_code);
			}
			else if (has_occ)
			{
				printf("File %llu contains the pattern\n", i + 1);
			}
			else
			{
				printf("File %llu does not contain the pattern\n", i + 1);
			}
		}
	}
}