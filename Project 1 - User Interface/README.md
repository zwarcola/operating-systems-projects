# Project 1 - User Interface

A design and implementation of a command-line user interface (CLI) using ```fork()```, ```exec()```, and threads if necessary. Manages command history, input and output redirection, communication of parent and child processes by pipe, and keeps working directory info.

### Task

**Part 1 - Executing commands in a child process**

• By tokenizing a given user input via the command line, we can create a command to execute. This is done by forking a child process and using execvp(). For example, an _args_ array containing the following:

```
args[0]="ps"
args[1]="-ael"
args[2]=NULL
```

can be executed by execvp(args[0], args). The program also checks for certain operators such as redirection (```>``` or ```<```), pipe (```|```), or background (```&```), and deals with them accordingly.

**Part 2 - Creating a history feature**

• A command implemented into the CLI is the history command, _!!_, which executes whatever command was used previously. For example,


```
ls -l
!!
```

will execute ```ls -l``` twice. A simple failsafe is added if there are no commands in the current history.

**Part 3 - Redirecting input and output**

• This CLI also adds support for the ```>``` and ```<``` redirection operators, constantly checking for their existence in the post-processing of user arguments. For example, if the user enters

```
ls > out.txt
```

the output of the ```ls``` command will be outputted to the file ```out.txt```. Similarly, input can be redirected as well. For example, if the user enters

```
sort < in.txt
```
the file ```in.txt``` will serve as the input for the ```sort``` command.

• This part of the program does not account for both operators at once and only assumes that each user command will contain either one input or output redirection and not both.

**Part 4 - Communication via a pipe**

• The shell also allows the output of one command to serve as input to another using a pipe. For example, the following command sequence

```
ls -l | less
```

has the output of the command ```ls −l``` serve as the input to the ```less``` command. Both the ```ls``` and ```less``` commands will run as separate processes and will communicate using the UNIX pipe() function.

• This function is implemented by having a parent process create a child process to execute the first command, and having that child process be the parent of another child for the second command.

• This part of the program assumes that commands will contain only one pipe character and will not be combined with any redirection operators.

**Other**

• Code analysis can be found in ```report.pdf```.
