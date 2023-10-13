#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Usage: command_name <input file> <output file>\n");
		printf("Program copies input's content into output\n");
		return 0;
	}
	
	if (argc != 3)
	{
		printf("Invalid input\n");
		return 1;
	}
	
	FILE* input_file = fopen(argv[1], "rb");
	FILE* output_file = fopen(argv[2], "wb");
	
	if (input_file == NULL)
	{
		printf("Cannot open the input file\n");
		return 2;
	}
	if (output_file == NULL)
	{
		fclose(input_file);
		printf("Cannot open the output file\n");
		return 2;
	}
	
	char buffer[1];
	while (fread(buffer, sizeof(char), 1, input_file) == 1)
	{
		if (fwrite(buffer, sizeof(char), 1, output_file) != 1)
		{
			fclose(input_file);
			fclose(output_file);
			printf("An error occurred while writing to the file\n");
			return 3;
		}
	}
	
	fclose(input_file);
	fclose(output_file);
	printf("File is successfully copied\n");
}