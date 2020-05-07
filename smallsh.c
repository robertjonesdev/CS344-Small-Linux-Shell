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
#include <fcntl.h>

char**	parse_buffer(char *, int *);
int 	proc_cd(char **);
int 	proc_status(int);
int 	proc_exit(char **);
int 	proc_exec_foreground(char **);
int 	proc_exec_background(char **, long *);
void 	catchSIGINT(int );

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
	long background_pids[100] = { (long)NULL };
	int shell_status = 0;
	int exit_status = 0;
	
	do /*while status != -1 */
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
			shell_status = 1;
		}
		else 
		{
			/* otherwise, parse the input buffer, look for built in commands
			 * if no built-ins are found, exec the arguments */
			int argc = 0;
			args = parse_buffer(buffer, &argc);
			if (strcmp(args[0], "exit") == 0) 
			{
				shell_status = proc_exit(args);
			} 
			else if (strcmp(args[0], "cd") == 0) 
			{
				shell_status = proc_cd(args);
			} 
			else if (strcmp(args[0], "status") == 0) 
			{
				shell_status = proc_status(exit_status);
			} 
			else 
			{
				if (*args[argc] == '&') 
				{
					args[argc] = NULL;
					argc--;
					shell_status = proc_exec_background(args, background_pids);
				}
				else 
				{
					exit_status = proc_exec_foreground(args);	
				}
			}
			free(args);
		}
		memset(buffer,'\0',sizeof(buffer));
	
	} while(shell_status != -1);

	free(buffer);
	return 0;
}

char **parse_buffer(char *input_buffer, int *argc)
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
	*argc = i - 1;
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

int proc_status(int exit_status)
{
	printf("exit status %d\n", exit_status);
	fflush(stdout);
	return 1;
}

int proc_exit(char **args)
{
	/* REQ: Kill any other processes running */

	return -1;
}

int proc_exec_foreground(char **args)
{
	/* first parse the argument array for input file, output file 
	 * or a background process flag. If any are found, remove them from
	 * the argument array by assigning NULL */
	int i = 0, redirect_in = 0, redirect_out = 0;
	
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
				redirect_out = 1;
				args[i] = NULL;
				args[i+1] = NULL;
				i++;
			}
		} else if (*args[i] == '<')
		{
			if (args[i+1] != NULL)
			{
				strcpy(input_file, args[i+1]);
				redirect_in = 1;
				args[i] = NULL;
				args[i+1] = NULL;
				i++;
			}
		}
		i++;
	}

	int sourceFD, targetFD;
	if (redirect_in == 1)
	{
		sourceFD = open(input_file, O_RDONLY);
		if (sourceFD == -1) { perror("smallsh: source open()"); exit(1); }
		int result = dup2(sourceFD, 0);
		if (result == -1) { perror("smallsh: source: dup2()"); exit(1); }
		fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
	}
	if (redirect_out == 1)
	{
		targetFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (targetFD == -1) { perror("smallsh: target open()"); exit(1); }
		int result = dup2(targetFD, 1);
		if (result == -1) { perror("smallsh: target: dup2()"); exit(1); }
		fcntl(targetFD, F_SETFD, FD_CLOEXEC);
	}
	

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


int proc_exec_background(char **args, long *background_pids)
{

	return 1;
}


void catchSIGINT( int signo )
{
	char* message = "SIGINT. Use CTRL-Z to Stop.\n";
	write(STDOUT_FILENO, message, 28);
}
