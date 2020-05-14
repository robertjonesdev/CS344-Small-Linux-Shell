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


/* Function Definitions */
char**	parse_buffer(char *, int *);
int 	proc_cd(char **);
int 	proc_status(int, int);
int 	proc_exit(char **);
int 	proc_exec_foreground(char **, int*);
int 	proc_exec_background(char **);

void	storePid(pid_t);
void	trashPid(pid_t);
pid_t	getStoredPid();

void 	catchSIGTSTP(int );
void	catchSIGCHLD(int );
void	catchSIGUSR2(int );

/* Global Variables */
int 	allow_background = 1;
pid_t	pidList[100] = { 0 };

int main ()
{
	/* SIGNAL ACTIONS */
	/* SIGSTOP: This will catch Ctl-Z to turn on/off the background process command */
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = catchSIGTSTP;
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigfillset(&SIGTSTP_action.sa_mask);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	/* Set SIGINT to Ignore by default. A foreground exec process */
	signal(SIGINT, SIG_IGN);

	/* SIGCHLD: This will catch exit signals from child proceses and send to handler */
	struct sigaction SIGCHLD_action = {0};
	SIGCHLD_action.sa_handler = catchSIGCHLD;
	SIGCHLD_action.sa_flags = SA_RESTART;
	sigfillset(&SIGCHLD_action.sa_mask);
	sigaction(SIGCHLD, &SIGCHLD_action, NULL);
	
	/* Declare command line input buffer */
	char *buffer;
	size_t bufsize = 2069;
	size_t characters;
	buffer = (char *)malloc(bufsize * sizeof(char));
	memset(buffer, '\0', sizeof(buffer));

	
	char **args; /* character array for command arguments */
	int shell_status = 0; /* used for exiting the program */
	int exec_status = 0;  /* processes exit status */
	int exit_or_term = 0; /* process exit type */
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
			{       /* call exit function, kill all processes
				 * return with shell_status = -1 and end shell program */
				shell_status = proc_exit(args);
			} 
			else if (strcmp(args[0], "cd") == 0) 
			{
				/* built-in function to change directories */
				shell_status = proc_cd(args);
			} 
			else if (strcmp(args[0], "status") == 0) 
			{
				/* built-in function to display previous exit status */
				shell_status = proc_status(exec_status, exit_or_term);
			} 
			else 
			{
				/* background execute command */
				if (*args[argc] == '&') 
				{
					args[argc] = NULL;
					argc--;
					/* if background processes have been turned off, 
					 * execute as if it were a foreground process 
					 * otherwise execute as background */
					if (allow_background) {
						shell_status = proc_exec_background(args);
					} else {
						exec_status = proc_exec_foreground(args, &exit_or_term);
					}
				}
				else 
				{
					/* execute as a foreground process because no &
					 * was provided at the last argument */
					exec_status = proc_exec_foreground(args, &exit_or_term);	
				}
			}
			free(args);
		}
		memset(buffer,'\0',sizeof(buffer));
	
	} while(shell_status != -1);

	free(buffer);
	return 0;
}


/**************************************
 * This procedure prases the input buffer arguments while also expanding
 * $$ into the shell pid 
 * Receives: character string, reference to integer for the number of arguments
 * Returns: arragy of strings, and the number of elements in that array
 ******************************************/
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

	/* maximum number of argument items is 512 */
	int numElems = 512;
	int i = 0;
	/* allocate memory here for tokens, memory is free'd at the end of the 
	 * execution loop in main() */
	char** tokens = malloc(numElems * sizeof(char*));
	char* token;

	/* arguments are tokens separated by a space */
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


/***********************************
*
* Change Directory
* This procedure changed the current working directory of the shell
* If no argument is given, then it changes directory to the environment 
* variable home directory
* Receives: array of arguments
* Returns: none
*
*************************************/
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


/***********************************************
 * 
 * Status
 * This procedure displays the last exit or terminating status.
 * If no processes has exited or terminated, it will display 0.
 * Receives: status values, exit or term value
 * Returns: none
 * *********************************************/
int proc_status(int status, int exit_or_term)
{
	if (exit_or_term == 0)
	{
		printf("exit status %d\n", status);
		fflush(stdout);
	} 
	else
	{
		printf("terminated by signal %d\n", status);
		fflush(stdout);
	}
	return 1;
}


/***********************************************
 *
 * Exit
 * This procedure loops through all the active background processes 
 * and kills them. Then returns a shell status of -1 to end the program.
 *
 ************************************************/
int proc_exit(char **args)
{
	/* REQ: Kill any other processes running */
	/* Loop through all ongoing background processes */
	pid_t pid = getStoredPid();
	while (pid != 0)
	{
		kill(pid, SIGKILL);
		trashPid(pid);
		pid = getStoredPid();
	}

	return -1;
}


