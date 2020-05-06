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
#include <signal.h>


char **parse_buffer(char *);
int proc_cd(char **);
int proc_status(char **);
int proc_exit(char **);
int proc_exec(char **);
void catchSIGINT(int );

int main ()
{
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	sigaction(SIGINT, &SIGINT_action, NULL);

	char *buffer;
	size_t bufsize = 2049;
	size_t characters;
	buffer = (char *)malloc(bufsize * sizeof(char));
	memset(buffer, '\0', sizeof(buffer));

	char **args;
	int status = -5;

	while (1) 
	{
		while (1)	
		{
			printf(":");
			fflush( stdout );
			characters = getline(&buffer, &bufsize, stdin);
			if (characters == -1) {
				clearerr(stdin);
			}
			else {
				buffer[strcspn(buffer, "\n")] = 0;
				characters--;
				break;
			}
		}


		if ( buffer[0] == '\0' || buffer[0] == '#' ) 
		{ 
			/* empty line or comment lie, do nothing but keep status 1 */
			/* shell will clear buffer and re-prompt for input */
			status = 1;
		}
		else 
		{
			/* otherwise, parse the input buffer, look for built in commands
			 * if no built-ins are found, exec the arguments */
			args = parse_buffer(buffer);
			if (strcmp(args[0], "exit") == 0) 
			{
				status = 0;
			} 
			else if (strcmp(args[0], "cd") == 0) 
			{
				status = proc_cd(args);
			} 
			else if (strcmp(args[0], "status") == 0) 
			{
				status = proc_status(args);
			} 
			else {
				status = proc_exec(args);				
			}
			free(args);
		}
		memset(buffer,'\0',sizeof(buffer));
	
	}

	free(buffer);
	return 0;
}

char **parse_buffer(char *input_buffer)
{
	int numElems = 512;
	int i = 0;
	char** tokens = malloc(numElems * sizeof(char*));
	char* token;


	token = strtok(input_buffer," ");
	while (token != NULL) {
		tokens[i] = token;
		i++;
		token = strtok(NULL," ");
	}
	tokens[i] = NULL;
	return tokens;
}


int proc_cd(char **args) 
{
	if (args[1] == NULL) 
	{
		/* no argument, go to home directory */
	} 
	else 
	{
		/* with an argument, change directories to arg[1] */
		if(chdir(args[1]) != 0) { perror("smallsh: proc_cd"); }
	}
	return 1;
}

int proc_status(char **args)
{

	return 1;
}

int proc_exec(char **args)
{
	return 1;
}



void catchSIGINT( int signo )
{
	char* message = "SIGINT. Use CTRL-Z to Stop.\n";
	write(STDOUT_FILENO, message, 28);
}
