//Authors: Patrick Taylor, CJ Mitterer(In other Section)

#include "sh.h"
#include <signal.h>
#include <stdio.h>


#define MAXLINE 128


void sig_handler(int signal); 

int main( int argc, char **argv, char **envp )
{
  /* put signal set up stuff here */
  signal(SIGINT, sig_handler);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  return sh(argc, argv, envp);
}

void sig_handler(int signal)
{
  fflush(stdout);
  printf("\n");
/* define your signal handler */
}

