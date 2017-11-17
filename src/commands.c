#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>

#include "commands.h"
#include "built_in.h"

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};
typedef struct {
	unsigned int _sigbits[4];
}sigset_t;


static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);
 

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1;// not found.

  
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);

    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1)
    {
	 if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv))
	 {
       		 if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0)
		 {
         		 fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
       		 }
     	 }
	 else
	 {
       		 fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
       		 return -1;
     	 }
    }
    else if (strcmp(com->argv[0], "") == 0)
    {
   	 return 0;
    }
    else if (strcmp(com->argv[0], "exit") == 0)
    {
   	 return 1;
    }
    else
    {
	 //printf("we have to do is %s\n",com[0].argv[0]);
	 int is_back = 0;
	 int status;
	 int pid;
	 int pfd[2];
	 sigset_t set;
	 for(int i=0; i < n_commands; i++)
	 {
		//printf("wait... %d\n",com[i].argc);
		for(int t=0; t<com[i].argc; t++)
		{
			if(!strcmp(com[i].argv[t],"&"))
			{
				is_back=1;
				com[i].argv[t] = NULL;
				break;
				//printf("background command!\n");
			}
		}
	 }
	 int i=0;
	 pid = fork();
	 if(pid == -1)
	 {
		perror("fork error");
		exit(1);
	 }
	 else if(pid == 0)
	 {			

		for(i=0; i<n_commands-1; i++)
		{
			pipe(pfd);
			switch(fork())
			{
				case -1:
					perror("fork error");
					exit(1);
				case 0:
					close(pfd[0]);
					dup2(pfd[1], STDOUT_FILENO);
					execv(com[i].argv[0], com[i].argv);
					perror("exec error");
					exit(1);
				default:
					close(pfd[1]);
					dup2(pfd[0], STDIN_FILENO);
			}
		}
		intmax_t PID = getpid();
			
		if(is_back == 1)
			printf("%jd\n",PID);
		
                switch(fork())
                {
                        default:
                        execv(com[i].argv[0],com[i].argv);
                        perror("exec error");
                        _exit(0);

			case 0:
				if(is_back == 1)
				{
                        		waitpid(pid,&status,WUNTRACED);
                        		printf("%jd Done.\n",PID);
				}
				exit(0);
		}
	 }
	 else if(pid != 0 && is_back == 0)
	 {
		waitpid(pid,&status,WUNTRACED);
	 }
   	 return 0;
    }
  }
  return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
