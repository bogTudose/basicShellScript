/*
 * source.c
 *
 *  Created on: Aug 21, 2017
 *      Author: bogdan
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
#define RESET_COLOR "\e[m"
#define MAKE_GREEN "\e[32m"
#define MAKE_BLUE "\e[36m"



int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_ls(char **args);

char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "ls"
};

int (*builtin_func[]) (char **) =
{
		&lsh_cd,
		&lsh_help,
		&lsh_exit,
		&lsh_ls
};

int lsh_num_builtins()
{
	return sizeof(builtin_str) / sizeof(char*);
}

int lsh_cd(char **args)
{
	if(args[1] == NULL)
	{
		fprintf(stderr,"lsh: expected argument");
	}
	else
	{
		if(chdir(args[1]) != 0)
		{
			perror("lsh");
		}
	}
	return 1;
}

int lsh_help(char **args)
{
	int i;
	printf("Bogdan shell\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < lsh_num_builtins(); i++)
	{
		printf("  %s\n", builtin_str[i]);
	}

	printf("Use the man command for information on other programs.\n");
	return 1;
}

int lsh_exit(char **args)
{
	return 0;
}

int lsh_ls(char **args)
{
	char *curr_dir = NULL;
	DIR *dp = NULL;
	struct dirent *dptr = NULL;
	unsigned int count = 0;

	curr_dir = getenv("PWD");
	if(NULL == curr_dir)
	{
		printf("\n ERROR: Could not get the working directory");
		//return -1;
	}

	dp = opendir((const char*)curr_dir);
	if(NULL == dp)
	{
		printf("\n ERROR: Could not open the working the directory");
		//return -1;
	}
	printf("> ");
	for(count = 0; NULL != (dptr = readdir(dp)); count ++)
	{
		if(dptr ->d_name[0] != '.')
		{
			if(!access(dptr->d_name,X_OK))
			{
				int fd = -1;
				struct stat st;
				fd = open(dptr->d_name,O_RDONLY,0);
				if(fd == -1)
				{
					printf("Opening file/directory failed");
				}
				fstat(fd,&st);
				if(S_ISDIR(st.st_mode))
				{
					 printf(MAKE_BLUE"%s     "RESET_COLOR,dptr->d_name);
				}
				else
				{
					 printf(MAKE_GREEN"%s     "RESET_COLOR,dptr->d_name);
				}
				close(fd);
			}
			else
			{
				printf("%s     ",dptr->d_name);
			}
		}
	}
	printf("\n");
	return 1;
}

int lsh_launch(char **args)
{
	pid_t pid, wpid;
	int status;
	pid = fork();
	//printf("My pid is %d", pid);
	if(pid == 0)
	{
		if(execvp(args[0],args) == -1)
		{
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	}
	else if(pid<0)
	{
		perror("lsh");
	}
	else
	{
		do
		{
			wpid = waitpid(pid,&status,WUNTRACED);
		}while(!WIFEXITED(status)&& !WIFSIGNALED(status));
	}
	return 1;

}

int lsh_execute(char **args)
{
	int i;
	if(args[0] == NULL)
	{
		return 1;
	}
	for(i=0;i< lsh_num_builtins();i++)
	{
		if(strcmp(args[0],builtin_str[i]) == 0)
		{
			return (*builtin_func[i])(args);
		}
	}
	return lsh_launch(args);
}

char *lsh_read_line(void)
{
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if(!buffer)
	{
		fprintf(stderr,"lsg: allocation error \n");
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		c = getchar ();
		if(c == EOF || c == '\n')
		{
			buffer[position] = '\0';
			return buffer;
		}
		else
		{
			buffer[position] = c;
		}
		position++;
		if (position >= bufsize)
		{
			bufsize += LSH_RL_BUFSIZE;
		    buffer = realloc(buffer, bufsize);
		    if (!buffer)
		    {
		        fprintf(stderr, "lsh: allocation error\n");
		        exit(EXIT_FAILURE);
		    }
		}
	}
}

char **lsh_split_line(char *line)
{
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char));
	char * token;

	if(!tokens)
	{
		fprintf(stderr, "lsh: allocation error\n");
	}
	token = strtok(line, LSH_TOK_DELIM);
	while (token != NULL)
	{
		tokens[position] = token;
	    position++;

	    if (position >= bufsize)
	    {
	    	bufsize += LSH_TOK_BUFSIZE;
	    	tokens = realloc(tokens, bufsize * sizeof(char*));
	    	if (!tokens)
	    	{
	    		fprintf(stderr, "lsh: allocation error\n");
	    		exit(EXIT_FAILURE);
	    	}
	    }

	    token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

void lsh_loop()
{
	char *line;
	char **args;
	int status;
	do
	{
		printf("> ");
		line = lsh_read_line();
		args = lsh_split_line(line);
		status = lsh_execute(args);

		free(line);
		free(args);
	}while(status);

}


int main(int argc, char **argv)
{
	lsh_loop();
}

