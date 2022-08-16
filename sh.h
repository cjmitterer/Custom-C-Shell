
#include "get_path.h"

int pid;
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
void where(char *command, struct pathelement *pathlist);
void list (char **args);
void printenv(char **args);
void cd(char **args,char *cowd);
void prompt_func(char **args,char *prompt);
void kill_func(char **args);
void setenv_func(char **args);

#define PROMPTMAX 32
#define MAXARGS 10
