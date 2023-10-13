#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

typedef enum
{
	OK,
	INVALID_INPUT,
	INVALID_NUMBER,
	INVALID_BASE,
	OVERFLOW,
	BAD_ALLOC,
	NULL_POINTER_ERROR
} status_codes;

typedef enum
{
	CMD_XOR8,
	CMD_XOR32,
	CMD_MASK
} command_names;

int char_to_int(char ch)
{
	if (isdigit(ch))
	{
		return ch - '0';
	}
	if (isalpha(ch))
	{
		return toupper(ch) - 'A' + 10;
	}
	return -1;
}

status_codes str_to_ll(char* str_ll, int base, long long* res)
{
	if (str_ll == NULL || res == NULL)
	{
		return NULL_POINTER_ERROR;
	}
	if (base < 2 || base > 36)
	{
		return INVALID_BASE;
	}
	
	long long number = 0;
	if (str_ll[0] != '-')
	{
		for (int i = 0; str_ll[i]; ++i)
		{
			if (!isdigit(str_ll[i]) && !isalpha(str_ll[i]))
			{
				return INVALID_INPUT;
			}
			int add = char_to_int(str_ll[i]);
			if (add >= base)
			{
				return INVALID_INPUT;
			}
			if (number > (LLONG_MAX - add) / base)
			{
				return OVERFLOW;
			}
			number = number * base + add;
		}
	}
	else
	{
		if (str_ll[1] == '\0')
		{
			return INVALID_INPUT;
		}
		for (int i = 1; str_ll[i]; ++i)
		{
			if (!isdigit(str_ll[i]) && !isalpha(str_ll[i]))
			{
				return INVALID_INPUT;
			}
			int subtr = char_to_int(str_ll[i]);
			if (subtr >= base)
			{
				return INVALID_INPUT;
			}
			if (number < (LLONG_MIN + subtr) / base)
			{
				return OVERFLOW;
			}
			number = number * base - subtr;
		}
	}
	*res = number;
	return OK;
}

int is_argc_valid(int argc, command_names cmd)
{
	if (cmd == CMD_XOR8 || cmd == CMD_XOR32)
	{
		return argc == 3;
	}
	if (cmd == CMD_MASK)
	{
		return argc == 4;
	}
	return 0;
}

int handle_xor8(FILE* file)
{
	int sum = 0;
	char buffer[1];
	while (fread(buffer, sizeof(char), 1, file) == 1)
	{
		sum = (sum + buffer[0]) & 1;
	}
	return sum;
}

int handle_xor32(FILE* file)
{
	int sum = 0;
	char buffer[4];
	while (fread(buffer, sizeof(char), 4, file) == 4)
	{
		sum += buffer[0] << 24;
		sum += buffer[1] << 16;
		sum += buffer[2] << 8;
		sum += buffer[3];
		sum &= 1;
	}
	return sum;
}

status_codes handle_mask(FILE* file, char* str_mask, long long* res)
{
	if (file == NULL || str_mask == NULL || res == NULL)
	{
		return NULL_POINTER_ERROR;
	}
	
	long long mask_ll;
	status_codes convert_code = str_to_ll(str_mask, 16, &mask_ll);
	if (convert_code != OK)
	{
		return convert_code;
	}
	int32_t mask = mask_ll;
	
	long long cnt = 0;
	int32_t buffer;
	while (fread(&buffer, sizeof(int32_t), 1, file) == 1)
	{
		if ((buffer & mask) == mask)
		{
			++cnt;
		}
	}
	*res = cnt;
	return OK;
}

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Usage: command_name <input file> <flag>\n");
		printf("flags:\n");
		printf("xor8              -   sum bytes of file modulo 2\n");
		printf("xor32             -   sum 4-byte groups of file modulo 2\n");
		printf("mask <hex mask>   -   count 4-byte integers in file which match the given 4-byte hex mask\n");
		return 0;
	}
	
	if (argc < 3)
	{
		printf("Invalid input\n");
		return 1;
	}
	
	command_names cmd;
	if (!strcmp(argv[2], "xor8"))
	{
		cmd = CMD_XOR8;
	}
	else if (!strcmp(argv[2], "xor32"))
	{
		cmd = CMD_XOR32;
	}
	else if (!strcmp(argv[2], "mask"))
	{
		cmd = CMD_MASK;
	}
	else
	{
		printf("Invalid flag\n");
		return 1;
	}
	
	if (!is_argc_valid(argc, cmd))
	{
		printf("Invalid input\n");
		return 1;
	}
	
	FILE* input_file = fopen(argv[1], "rb");
	if (input_file == NULL)
	{
		printf("Cannot open the input file\n");
		return 2;
	}
	
	switch (cmd)
	{
		case CMD_XOR8:
			printf("Sum of bytes modulo 2 = %d\n", handle_xor8(input_file));
			break;
		case CMD_XOR32:
			printf("Sum of 4-byte groups modulo 2 = %d\n", handle_xor32(input_file));
			break;
		case CMD_MASK:
		{
			long long res;
			switch (handle_mask(input_file, argv[3], &res))
			{
				case OK:
					printf("There is %lld 4-byte integers in the file which match the mask\n", res);
					break;
				case INVALID_INPUT:
					printf("Invalid input\n");
					return 1;
				case OVERFLOW:
					printf("An overflow occurred while processing the file\n");
					return 3;
				default:
					printf("An unexpected error occurred\n");
					return 4;
			}
		}
	}
}