/*********************************************************
 *
 * Execute Foreground process
 * This procedure executes a foreground process
 * Receives argument list
 * Returns: Status and if it was exit or terminated
 * ********************************************************/
int proc_exec_foreground(char **args, int *exit_or_term)
{
	/* first parse the argument array for input file, output file 
	 * or a background process flag. If any are found, remove them from
	 * the argument array by assigning NULL */
	int i = 0, redirect_in = 0, redirect_out = 0;
	
	/* create string for input and output file names */
	char input_file[100]; /* < */
	memset(input_file, '\0', 100);
	char output_file[100]; /* > */
	memset(output_file, '\0', 100);
	
	/* scan through the argument list and look for redirection symbols
	 * if found, then copy the next argument to the input or output file 
	 * buffers */
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


	/* Spawn a child process*/
	pid_t spawnPid = -5;
	pid_t waitPid = -5;
	int childExitStatus = -5;
	int sourceFD, targetFD;
	
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGTSTP);
	sigprocmask(SIG_BLOCK, &sigset, NULL);


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
			/* return sigint status to default to catch sigint  as normal */
			;
			struct sigaction default_action = {0};
			default_action.sa_handler = SIG_DFL;
			sigaction(SIGINT, &default_action, NULL);
			
			struct sigaction ignore_action = {0};
			ignore_action.sa_handler = SIG_IGN;
			sigaction(SIGTSTP, &ignore_action, NULL);

			//signal(SIGINT, SIG_DFL);
			//signal(SIGTSTP, SIG_IGN);			
			/* if there is file redirection, open the file */
			/* and redirect standard in */
			if (redirect_in == 1)
			{
				sourceFD = open(input_file, O_RDONLY);
				if (sourceFD == -1) { perror("smallsh: cannot open"); exit(1); }
				int result = dup2(sourceFD, 0);
				if (result == -1) { perror("smallsh: cannot redirect"); exit(1); }
				fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
			}
			/* if there is file redirection for output, open the file
			 * and redirect standard out */
			if (redirect_out == 1)
			{
				targetFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (targetFD == -1) { perror("smallsh: cannot open"); exit(1); }
				int result = dup2(targetFD, 1);
				if (result == -1) { perror("smallsh: cannot redirect"); exit(1); }
				fcntl(targetFD, F_SETFD, FD_CLOEXEC);
			}
			/* execute the command and catch an error */
			if (execvp(args[0], args) == -1) {
				perror("smallsh");
			}
			exit(EXIT_FAILURE);
			break;
		default:
			/* parent */

			/* loop while waiting for the foreground process to terminate
			 * catch the pid and evaluate the exit status */
			do {
				waitPid = waitpid(spawnPid, &childExitStatus, 0);
				if (WIFEXITED(childExitStatus))
				{
					*exit_or_term = 0;
					sigprocmask(SIG_UNBLOCK, &sigset, NULL);
					return WEXITSTATUS(childExitStatus);
				}
				else if (WIFSIGNALED(childExitStatus))
				{
					*exit_or_term = 1;
					printf("terminated by signal %d\n", WTERMSIG(childExitStatus));
					sigprocmask(SIG_UNBLOCK, &sigset, NULL);
					return WTERMSIG(childExitStatus);
				}
			} while (!WIFEXITED(childExitStatus) && !WIFSIGNALED(childExitStatus));
			
			
			break;
	}
	return 1;
}

/***************************************************
 *
 * Exec Background Process
 * This procedure executes a background process
 * Receives: argument list
 * Returns: none
 ***************************************************/
