#include <stdio.h>

#define BYTE_COUNT_1 11
#define BYTE_COUNT_2 4

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Usage: command_name <input file>\n");
		return 0;
	}
	
	if (argc != 2)
	{
		printf("Invalid input");
		return 1;
	}
	
	FILE* file = fopen(argv[1], "w");
	if (file == NULL)
	{
		printf("Cannot open the file\n");
		return 2;
	}
	
	char wBytes[BYTE_COUNT_1] = { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5 };
	
	if (fwrite(wBytes, sizeof(char), BYTE_COUNT_1, file) != BYTE_COUNT_1)
	{
		printf("A Writing error occurred\n");
		fclose(file);
		return 3;
	}
	
	fclose(file);
	file = fopen(argv[1], "r");
	if (file == NULL)
	{
		printf("Cannot open the file\n");
		return 2;
	}
	
	char ch;
	while (fread(&ch, sizeof(char), 1, file))
	{
		printf("%x %p\n", ch, file->_Placeholder);
	}
	
	fclose(file);
	file = fopen(argv[1], "r");
	if (file == NULL)
	{
		printf("Cannot open the file\n");
		return 2;
	}
	
	fseek(file, 3, SEEK_SET);
	char rBytes[BYTE_COUNT_2];
	
	if (fread(rBytes, sizeof(char), BYTE_COUNT_2, file) != BYTE_COUNT_2)
	{
		printf("A reading error occurred\n");
		fclose(file);
		return 4;
	}
	
	for (int i = 0; i < BYTE_COUNT_2; ++i)
	{
		printf("%x ", rBytes[i]);
	}
	printf("\n");
	
	fclose(file);
}