/* ____________________________________________________________________________________________________________

	FILE: 		sdshell.c
	AUTHOR:		Shreyansh Doshi
	DATE:		Sunday, 4th October 2015
	BRIEF:		SDSHELL, implementation of a simple unix shell in C
_______________________________________________________________________________________________________________*/
	

#include<stdio.h>  		//for fprintf() , perror() , stderr, getchar(), printf()
#include<stdlib.h>  	//for malloc() , realloc() , free() , exit() , execvp() , EXIT_SUCCESS , EXIT_FAILURE
#include<string.h>  	//for strcmp() , for strtok()
#include<unistd.h>  	//for chdir() , fork() , exec() , pid_t
#include<sys/wait.h> 	//for waitpid() , WUNTRACED , WIFEXITED() , WIFSIGNALED()

#define SDSHELL_TOK_BUFFER 64
#define SDSHELL_TOK_DELIM " \t\r\n\a"

int sdshell_cd(char** args);
int sdshell_help(char** args);
int sdshell_exit(char** args);

char* builtin_str[] = {"cd" , "help" , "exit"};

//	declaration of function pointers. 
int (*builtin_functions[])(char**) = {
	&sdshell_cd,
	&sdshell_help,
	&sdshell_exit
}

//	cd: Change directory is also a builtin function.
//	the reason why cd is a buitin function is simple. when a new process is created, the parent process forks a child process 
//	and once the execution of the child process terminates, the parent process continues execution. In case of cd, the child process 
//	will change its current directory, but this change will not be reflected to the parent directory, which is not what we want.
int sdshell_cd(char** args)
{
	if(args[1] == NULL)
	{
		fprintf(stderr, "sdshell: Expected arguement to \"cd\"\n");
	}
	else
	{
		if(chdir(args[1]) != 0)
		{
			perror("sdshell");
		}
	}
	return 1;
}

// help is a builtin function which displays all 3 built in commands.
int sdshell_help(char** args)
{
	int i;
	printf("Type command name and arguement(s), and hit ENTER.\n");
	printf("The following are built in:\n");

	for(i=0;i<3;i++)
	{
		printf("%s\n", builtin_str[i] );	
	}

	printf("Use the man command for information on other commands. Happy Linux :)\n");
	return 1;
}


//	for exit command. Terminates process.
int sdshell_exit(char** args)
{
	return 0;
}

//	This function uses 3 system calls : fork(), execvp() , waitpid().
//	To execute a command in linux, we use the fork() system call, which creates a copy of the parent process.
//	If a fork() call returns 0, it denotes child process. non-zero return value denotes parent process.
//	execvp() system call executes the command as a seperate(child) process. returns 1 on success.
//	waitpid() system call allows the parent process to wait for its child processes to terminate.
int sdshell_launch(char** args)
{
	pid_t pid,wpid;
	int status;

	pid = fork();
	if(pid == 0) 
	{
		if(execvp(args[0] , args) == -1)
		{
			perror("sdshell");
		}
		exit(EXIT_FAILURE);	
	}
	else if(pid < 0)
	{
		perror("sdshell");
	}
	else
	{
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		}
		while( !WIFSIGNALED(status) && !WIFEXITED(status) );
	}
	return 1;
}

//	this function checks whether args[0] i.e. the program to be executed is an builtin function.
//	if it is an buitin function, then respective buitin function is called.
//	else, it calls the sdshell_launch() function.
int sdshell_execute(char** args)
{
	int i;
	if(args[0] == NULL)
	{
		return 1;
	}

	for(i=0;i<3;i++)
	{
		if(strcmp(args[0],builtin_str[i]) == 0)
		{
			return (*builtin_functions[i])(args);
		}
	}

	return sdshell_launch(args);
}

//	this function splits the std input into "tokens", which is a pointer to a character pointer "token".
//	to take token, i am using the strtok() which is a "string.h" library function.
//	Initially, memory allocated to tokens is 64 bytes, but if the arguements exceed the memory limit, additional memory 
//	will be reallocated using the realloc() lib function.
//	This function returns a pointer to a character pointer which points to args[0]. 
char** sdshell_split_line(char* line)
{
	int buffer_size = SDSHELL_TOK_BUFFER;
	int position = 0;
	int token_length;
	char* token;
	char* token_copy;

	char** tokens = malloc(buffer_size*sizeof(char));

	if(!tokens)
	{
		fprintf(stderr, "sdshell: allocation error.\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, SDSHELL_TOK_DELIM);

	while(token != NULL)
	{	
		token_length = strlen(token);
		token_copy = malloc((token_length+1)*sizeof(char));
		strcpy(token_copy , token);

		tokens[position] = token_copy;
		position++;

		if(position <= SDSHELL_TOK_BUFFER)
		{
			buffer_size += SDSHELL_TOK_BUFFER;
			tokens = realloc(tokens , buffer_size);
			if(!tokens)
			{
				fprintf(stderr, "sdshell: allocation error.\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL , SDSHELL_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}


//  this function will read the standard input ( command + arguement) form the console.
//  Using the getline() function. it will dynamically allocate buffer size.
//	This function will return a char pointer which will point to the starting block of std input.
char *sdshell_read_line(void)   
{
  char *line = NULL;
  ssize_t buffer_size = 0;
  getline(&line, &buffer_size, stdin);
  return line;
}

//	this function will free all the allocated memory to the args from each loop.
//	actually the data type of args is char** i.e. pointer to a character pointer.so it will free all the memory that is pointed
//	by args and then it will free itself.
void sdshell_free_args(char** args)
{
	char** i = args;
	while(*i != NULL)
	{
		free(*i);
		i++;
	}
	free(args);
}

//	this function will loop until an exit command is encountered.
void sdshell_loop(void)
{
	char* line;
	char** args;
	int status;

	do{
		printf("> ");
		line = sdshell_read_line();         
		args = sdshell_split_line(line);
		status = sdshell_execute(args);

		free(line);
		sdshell_free_args(args);
	}
	while(status);
}

//	this will print the fancy welcome board.
void sdshell_starup(void) 
{	
	int i;
	printf("\n");
	for(i=0;i<80;i++)
	{
		printf("-");
	}
	printf("\n");
	printf("\t\t| Welcome to Shreyansh Doshi's SDSHELL. |\n");
	for(i=0;i<80;i++)
	{
		printf("-");
	}
	printf("\n");
	printf("Type command name and arguement(s), and hit ENTER.\n");
	printf("The following are built in:\n");

	for(i=0;i<3;i++)
	{
		printf("%d . %s\n",i+1, builtin_str[i] );
	}

	printf("Use the man command for information on other commands. Happy Linux :)\n");
}

// main function.
int main(int argc , char** argv)
{	
	sdshell_starup();
	sdshell_loop();
	return EXIT_SUCCESS;
}
