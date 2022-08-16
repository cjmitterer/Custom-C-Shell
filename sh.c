//Authors: Patrick Taylor, CJ Mitterer(In other Section)

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sh.h"
#include <glob.h>

#define MAXLINE 128

extern char **environ;

int sh( int argc, char **argv, char **envp )
{
 // char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
 // char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
 // prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();


  while ( go )
  {
    /* print your prompt */
     char prompt[PROMPTMAX];
     strcpy(prompt, ">>\0");
     printf("%s ",prompt);
    /* get command line and process */
    char buf[MAXLINE];
    while(fgets(buf, MAXLINE, stdin) != NULL) {
	if(buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = 0;
	char *temp;
	char *args[MAXARGS];
	int num = 0;
	temp = strtok(buf, " ");
	while (temp != NULL && num < MAXARGS) {
	    args[num] = temp;
	    num++;
	    temp = strtok(NULL, " ");
	}if(args[0] != EOF){
	args[num] = (char *)NULL;
       /* check for each built in command and implement */
        if(strcmp(args[0], "exit") == 0) {
	    printf("Executing built-in %s\n",args[0]);
	    free(commandline);
	    free(owd);
	    free(pwd);
	    while(pathlist->next != NULL){
                struct pathelement *tmp = pathlist;
                pathlist = pathlist->next;
                free(tmp);
             }
	    free(pathlist);
	    exit(0);
        }
	else if(strcmp(args[0], "pwd") == 0) {
	    printf("Executing built-in %s\n",args[0]);
	    char *tmp = getcwd(NULL,0);
	    printf("%s\n",tmp);
	    free(tmp);
	}
	else if(strcmp(args[0], "pid") == 0) {
            printf("Executing built-in %s\n",args[0]);
	    printf("%d\n",getpid());
	}
	else if(strcmp(args[0], "cd") == 0) {
            printf("Executing built-in %s\n",args[0]);
	    //strcpy(owd,cd(args,owd));
	    cd(args,owd);
	}
	else if(strcmp(args[0], "prompt") == 0){
            printf("Executing built-in %s\n",args[0]);
	    prompt_func(args,prompt);
	}
	else if(strcmp(args[0], "list") == 0) {
            printf("Executing built-in %s\n",args[0]);
	    list(args);
   }
	else if(strcmp(args[0], "which") == 0) {
            printf("Executing built-in %s\n",args[0]);
	    int i = 1;
	    while(args[i] != NULL){
		struct pathelement *path = get_path();
		if(which(args[i],path) != NULL){
			printf("[%s]\n",which(args[i],path));
		}
		i++;
		while(path->next != NULL){
                struct pathelement *tmp = path;
                path = path->next;
                free(tmp);
             }
             free(path);
	    }
	}
	else if(strcmp(args[0], "where") == 0) {
            printf("Executing built-in %s\n",args[0]);
            int i = 1;
            while(args[i] != NULL){
		struct pathelement *path = get_path();
		where(args[i],path);
		i++;
	     while(path->next != NULL){
		struct pathelement *tmp = path;
		path = path->next;
	     	free(tmp);
	     }
	     free(path);
	   }
        }

	else if(strcmp(args[0], "kill") == 0) {
            printf("Executing built-in %s\n",args[0]);
	   kill_func(args);
	}
	else if(strcmp(args[0] , "printenv") == 0) {
            printf("Executing built-in %s\n",args[0]);
	    printenv(args);
	}
	else if(strcmp(args[0], "setenv") == 0) {
            printf("Executing built-in %s\n",args[0]);
	    setenv_func(args);
	}
	else{
		pid_t pid;
		if((pid = fork()) < 0) {
			puts("ERROR");
		}  else if(pid == 0) {
			char    *execargs[MAXARGS];
			glob_t  paths;
			int     csource, j;
			char    **p;
			execargs[0] = malloc(strlen(args[0])+1);
			strcpy(execargs[0], args[0]);  // copy command
			j = 1;
                   for (i = 1; i < num; i++) { // check arguments
                          if (strchr(args[i], '*') != NULL || strchr(args[i], '?') != NULL) { // checks each argument for a wildcard
                            csource = glob(args[i], 0, NULL, &paths); //calls glob to expand wildcards and put them into the path varible
                            if (csource == 0) { //checks  glob error
                              for (p = paths.gl_pathv; *p != NULL; ++p) { //parses paths and puts them into the execargs vaible which is whats used when execv is called
                                execargs[j] = malloc(strlen(*p)+1);
                                strcpy(execargs[j], *p);
                                j++;
                              }

                              globfree(&paths); //frees the glob path
                            }
                          }
                          else { //if the arg didnt have a wildcard then copy it into the execargs valible
                            execargs[j] = malloc(strlen(args[i]) + 1);
                            strcpy(execargs[j], args[i]);
                            j++;
                          }
                        }
                        execargs[j] = NULL; //null the end of the list of args
			struct stat sb;
			struct pathelement *path = get_path();
			if((args[0][0] == '.' || args[0][0] == '/') && access(args[0], X_OK) == 0 && stat(args[0], &sb) == 0 && S_ISREG(sb.st_mode)){
				printf("Executing %s\n",args[0]);
				execve(args[0],execargs,environ);
			}else if(which(args[0],path) != NULL){
				printf("Executing %s\n",which(args[0],path));
				execve(which(args[0],path),execargs,environ);
			}else {
			     printf("%s: Command not found\n",args[0]);
			     return -1;
			}
		}
		else {
			waitpid(pid, NULL, 0);
		}
	}
        printf("%s ",prompt);
      }
  }
}
  return 0;
} /* sh() */

/*
 name: which
 desc: finds the first path list for the given command
 params: char *command the command path to find, struct pathelement *pathlist the current whole path list
 returns: path list of the command
 side effects: none
*/


char * which(char *command, struct pathelement *pathlist )
{
   /* loop through pathlist until finding command and return it.  Return
   NUL*/
   char *path;
   while (pathlist) {         // WHICH
    sprintf(path, "%s/%s", pathlist->element, command);
    if (access(path, X_OK) == 0) {
      break;
    }
    pathlist = pathlist->next;
  }
  if (access(path, X_OK) == 0) {
     return path;
 }else {
  return NULL;
 }
} /* which() */

/*
 name: where
 desc: prints all the  path lists for the given command
 params: char *command the command path to find, struct pathelement *pathlist the current whole path list
 returns: nothing
 side effects: none
*/


void where(char *command, struct pathelement *pathlist )
{
  /* similarly loop through finding all locations of command */

   char *path;
   while (pathlist) {         // WHICH
    sprintf(path, "%s/%s", pathlist->element, command);
    if (access(path, X_OK) == 0) {
      printf("[%s]\n", path);
    }
    pathlist = pathlist->next;
  }

} /* where() */

/*
 name: list
 desc: lists the files in each directory given as an argument
 params: char **args the argument passed to the command
 returns: returns nothing
 side effects: none
*/


void list (char **args)
{
  if(args[1] == NULL) {
                char *cwd = getcwd(NULL,0);
                DIR *dfd = opendir(cwd);
                struct dirent *temp = readdir(dfd);
                while(temp != NULL){
                    printf("%s\n", temp->d_name);
                    temp = readdir(dfd);
                }
                closedir(dfd);
                 free(cwd);
            }
            else {
                int i = 1;
                while(args[i] != NULL){
                    printf("\n%s:\n",args[i]);
                    DIR *dfd = opendir(args[i]);
                    if(dfd != NULL){
                        struct dirent *temp = readdir(dfd);
                        while(temp != NULL){
                                printf("%s\n", temp->d_name);
                                temp = readdir(dfd);
                        }
                   }
                    closedir(dfd);
                    i++;
                }

            }

} /* list() */


/*
 name: cd
 desc: changes the current directory
 params: char **args the argument passed to the command, char *cowd the current old working directory
 returns: returns nothing
 side effects: changes the value of the string passed in to cowd
*/

void cd(char **args,char *cowd) {
  // char *owd = getcwd(NULL,0);
  // owd = calloc(strlen(cowd) + 1, sizeof(char));
   if(args[2] == NULL){
      if(args[1] == NULL){
        char *temp = getcwd(NULL,0);
        strcpy(cowd,temp);
        free(temp);
        chdir(getenv("HOME"));
      }else if(strcmp(args[1],"-") == 0){
        chdir(cowd);
      }else {
        char *temp = getcwd(NULL,0);
        if(chdir(args[1]) == 0){
           strcpy(cowd,temp);
        }else{
	}
        //free(temp);
        //return (owd);
     }
  }
  else if(args[1] == NULL){
     chdir(getenv("HOME"));
     char *temp = getcwd(NULL,0);
        strcpy(cowd,temp);
        free(temp);
     //return(owd);
  }
  else{
     printf("Too many args\n");
     //return(cowd);
  }
}


/*
 name: prompt_func
 desc: changes the prompt
 params: char **args the argument passed to the command, char *prompt the current prompt
 returns: returns nothing
 side effects: changes the value of the string passed in to prompt
*/


void prompt_func(char **args,char *prompt){
  if(args[1] == NULL) {
                printf("Please input a prompt prefix: ");
                char test[MAXLINE];
                fgets(test, MAXLINE, stdin);
                if(test[strlen(test) - 1] == '\n'){
                     test[strlen(test) - 1] = '\0';
                }
                strcpy(prompt,test);
            }else {
                char test[MAXLINE];
                strcpy(test,args[1]);
                if(test[strlen(test) - 1] == '\n'){
                  test[strlen(test) - 1] = '\0';
                }
                strcpy(prompt,test);
            }
}


/*
 name: kill_func
 desc: kills the process given in the args
 params: char **args the argument given to the command
 returns: nothing
 side effects: none
*/


void kill_func(char **args) {

if(args[1] != NULL){
                if(args[2] == NULL) {
                   kill(atoi(args[1]),15);
                }
                else {
                   int sig = atoi(args[1]) * -1;
                   kill(atoi(args[2]),sig);
                }
           }

}


/*
 name: printenv
 desc: prints the environment
 params: char **args the argument passed to the command
 returns: returns nothing
 side effects: none
*/


void printenv(char **args) {
 if(args[2] != NULL && args[1] != NULL) {
                printf("printenv: too many arguments:\n");
           }else if(args[1] != NULL){
                printf("%s\n",getenv(args[1]));
           }else {
                int i = 0;
                while(environ[i] != NULL){
                    printf("%s\n", environ[i]);
                    i++;
                }

           }
}


/*
 name: setenv_func
 desc: add a new or changes an exsiting aspect of the environment
 params: char **args the argument passed to the command
 returns: returns nothing
 side effects: changes the environment
*/


void setenv_func(char **args) {

if(args[1] == NULL) {
                int i = 0;
                while(environ[i] != NULL){
                    printf("%s\n", environ[i]);
                    i++;
                }
           }
           else if(args[2] == NULL){
                setenv(args[1],"",1);
           }else {
                setenv(args[1],args[2],1);
           }

}
