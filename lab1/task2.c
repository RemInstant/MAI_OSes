#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef enum
{
	OK,
	INVALID_INPUT,
	INVALID_LOGIN,
	INVALID_PIN,
	UNAVAILABLE_LOGIN,
	INCORRECT_PIN,
	OVERFLOW,
	BAD_ALLOC
} status_codes;

typedef struct
{
	char login[7];
	int pin;
	int is_sanctioned;
	int sanction_restriction;
} user;

int is_separator(char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\n';
}

status_codes validate_string_integer(char* str_int)
{
	int integer = 0;
	
	if (str_int[0] != '-')
	{
		for (int i = 0; str_int[i]; ++i)
		{
			if (!isdigit(str_int[i]))
			{
				return INVALID_INPUT;
			}
			
			int add = str_int[i] - '0';
			if (integer > (INT_MAX - add) / 10)
			{
				return OVERFLOW;
			}
			
			integer = integer * 10 + add;
		}
	}
	else
	{
		if (str_int[1] == '\0')
		{
			return INVALID_INPUT;
		}
		for (int i = 1; str_int[i]; ++i)
		{
			if (!isdigit(str_int[i]))
			{
				return INVALID_INPUT;
			}
			
			int subtr = str_int[i] - '0';
			if (integer < (INT_MIN + subtr) / 10)
			{
				return OVERFLOW;
			}
			
			integer = integer * 10 - subtr;
		}
	}
	return OK;
}

status_codes read_word(char** word, int* size)
{
	int iter = 0;
	*size = 2;
	*word = (char*) malloc(sizeof(char) * *size);
	
	if (*word == NULL)
	{
		return BAD_ALLOC;
	}
	
	char ch;
	scanf("%c", &ch);
	
	while (!is_separator(ch))
	{
		if (iter > *size - 2)
		{
			*size *= 2;
			char* temp_word = realloc(*word, *size);
			if (temp_word == NULL)
			{
				free(*word);
				return BAD_ALLOC;
			}
			*word = temp_word;
		}
		
		(*word)[iter++] = ch;
		
		scanf("%c", &ch);
	}
	
	(*word)[iter] = '\0';
	
	return OK;
}

status_codes validate_login(char* login, int size)
{
	int cnt = 0;
	for (int i = 0; login[i]; ++i)
	{
		if (!isdigit(login[i]) && !isalpha(login[i]))
		{
			return INVALID_LOGIN;
		}
		++cnt;
	}
	if (cnt > 7 || cnt == 0)
	{
		return INVALID_LOGIN;
	}
	return OK;
}

status_codes handle_signing_up(int* users_cnt, int* users_size, user** users)
{
	if (*users_cnt == *users_size)
	{
		user* tmp = (user*) realloc(*users, *users_size * 2);
		if (tmp == NULL)
		{
			return BAD_ALLOC;
		}
		*users_size *= 2;
		*users = tmp;
	}
	
	char* login;
	int login_str_size;
	char* pin_str;
	int pin_str_size;
	int pin;
	
	printf("Enter login (up to 6 letters or digits): ");
	if (read_word(&login, &login_str_size) != OK)
	{
		return BAD_ALLOC;
	}
	if (validate_login(login, login_str_size) != OK)
	{
		free(login);
		return INVALID_LOGIN;
	}
	
	for (int i = 0; i < *users_cnt; ++i)
	{
		user some_user = (*users)[i];
		if (!strcmp(some_user.login, login))
		{
			free(login);
			return UNAVAILABLE_LOGIN;
		} 
	}
	
	printf("Enter PIN (a number from 0 to 100000): ");
	if (read_word(&pin_str, &pin_str_size) != OK)
	{
		free(login);
		return BAD_ALLOC;
	}
	pin = atoi(pin_str);
	if (pin < 0 || pin > 100000)
	{
		free(login);
		free(pin_str);
		return INVALID_PIN;
	}
	
	user new_user;
	
	strcpy(new_user.login, login);
	new_user.pin = pin;
	new_user.is_sanctioned = 0;
	new_user.sanction_restriction = 0;
	
	(*users)[*users_cnt] = new_user;
	++(*users_cnt);
	
	
	free(login);
	free(pin_str);
	return OK;
}

