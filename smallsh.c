/********************************************************
 * Name: Robert Jones
 * Class: CS344 Spring 2020
 * Due: May 21, 2020
 *
 * file smallsh.c
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
 
int main ()
{
	

	char *buffer;
	size_t bufsize = 80;
	size_t characters;
	buffer = (char *)malloc(bufsize * sizeof(char));

	while (1) 
	{
	
		printf(":");
	//	fflush();
		characters = getline(&buffer, &bufsize, stdin);
		buffer[strcspn(buffer, "\n")] = 0;
		characters--;

		printf("%zu characters\n", characters);
		printf("You typed: '%s'\n",buffer); 
	
	

		if ( buffer[0] == '\0' )
		{
			printf("empty string\n");
		}
		else if ( buffer[0] == '#' ) 
		{
			printf("This is a comment: %s\n", buffer);
		}
		else if (strcmp(buffer, "exit") == 0)
		{
			return 0;
		}
		else if (strcmp(buffer, "cd") == 0)
		{
			
		}
		else if (strcmp(buffer, "status") == 0)
		{

		}
		else 
		{

		}
		memset(buffer,'\0',sizeof(buffer));
	
	}

	free(buffer);
	return 0;
}