int proc_exec_background(char **args)
{
	/* first parse the argument array for input file, output file 
	 * or a background process flag. If any are found, remove them from
	 * the argument array by assigning NULL */
	int i = 0, redirect_in = 0, redirect_out = 0;
	
	/* create buffers for any file redirection for file names */
	/* by default these will be set to /dev/null bu twill be overwritten 
	 * if there is any provided file redirection */
	char input_file[100]; /* < */
	memset(input_file, '\0', 100);
	strcpy(input_file, "/dev/null");
	char output_file[100]; /* > */
	memset(output_file, '\0', 100);
	strcpy(output_file, "/dev/null");
	
	/* loop through the argument list looking for any provided redirection */
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

	pid_t spawnPid = -5;
	pid_t waitPid = -5;
	int childExitStatus = -5;
	int sourceFD, targetFD, result;

	/* spawn a child process */
	spawnPid = fork();
	switch (spawnPid)
	{
		case -1:
			/* error */
			perror("smallsh");
			exit(1);
			break;
		case 0:;
			/* child */

			/* Ignore signal tstp (Ctl-Z) */
			struct sigaction ignore_action = {0};
			ignore_action.sa_handler = SIG_IGN;
			sigaction(SIGTSTP, &ignore_action, NULL);


			/* redirection will occur whether default or provided */
			/* if no redirection argument is given then input and output are 
			 * directed to/from /dev/null */
			sourceFD = open(input_file, O_RDONLY);
				if (sourceFD == -1) { perror("smallsh: source open()"); exit(1); }
				result = dup2(sourceFD, 0);
				if (result == -1) { perror("smallsh: source: dup2()"); exit(1); }
				fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
		
			targetFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (targetFD == -1) { perror("smallsh: target open()"); exit(1); }
				result = dup2(targetFD, 1);
				if (result == -1) { perror("smallsh: target: dup2()"); exit(1); }
				fcntl(targetFD, F_SETFD, FD_CLOEXEC);
			
			/* execute the process and catch error */
			if (execvp(args[0], args) == -1) {
				perror("smallsh");
			}
			exit(EXIT_FAILURE);
			break;
		default:
			/* parent */
			
			/* since this is a background process, do you not wait for it
			 * to terminate, intsead store the pid for future catch SIGCHLD
			 * or cleanup (exit) */
			storePid(spawnPid);
			printf("background pid is %ld\n", (long)spawnPid);
			fflush(stdout);
			break;
	}
	return 1;
}


/******************************************
 * SIGTSTP signal handler
 * This procedure will catch the SIGSTSP and toggle between
 * foreground-only mode and allow-foreground mode. 
 * The shell stores the toggle value as a global integer.
****************************************/
void catchSIGTSTP( int signo )
{
	if (allow_background)
	{
		allow_background = 0;
		char* message = "\nEntering foreground-only mode (& is now ignored)\n:";
		write(STDOUT_FILENO, message, 51);
	}
	else 
	{
		allow_background = 1;
		char* message = "\nExiting foreground-only mode\n:";
		write(STDOUT_FILENO, message, 31);
	}
	return;
}



/***************************************************
 * SIGCHLD signal handler
 * This procedure will catch the SIGCHLD signal handler when a 
 * child process exits. Foreground processes are caught with the waitpid in the foreground
 * exec procedure. Therefore this will only catch background processes as they exit.
 * *************************************************/
void catchSIGCHLD( int signo )
{

	pid_t pid;
	int status;
	/* check for pid of any process that is exited, but do not wait. */
	while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
	{
		/* remove the pid from the list of active background pids */
		trashPid(pid);
		/* print to the screen the process id of the exiting process */
		write(STDOUT_FILENO, "\nbackground pid ",16); 
		char pid_buffer[7];
		memset(pid_buffer, '\0', 7);
		snprintf(pid_buffer, 7, "%ld", (long)pid); 
		write(STDOUT_FILENO, pid_buffer,7);
		write(STDOUT_FILENO, " is done: ",10);

		/* print the exit or termination status */
		if (WIFEXITED(status))
		{
			write(STDOUT_FILENO, "exit status ",12);
			char status_buffer[7];
			memset(status_buffer, '\0', 7);
			snprintf(status_buffer, 7, "%d", WIFEXITED(status));
			write(STDOUT_FILENO, status_buffer, 7);
			write(STDOUT_FILENO, "\n:", 2);
		}
		else if (WIFSIGNALED(status))
		{
			write(STDOUT_FILENO, "terminated by signal ",21);
			char status_buffer[7];
			memset(status_buffer, '\0', 7);
			snprintf(status_buffer, 7, "%d", WTERMSIG(status));
			write(STDOUT_FILENO, status_buffer, 7);
			write(STDOUT_FILENO, "\n:", 2);
		}
		 
	}
	return;

}

/********************************
 * Helper functions for storing, deleting, and retreiving the global array of
 * active background processes. This is primarily used for the exit function
 * to kill any active processes 
 * *********************************/
void storePid(pid_t pid)
{
	int i = 0;
	while (pidList[i] != 0 && i < 100) { i++; }
	pidList[i] = pid;
	return;
}

void trashPid(pid_t pid)
{
	int i = 0;
	while (pidList[i] != pid && i < 100) { i++; }
	while (pidList[i] != 0 && i < 99) { 
		pidList[i] = pidList[i+1]; 
		i++;	
	}
	pidList[i] = 0;
	return;
}

pid_t getStoredPid()
{
	int i = 0;
	while (i <= 100 && pidList[i] != 0) {
		i++;
	}
	pid_t returnVal = pidList[i-1];
	return returnVal;

}
