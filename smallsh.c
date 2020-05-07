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
	size_t bufsize = 2069;
	size_t characters;
	buffer = (char *)malloc(bufsize * sizeof(char));
	memset(buffer, '\0', sizeof(buffer));

	char **args;
	int status = 0;

	do /*while status != 0 */
	{
		while (1) 
		{
			/* Display command prompt and read characters from stdin */
			/* Then remove the new line character at the end */
			printf(":");
			fflush( stdout );
			characters = getline(&buffer, &bufsize, stdin);
			if (characters == -1) {
				clearerr(stdin);
			}
			else {
				buffer[strcspn(buffer, "\n")] = '\0';
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
				status = proc_exit(args);
			} 
			else if (strcmp(args[0], "cd") == 0) 
			{
				status = proc_cd(args);
			} 
			else if (strcmp(args[0], "status") == 0) 
			{
				status = proc_status(args);
			} 
			else 
			{
				status = proc_exec(args);				
//				status = 1;
			}
			free(args);
		}
		memset(buffer,'\0',sizeof(buffer));
	
	} while(status != 0);

	free(buffer);
	return 0;
}

char **parse_buffer(char *input_buffer)
{
	/* find $$ and replace with PID */
	char tmp_buffer[2069];
	char * pch = input_buffer;
	char pid_string[10];
	sprintf(pid_string, "%ld", (long)getpid());
	while (pch = strstr(pch, "$$"))
	{
		/* loop through the buffer and use a temporary buffer to 
		 * split the string in to two around the $$ and piece back
		 * together with the pid inserted. */
		strncpy(tmp_buffer, input_buffer, (pch - input_buffer));
		tmp_buffer[(pch - input_buffer)] = '\0';
		strcat(tmp_buffer, pid_string);
		strcat(tmp_buffer, (pch + 2));
		strcpy(input_buffer, tmp_buffer);
		pch++;
	}


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
		if(chdir(getenv("HOME")) != 0) { perror("smallsh: cd"); }
	} 
	else 
	{
		/* with an argument, change directories to arg[1] */
		/* REQ: takes absolute and relative paths */

		if(chdir(args[1]) != 0) { perror("smallsh: cd"); }
	}
	return 1;
}

int proc_status(char **args)
{

	return 1;
}

int proc_exit(char **args)
{
	/* REQ: Kill any other processes running */

	return 0;
}

int proc_exec(char **args)
{
	
	int j =0;
	while (args[j] != NULL)
	{
		printf("%s\n", args[j]);
		j++;
	}
	/* get redirection */
	int i = 0;
	char input_file[100]; /* < */
	memset(input_file, '\0', 100);
	char output_file[100]; /* > */
	memset(output_file, '\0', 100);
	while (args[i] != NULL && i < 512)
	{
		if (*args[i] =='>')
		{
			if (args[i+1] != NULL)
			{
				strcpy(output_file, args[i+1]);
				args[i] = NULL;
				args[i+1] = NULL;
				i++;
			}
		} else if (*args[i] == '<')
		{
			if (args[i+1] != NULL)
			{
				strcpy(input_file, args[i+1]);
				args[i] = NULL;
				args[i+1] = NULL;
				i++;
			}
		}
		i++;
	}

	printf("%s \t %s\n", input_file, output_file);	
	pid_t spawnPid = -5;
	pid_t waitPid = -5;
	int childExitStatus = -5;

	spawnPid = fork();
	switch (spawnPid)
	{
		case -1:
			/* error */
			perror("smallsh");
			exit(1);
			break;
		case 0:
			/* child */
			if (execvp(args[0], args) == -1) {
				perror("smallsh");
			}
			exit(EXIT_FAILURE);
			break;
		default:
			/* parent */
			do {
			waitPid = waitpid(spawnPid, &childExitStatus, 0);
			} while(!WIFEXITED(childExitStatus) && !WIFSIGNALED(childExitStatus));
			break;
	}

	return 1;
}

void catchSIGINT( int signo )
{
	char* message = "SIGINT. Use CTRL-Z to Stop.\n";
	write(STDOUT_FILENO, message, 28);
}
