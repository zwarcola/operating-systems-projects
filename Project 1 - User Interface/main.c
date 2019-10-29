//
//  project1
//  main.c
//  
//
//  Alex Benasutti
//  CSC345
//  September 20 2019
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>

#define MAX_LINE 80 /* The maximum length command */

int main(void)
{
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    char str[MAX_LINE/2 + 1];   /* placeholder for shell input */
    char *token;                /* holds string for argument parsing */
    char *hist = "No recent commands in history";   /* holds previous command history */
    char *cmd;                  /* holds arguments to be parsed */
    int should_run = 1;         /* flag to determine when to exit program -- user must run "exit" */
    int background_process;     /* flag to determine when to run a process in the background -- & */
    int redirect_out;           /* flag to determine file redirection OUT -- > */
    int redirect_in;            /* flag to determine file redirection IN  -- < */
    int pipe_process;           /* flag to determine if the pipe operator is used */ 
    int argc;                   /* counts the number of arguments */
    int len;                    /* counts the length of the input submitted by the user */
    int pipefd[2];              /* pipe file descriptor */
    
    pid_t pid;                  /* process id */
    pid_t pid_pipe;             /* process id for pipe */

    while (should_run)
    {
        /* Reset shell flags and argc for next command */
        background_process = 0;
        redirect_out       = 0;
        redirect_in        = 0;
        pipe_process       = 0;
        argc               = 0;
        
        /* set up user input and flush stdout */
        printf("osh>");
        fflush(stdout);

        /* get user input */
        fgets(str,MAX_LINE,stdin);
        len = strlen(str);

        /* allocate memory for command string */
        cmd = malloc(MAX_LINE/2+1 * sizeof(char));

        /* set newline character '\n' to NULL */
        if (str[len-1] == '\n') str[len-1] = '\0'; 

        /*
        * execute previous command
        * echo command on users screen
        * command should be placed into history buffer as next command
        * basic error handling i.e. no commands in history
        */

        if (strcmp(str,"!!") == 0)
        {
            /* process previously submitted command */
            cmd = hist;
            hist = malloc(strlen(cmd) * sizeof(char));
            memcpy(hist,cmd,strlen(cmd));
            printf("%s\n", cmd);
        }
        else if (str[0] != '\n' && str[0] != '\0')
        {
            /* process command as normal */
            cmd = str;
            hist = malloc(MAX_LINE/2+1 * sizeof(char));
            memcpy(hist,str,len);
        }
            
        /* tokenize the command */
        token = strtok(cmd," ");
        
        while (token != NULL)
        {
            /* set each token to a different element of args */
            args[argc] = token;

            /* Find any pipes - can't set | to NULL just yet since that would terminate the loop */
            if (strcmp(token,"|") == 0) pipe_process = argc;

            token = strtok(NULL, " ");
            argc++;
        }
        args[argc] = '\0'; // set final argument to NULL so execvp() may terminate
        
        /* exit only if met as the first argument */
        if (strcmp(args[0],"exit") == 0) 
        {
            should_run = 0;
            // break; honestly you should have just required a break statement
        }

        /* directory change (cd) only if met as the first argument */
        if (strcmp(args[0],"cd") == 0) 
        {
            chdir(args[1]);
        }

        /* background process handling only if met as the last argument */
        if (strcmp(args[argc-1],"&") == 0) 
        {
            background_process = 1;
            args[argc-1] = '\0';
        }

        /* redirection handling for 2nd to last argument i.e. [some lengthy command] > out.txt */
        if (argc > 2)
        {
            if (strcmp(args[argc-2],">") == 0)
            {
                args[argc-2] = '\0';
                redirect_out = 1;
            } 
            else if (strcmp(args[argc-2],"<") == 0) 
            {
                args[argc-2] = '\0';
                redirect_in = 1; 
            }
        }
        
        /* time for fork our parents and children */
        pid = fork();
        
        if (pid == 0) // CHILD PROCESS #1
        {
            if (argc > 0)
            {
                /* 
                 * use > or < redirection operators for filestream management - use dup2()
                 * assume commands will only contain one operator or the other and not both
                 */

                if (redirect_out)
                {
                    // redirect data out
                    int fdOut;
                    if ( (fdOut = creat(args[argc-1], 0644)) < 0 )
                    {
                        printf("File error - could not create file.\n");
                        exit(0);
                    }
                    dup2(fdOut, STDOUT_FILENO);
                    close(fdOut);
                }
                else if (redirect_in)
                {
                    // redirect data in
                    int fdIn;
                    if ( (fdIn = open(args[argc-1], O_RDONLY)) < 0 )
                    {
                        printf("File error - could not open file.\n");
                        exit(0);
                    }
                    dup2(fdIn, STDIN_FILENO);
                    close(fdIn);
                }

                /* 
                * use | pipe to allow output of one command serve as input for another
                * have child create another child and use dup2()
                * assume one pipe per command and NO redirection operators
                */

                if (pipe_process)
                {
                    /* create the pipe and handle any errors it presents */
                    if (pipe(pipefd) < 0)
                    {
                        printf("Failed to create the pipe.\n");
                        exit(1);
                    }

                    /* fork another process for the child to execute */
                    pid_pipe = fork();

                    if (pid_pipe == 0) // CHILD PROCESS #2
                    {
                        /* redirection for STDOUT to pipe */
                        dup2(pipefd[1], STDOUT_FILENO);
                        args[pipe_process] = '\0';
                        close(pipefd[0]);

                        /* execute that first argument */
                        execvp(args[0],args);
                    }
                    else if (pid_pipe > 0) // PARENT PROCESS #2
                    {
                        wait(NULL);

                        /* redirection for STDIN to pipe */
                        dup2(pipefd[0],STDIN_FILENO);
                        close(pipefd[1]);

                        /* execute those args after the pipe "|" */
                        execvp(args[pipe_process + 1], &args[pipe_process + 1]);
                    }
                }
                else /* invoke execvp() */
                    execvp(args[0],args);
            }
            else
            {
                fprintf(stderr,"You have zero arguments.\n");
            }
        }
        else if (pid > 0) // PARENT PROCESS #1
        {
            /* invoke wait unless command included & */
            if (!background_process) wait(NULL);
        }
    }
    return 0;
}