status_codes handle_signing_in(int users_cnt, user* users, int* logged_user)
{
	char* login;
	int login_str_size;
	char* pin_str;
	int pin_str_size;
	int pin;
	
	printf("Enter login (up to 6 letters or digits): ");
	if (read_word(&login, &login_str_size) != OK)
	{
		return BAD_ALLOC;
	}
	if (validate_login(login, login_str_size) != OK)
	{
		free(login);
		return INVALID_LOGIN;
	}
	
	*logged_user = -1;
	for (int i = 0; i < users_cnt; ++i)
	{
		user some_user = users[i];
		if (!strcmp(some_user.login, login))
		{
			*logged_user = i;
			break;
		} 
	}
	if (*logged_user == -1)
	{
		return UNAVAILABLE_LOGIN;
	}
	
	user cur_user = users[*logged_user];
	
	printf("Enter PIN (a number from 0 to 100000): ");
	if (read_word(&pin_str, &pin_str_size) != OK)
	{
		free(login);
		return BAD_ALLOC;
	}
	pin = atoi(pin_str);
	if (pin < 0 || pin > 100000)
	{
		free(login);
		free(pin_str);
		return INVALID_PIN;
	}
	
	if (cur_user.pin != pin)
	{
		free(login);
		free(pin_str);
		return INCORRECT_PIN;
	}
	
	free(login);
	free(pin_str);
	return OK;
}

int main()
{
	int users_cnt = 0;
	int users_size = 1;
	user* users = (user*) malloc(sizeof(user));
	
	int is_logged = 0;
	int logged_user = -1;
	int run_flag = 1;
	while(run_flag)
	{
		if(is_logged)
		{
			printf("Print \"logout\" to log out\n");
			char* cmd;
			int size;
			
			if (read_word(&cmd, &size) != OK)
			{
				free(users);
				printf("Memory lack error\n");
				return 1;
			}
			
			if (!strcmp(cmd, "logout"))
			{
				is_logged = 0;
			}
		}
		else
		{
			printf("Print \"reg\" to sign up\nPrint \"log\" to sign in\nPrint \"exit\" to exit\n");
			char* cmd;
			int size;
			
			if (read_word(&cmd, &size) != OK)
			{
				free(users);
				printf("Memory lack error\n");
				return 1;
			}
			
			if (!strcmp(cmd, "reg"))
			{
				switch (handle_signing_up(&users_cnt, &users_size, &users))
				{
					case OK:
						is_logged = 1;
						logged_user = users_cnt - 1;
						printf("You are successfully signed up\n\n");
						break;
					case INVALID_LOGIN:
						printf("Invalid login\n\n");
						break;
					case INVALID_PIN:
						printf("Invalid pin\n\n");
						break;
					case UNAVAILABLE_LOGIN:
						printf("This account already exists\n\n");
						break;
					case OVERFLOW:
						printf("Entered pin caused overflow\n");
						break;
					case BAD_ALLOC:
						printf("There is not enough memory to create a new account\n\n");
						break;
					default:
						printf("Unexpected error occurred\n\n");
						break;
				}
			}
			else if (!strcmp(cmd, "log"))
			{
				switch (handle_signing_in(users_cnt, users, &logged_user))
				{
					case OK:
						is_logged = 1;
						printf("You are successfully signed in\n\n");
						break;
					case INVALID_LOGIN:
						printf("Invalid login\n\n");
						break;
					case INVALID_PIN:
						printf("Invalid pin\n\n");
						break;
					case UNAVAILABLE_LOGIN:
						printf("\nThis account does not exist\n\n");
						break;
					case INCORRECT_PIN:
						printf("Incorrect PIN\n\n");
						break;
					case OVERFLOW:
						printf("Entered pin caused overflow\n");
						break;
					default:
						printf("Unexpected error occurred\n\n");
						break;
				}
			}
			else if (!strcmp(cmd, "exit"))
			{
				run_flag = 0;
			}
		}
	}
	
	free(users);
}