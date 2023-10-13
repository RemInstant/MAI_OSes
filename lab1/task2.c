#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

typedef enum
{
	OK,
	INVALID_INPUT,
	INVALID_NUMBER,
	INVALID_FLAG,
	INVALID_LOGIN,
	INVALID_PIN,
	INVALID_DATE,
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
	int sanction_cmd_limit;
} user;

int is_separator(char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\n';
}

int is_leap_year(int year)
{
	return year % 400 == 0 && year % 100 != 0 && year % 4 == 0;
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

status_codes read_string(char** str, int* size)
{
	int iter = 0;
	*size = 2;
	*str = (char*) malloc(sizeof(char) * *size);
	
	if (*str == NULL)
	{
		return BAD_ALLOC;
	}
	
	char ch;
	scanf("%c", &ch);
	
	while (ch != '\n')
	{
		if (iter > *size - 2)
		{
			*size *= 2;
			char* temp_word = realloc(*str, sizeof(char) * *size);
			if (temp_word == NULL)
			{
				free(*str);
				return BAD_ALLOC;
			}
			*str = temp_word;
		}
		
		(*str)[iter++] = ch;
		
		scanf("%c", &ch);
	}
	
	(*str)[iter] = '\0';
	
	return OK;
}

status_codes separate_into_args(char* str, int* argc, char*** argv)
{
	if (str == NULL || argc == NULL || argv == NULL)
	{
		return INVALID_INPUT;
	}
	
	*argc = 0;
	for (int i = 0; str[i]; ++i)
	{
		if (i == 0 || (str[i-1] == ' ' && str[i] != ' '))
		{
			++(*argc);
		}
	}
	if (*argc == 0)
	{
		return INVALID_INPUT;
	}
	
	*argv = (char**) malloc(sizeof(char*) * (*argc));
	if (*argv == NULL)
	{
		return BAD_ALLOC;
	}
	
	// start and end of word in string (end - pos of first space after start)
	int start = 0, end = 1;
	for (int i = 0; i < *argc; ++i)
	{
		while (str[end] != ' ' && str[end] != '\0')
		{
			++end;
		}
		
		(*argv)[i] = (char*) malloc(sizeof(char) * (end-start+1));
		if ((*argv)[i] == NULL)
		{
			for (int j = 0; j < i; ++i)
			{
				free((*argv)[i]);
			}
			free(*argv);
			return BAD_ALLOC;
		}
		
		for (int j = 0; j < end - start; ++j)
		{
			(*argv)[i][j] = str[start+j];
		}
		(*argv)[i][end - start] = '\0';
		
		while (str[end] == ' ')
		{
			++end;
		}
		start = end;
	}
	return OK;
}

status_codes validate_login(char* login)
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

status_codes find_user(user* users, int users_cnt, char* login, int* some_user)
{
	if (users == NULL || login == NULL || some_user == NULL)
	{
		return INVALID_INPUT;
	}
	
	*some_user = -1;
	for (int i = 0; i < users_cnt; ++i)
	{
		user tmp_user = users[i];
		if (!strcmp(tmp_user.login, login))
		{
			*some_user = i;
			return OK;
		} 
	}
	return OK;
}

status_codes handle_signing_up(user** users, int* users_size, int* users_cnt)
{
	if (*users_cnt == *users_size)
	{
		user* tmp = (user*) realloc(*users, sizeof(user) * *users_size * 2);
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
	if (read_string(&login, &login_str_size) != OK)
	{
		return BAD_ALLOC;
	}
	if (validate_login(login) != OK)
	{
		free(login);
		return INVALID_LOGIN;
	}
	
	int some_user = -1;
	find_user(*users, *users_cnt, login, &some_user);
	if (some_user != -1)
	{
		free(login);
		return UNAVAILABLE_LOGIN;
	}
	
	printf("Enter PIN (a number from 0 to 100000): ");
	if (read_string(&pin_str, &pin_str_size) != OK)
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
	new_user.sanction_cmd_limit = 0;
	
	(*users)[*users_cnt] = new_user;
	++(*users_cnt);
	
	
	free(login);
	free(pin_str);
	return OK;
}

status_codes handle_signing_in(user* users, int users_cnt, int* logged_user)
{
	char* login;
	int login_str_size;
	char* pin_str;
	int pin_str_size;
	int pin;
	
	printf("Enter login (up to 6 letters or digits): ");
	if (read_string(&login, &login_str_size) != OK)
	{
		return BAD_ALLOC;
	}
	if (validate_login(login) != OK)
	{
		free(login);
		return INVALID_LOGIN;
	}
	
	*logged_user = -1;
	find_user(users, users_cnt, login, logged_user);
	if (*logged_user == -1)
	{
		free(login);
		return UNAVAILABLE_LOGIN;
	}
	
	user cur_user = users[*logged_user];
	
	printf("Enter PIN (a number from 0 to 100000): ");
	if (read_string(&pin_str, &pin_str_size) != OK)
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

void get_current_time(char* str_time)
{
	time_t cur_time = time(NULL);
	struct tm *cur_tm = localtime(&cur_time);
	sprintf(str_time, "%d:%d:%d", cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec);
}

void get_current_date(char* str_time)
{
	time_t cur_time = time(NULL);
	struct tm *cur_tm = localtime(&cur_time);
	char day[3], month[3];
	if (cur_tm->tm_mday < 10)
	{
		sprintf(day, "0%d", cur_tm->tm_mday);
	}
	else
	{
		sprintf(day, "%d", cur_tm->tm_mday);
	}
	if (cur_tm->tm_mon < 9)
	{
		sprintf(month, "0%d", cur_tm->tm_mon + 1);
	}
	else
	{
		sprintf(month, "%d", cur_tm->tm_mon + 1);
	}
	sprintf(str_time, "%s.%s.%d", day, month, cur_tm->tm_year + 1900);
}

status_codes str_to_time(char* str_time, time_t* time)
{
	if (str_time == NULL || str_time[10] != '\0' || str_time[2] != '.' || str_time[5] != '.')
	{
		return INVALID_INPUT;
	}
	for (int i = 0; i < 10; ++i)
	{
		if (str_time[i] == '\0' || (i != 2 && i != 5 && !isdigit(str_time[i])))
		{
			return INVALID_INPUT;
		}
	}
	
	char str_day[11], str_month[11], str_year[11];
	strcpy(str_day, str_time);
	strcpy(str_month, str_time + 3);
	strcpy(str_year, str_time + 6);
	str_day[2] = str_month[2] = str_year[4] = '\0';
	
	int day = atoi(str_day);
	int month = atoi(str_month);
	int year = atoi(str_year);
	
	char month_content[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	char leap_month_content[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	
	if (year <= 1970 || year > 3000 || month > 11)
	{
		return INVALID_INPUT;
	}
	
	if (is_leap_year(year))
	{
		if (day > leap_month_content[month])
		{
			return INVALID_INPUT;
		}
	}
	else
	{
		if (day > month_content[month])
		{
			return INVALID_INPUT;
		}
	}
	
	struct tm some_tm;
	some_tm.tm_sec = some_tm.tm_min = some_tm.tm_hour = 0;
	some_tm.tm_isdst = -1; 
	some_tm.tm_mday = day;
	some_tm.tm_mon = month - 1;
	some_tm.tm_year = year - 1900;
	
	*time = mktime(&some_tm);
	return OK;
}

status_codes handle_howmuch(char** argv, long long* differ, char* message)
{
	if (argv == NULL || differ == NULL || message == NULL)
	{
		return INVALID_INPUT;
	}
	
	time_t cur_time = time(NULL);
	time_t some_time;
	if (str_to_time(argv[1], &some_time) != OK)
	{
		return INVALID_DATE;
	}
	else if (argv[2][0] != '-' || argv[2][2] != '\0')
	{
		return INVALID_FLAG;
	}
	else
	{
		switch (argv[2][1])
		{
			case 's':
				*differ = cur_time - some_time;
				sprintf(message, "seconds passed");
				break;
			case 'm':
				*differ = (cur_time - some_time) / 60;
				sprintf(message, "minutes passed");
				break;
			case 'h':
				*differ = (cur_time - some_time) / 3600;
				sprintf(message, "hours passed");
				break;
			case 'y':
				*differ = (cur_time - some_time) / 3600 / 24 / 365.2425;
				sprintf(message, "years passed");
				break;
			default:
				return INVALID_FLAG;
		}
	}
	return OK;
}

status_codes handle_sanctions(char** argv, user* users, int users_cnt, int* banned_user)
{
	if (argv == NULL || users == NULL)
	{
		return INVALID_INPUT;
	}
	
	char* login = argv[1];
	char* str_integer = argv[2];
	
	if (validate_login(login) != OK )
	{
		return INVALID_LOGIN;
	}
	if (validate_string_integer(str_integer) != OK)
	{
		return INVALID_NUMBER;
	}
	
	int cmd_limit = atoi(str_integer);
	if (cmd_limit < 0)
	{
		return INVALID_NUMBER;
	}
	
	int ban_user = -1;
	find_user(users, users_cnt, login, &ban_user);
	if (ban_user == -1)
	{
		return UNAVAILABLE_LOGIN;
	}
	
	char* ver_code = NULL;
	int ver_size = 0;
	printf("Please, enter verification code: ");
	if (read_string(&ver_code, &ver_size) != OK)
	{
		return BAD_ALLOC;
	}
	if (strcmp(ver_code, "12345"))
	{
		return INCORRECT_PIN;
	}
	
	users[ban_user].is_sanctioned = 1;
	users[ban_user].sanction_cmd_limit = cmd_limit;
	
	if (banned_user != NULL)
	{
		*banned_user = ban_user;
	}
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
	int cmd_counter = 0;
	while (run_flag)
	{
		// PRINT AVAILABLE COMMANDS
		if (is_logged)
		{
			printf("Print \"Time\" to get current time\nPrint \"Date\" to get current date\n");
			printf("Print \"Howmuch <time> <flag>\" to get time passed since date, defined in arg <time>");
			printf(" in secs (flag -s), mins (-m), hours (-h) or years (-y)\nPrint \"Logout\" to log out\n");
			printf("Print \"Sanctions <username> <number>\" to limit the number of commands user can execute in a session\n");
		}
		else
		{
			printf("Print \"Register\" to sign up\nPrint \"Login\" to sign in\nPrint \"Exit\" to exit\n");
		}
		
		// BEGIN READING COMMAND
		char* cmd = NULL;
		int size = -1;
		if (read_string(&cmd, &size) != OK)
		{
			free(users);
			printf("Critical memory lack error\n");
			return 1;
		}
		
		int argc = -1;
		char** argv = NULL;
		if (separate_into_args(cmd, &argc, &argv) != OK)
		{
			free(users);
			free(cmd);
			printf("Critical memory lack error\n");
			return 1;
		}
		free(cmd);
		// END READING COMMAND
		
		// BEGIN PROCESSING COMMAND
		if (is_logged)
		{
			++cmd_counter;
			// --- COMMAND: LOGOUT ---
			if (argc == 1 && !strcmp(argv[0], "Logout"))
			{
				printf("\nYou successfuly logged out\n\n");
				is_logged = 0;
				cmd_counter = 0;
			}
			// --- SANCTIONS CHECKER ---
			else if (users[logged_user].is_sanctioned && cmd_counter > users[logged_user].sanction_cmd_limit)
			{
				printf("\nYour command limit has been reached\n\n");
			}
			// --- COMMAND: TIME ---
			else if (argc == 1 && !strcmp(argv[0], "Time"))
			{
				char cur_time[9];
				get_current_time(cur_time);
				printf("\nTime: %s\n\n", cur_time);
			}
			// --- COMMAND: DATE ---
			else if (argc == 1 && !strcmp(argv[0], "Date"))
			{
				char cur_date[11];
				get_current_date(cur_date);
				printf("\nDate: %s\n\n", cur_date);
			}
			// --- COMMAND: HOWMUCH ---
			else if (argc == 3 && !strcmp(argv[0], "Howmuch"))
			{
				long long differ;
				char message[20];
				switch (handle_howmuch(argv, &differ, message))
				{
					case OK:
						printf("\n%lld %s\n\n", differ, message);
						break;
					case INVALID_DATE:
						printf("\nInvalid date\n\n");
						break;
					case INVALID_FLAG:
						printf("\nInvalid flag\n\n");
						break;
					default:
						printf("\nUnexpected error occurred\n\n");
						break;
				}
			}
			// --- COMMAND: SANCTIONS ---
			else if (argc == 3 && !strcmp(argv[0], "Sanctions"))
			{
				int banned_user;
				switch (handle_sanctions(argv, users, users_cnt, &banned_user))
				{
					case OK:
						printf("\nUser is succesfully sanctioned. Now %s can execute", users[banned_user].login);
						printf(" only up to %d commands in a session\n\n", users[banned_user].sanction_cmd_limit);
						break;
					case INVALID_NUMBER:
						printf("\nInvalid number\n\n");
						break;
					case INVALID_LOGIN:
						printf("\nInvalid username\n\n");
						break;
					case UNAVAILABLE_LOGIN:
						printf("\nThis account does not exist\n\n");
						break;
					case INCORRECT_PIN:
						printf("\nIncorrect verification code\n\n");
						break;
					case BAD_ALLOC:
						printf("\nThere is not enough memory to impose sanctions\n\n");
						break;
					default:
						printf("Unexpected erroc occured\n");
						break;
				}
			}
			else 
			{
				printf("\nInvalid command\n\n");
			}
		}
		else
		{
			// --- COMMAND: REGISTER ---
			if (argc == 1 && !strcmp(argv[0], "Register"))
			{
				switch (handle_signing_up(&users, &users_size, &users_cnt))
				{
					case OK:
						is_logged = 1;
						logged_user = users_cnt - 1;
						printf("\n%s, You are successfully signed up\n\n", users[logged_user].login);
						break;
					case INVALID_LOGIN:
						printf("\nInvalid login\n\n");
						break;
					case INVALID_PIN:
						printf("\nInvalid pin\n\n");
						break;
					case UNAVAILABLE_LOGIN:
						printf("\nThis account already exists\n\n");
						break;
					case OVERFLOW:
						printf("\nEntered pin caused overflow\n");
						break;
					case BAD_ALLOC:
						printf("\nThere is not enough memory to create a new account\n\n");
						break;
					default:
						printf("\nUnexpected error occurred\n\n");
						break;
				}
			}
			// --- COMMAND: LOGIN ---
			else if (argc == 1 && !strcmp(argv[0], "Login"))
			{
				switch (handle_signing_in(users, users_cnt, &logged_user))
				{
					case OK:
						is_logged = 1;
						printf("\n%s, You are successfully signed in\n", users[logged_user].login);
						if (users[logged_user].is_sanctioned)
						{
							printf("NOTE! Your account was sanctioned.");
							printf(" You can execute only %d commands in a session\n", users[logged_user].sanction_cmd_limit);
						}
						printf("\n");
						break;
					case INVALID_LOGIN:
						printf("\nInvalid login\n\n");
						break;
					case INVALID_PIN:
						printf("\nInvalid pin\n\n");
						break;
					case UNAVAILABLE_LOGIN:
						printf("\nThis account does not exist\n\n");
						break;
					case INCORRECT_PIN:
						printf("\nIncorrect PIN\n\n");
						break;
					case OVERFLOW:
						printf("\nEntered PIN caused overflow\n");
						break;
					default:
						printf("\nUnexpected error occurred\n\n");
						break;
				}
			}
			// --- COMMAND: EXIT ---
			else if (argc == 1 && !strcmp(argv[0], "Exit"))
			{
				run_flag = 0;
			}
			else
			{
				printf("\nInvalid command\n\n");
			}
		}
		// END PROCESSING COMMAND
		
		// FREE ARGUMENTS
		for (int i = 0; i < argc; ++i)
		{
			free(argv[i]);
		}
		free(argv);
	}
	
	free(users);
}