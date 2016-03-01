/* shell.c
   Michael Clifford
   Implements a shell with 10 types of functionality
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

int main() {
  void execute(char *f, char **a, int bk);
  char *getfile(char *list);
  char **getargs(char *list);
  int commands(char *arg, char *opt);
  void freeup(char **arr);
  char ** redirect(char **a);
  void pipeit(char **a);

 char *line = malloc(1024);
 if (line == NULL) {
    printf("malloc() failure\n");
    exit(3);
  }

  int infd = dup(0);
  int outfd = dup(1);
  char *f;
  char **a;
  int i;
  int pipe = 0;
  int back = 0;
  int status;
  int id;
  int c;
  printf("<shell>");
  signal(SIGINT, SIG_IGN);
  line = fgets(line, 1024, stdin);
  while (line != NULL) {   //loop: get input, execute command, repeat until EOF
    id = waitpid(-1, &status, WNOHANG);
    f = getfile(line);
    a = getargs(line);
    a = redirect(a);           //implement redirection, if needed
    i = 0;

    while (*(a + i) != NULL) {           //check for pipe
      if (strcmp(*(a + i), "|") == 0) {
	pipe = 1;
	break;
      }
      i++;
    }
    i = 0;
    while (*(a + i) != NULL) {            //check for &
      if (strcmp(*(a + i), "&") == 0 && *(a + i + 1) == NULL) {
	back = 1;
	*(a + i) = NULL;
	free(*(a + i));
      }
      i++;
    }
    if (*a == NULL) {
    }
    else if (commands(f, *(a + 1)) == 0 && pipe == 0) { //if builtin command called, execute it
      execute(f, a, back);         //otherwise execute non-builtin command
    }
    else if (pipe == 1) {           //otherwise pipe
      pipeit(a);
    }
    free(f);
    freeup(a);
    free(a);
    dup2(infd, 0);
    dup2(outfd, 1);
    pipe = 0;
    back = 0;
    if (id > 0) {
      printf("Child reaped: %i\n", id);
    }
    printf("<shell>");
    signal(SIGINT, SIG_IGN);
    c = getchar();
    if (c != EOF) {
      ungetc(c, stdin);
      line = fgets(line, 1024, stdin);
    }
    else {
      free(line);
      line = NULL;
    }
  }
  printf("\n");
  return 0;
}

void sig_handler(int sig) {
  printf("\n");
  (kill(-2, sig));
}

void execute(char *f, char **a, int bk) { //fork and exec, code inspired by class demo
  void freeup(char **arr);
  void sig_handler(int sig);
  pid_t id;
  int status;
  if ((id = fork()) < 0) {       //fork
    printf("fork() error\n");
    exit(1);
  }
  else if (id == 0) {
    if (execvp(f, a) < 0) {       //execute
      printf("Invalid input\n");
      exit(1);
    }
  }
  else if (bk == 1) {             //run in background
  }
  else {                       //wait for child process to finish
    signal(SIGINT, sig_handler);     //trap SIGINT
    while (wait(&status) != id)  {
    }
  }
}

int commands(char *arg, char *opt) {  //execute builtin commands
  if (strcmp(arg, "exit") == 0) {           //exit
    exit(1);
    return 1;
  }
  else if (strcmp(arg, "myinfo") == 0) {      //myinfo
    printf("Process ID: %i\n", getpid());
    printf("Parent Process ID: %i\n", getppid());
    return 1;
  }
  else if (strcmp(arg, "cd") == 0) {
    if (opt == NULL) {             //cd
      chdir(getenv("HOME"));
    }
    else {
      char slash[] = "/";            //cd <dir>
      char *d = strcat(slash, opt);
      char *pwd = getenv("PWD");
      char *p = strcat(pwd, d);
      chdir(p);
    }
    return 1;
  }
  else {
    return 0;
  }
}
 
char **redirect(char **a) {    //implement file redirection
  void freeup(char **arr);
  FILE *file = NULL;
  FILE *file2 = NULL;
  char **b;
  int num;
  int num2;
  char *r1 = NULL;
  int p1 = 0;
  char *r2 = NULL;
  int p2 = 0;
  int j = 0;                     
  while (*(a + j) != NULL) {   //check for <'s and >'s in the input
    if ((strcmp(*(a + j), "<") == 0 || strcmp(*(a + j), ">") == 0) && r1 == NULL) {
      if (strcmp(*(a + j), "<") == 0 && j >= 3 && strcmp(*(a + j - 2), "|") == 0) {         //check for redirect into pipe
	printf("Can't redirect into pipe\n");
	freeup(a);
	*a = NULL;
	return(a);
      }
      if (strcmp(*(a + j), ">") == 0 && *(a + j + 2) != NULL && strcmp(*(a + j + 2), "|") == 0) {      //check for redirect into pipe
	printf("Can't redirect into pipe\n");
	freeup(a);
	*a = NULL;
	return a;
      }
      r1 = *(a + j);
      p1 = j;
    }
    else if (strcmp(*(a + j), "<") == 0 || strcmp(*(a + j), ">") == 0) {
      r2 = *(a + j);
      p2 = j;
    }
    j++;
  }
  if (r1 != NULL && r2 == NULL) {   //single redirection
    if (strcmp(r1, "<") == 0) {
      file = fopen(*(a + p1 + 1), "r");
      if (file == NULL) {
	perror(*(a + p1 + 1));
	freeup(a);
	free(a);
	*a = NULL;
	return(a);
      }
      num = fileno(file);
      dup2(num, STDIN_FILENO);
    }
    else if (strcmp(r1, ">") == 0) {
      file = fopen(*(a + p1 + 1), "w");
      num = fileno(file);
      dup2(num, STDOUT_FILENO);
    }
    b = malloc(1024);
    if (b == NULL) {
      printf("malloc() failure\n");
      exit(3);
    }
    int k = 0;
    while (k < p1) {
      *(b + k) = malloc(10);
      if (*(b + k) == NULL) {
	printf("malloc() failure\n");
	exit(3);
      }
      memcpy(*(b + k), *(a + k), strlen(*(a + k)));
      k++;
    }
    *(b + k) = NULL;
    freeup(a);
    free(a);
    fclose(file);
    return(b);
  }
  else if (r2 != NULL) {            //2 redirections
    if (strcmp(r1, "<") == 0) {
      file = fopen(*(a + p1 + 1), "r");
      if (file == NULL) {
	perror(*(a + p1 + 1));
	freeup(a);
	*a = NULL;
	return a;
      }
      file2 = fopen(*(a + p2 + 1), "w");
      num = fileno(file);
      num2 = fileno(file2);
      dup2(num, STDIN_FILENO);
      dup2(num2, STDOUT_FILENO);
    }
    else {
      file = fopen(*(a + p1 + 1), "w");
      file2 = fopen(*(a + p2 + 1), "r");
      if (file2 == NULL) {
	perror(*(a + p2 + 1));
	freeup(a);
	*a = NULL;
	return a;
      }
      num = fileno(file);
      num2 = fileno(file2);
      dup2(num, STDOUT_FILENO);
      dup2(num2, STDIN_FILENO);
    }
    b = malloc(1024);
    if (b == NULL) {
      printf("malloc() failure\n");
      exit(3);
    }
    int k = 0;
    while (k < p1) {
      *(b + k) = malloc(10);
      if (*(b + k) == NULL) {
	printf("malloc() failure\n");
	exit(3);
      }
      memcpy(*(b + k), *(a + k), strlen(*(a + k)));
      k++;
    }
    *(b + k) = NULL;
    freeup(a);
    free(a);
    fclose(file);
    fclose(file2);
    return(a);
  }
  return(a);
}

void pipeit(char **a) {           //creaete pipe, code inspired by in-class pipe demo
  void execute(char *f, char **a, int bk);
  void freeup(char **b);
  int commands(char *arg, char *opt);

  char **b;
  int j = 0;
  int fd[2];
  pid_t childpid;
  int status;
  while (*(a + j) != NULL) {           //find location of first pipe
    if (strcmp(*(a + j), "|") == 0) {
      break;
    }
    j++;
  }
  
  int i = 0;                       //find number of pipes
  int numpipes = 0;
  while (*(a + i) != NULL) {
    if (strcmp(*(a + i), "|") == 0) {
      numpipes++;
    }
    i++;
  }

  pipe(fd);            
  if ((childpid = fork()) < 0) {             //fork
    printf("fork() error\n");
    exit(1);
  }
  else if (childpid == 0) {             //in child, execute write part of pipe
    b = malloc(1024);
    if (b == NULL) {
      printf("malloc() failure\n");
      exit(3);
    }
    int k = 0;
    while (k < j) {
      *(b + k) = malloc(10);
      if (*(b + k) == NULL) {
	printf("malloc() failure\n");
	exit(3);
      }
      memcpy(*(b + k), *(a + k), strlen(*(a + k)));
      k++;
    }
    *(b + k) = NULL;
    close(fd[0]);
    int o = dup(1);
    dup2(fd[1], 1);
    if (execvp(*a, b) < 0) {
      dup2(o, 1);
      printf("Invalid input\n");
      exit(1);
    }
  }
  else {                                     //in parent: wait for child, then execute read part of pipe
    while (wait(&status) != childpid) {
    }
    close(fd[1]);
    dup2(fd[0], 0);
    if (numpipes > 1) {            //recurse if there is more piping to be done
      pipeit(a + j + 1);
    }
    else {
      execute(*(a + j + 1), a + j + 1, 0);
    }
  }
}
  
char *getfile(char *list) {  //extract name of command from input array
  char *file;

  file = malloc(20);
  if (file == NULL) {
    printf("malloc() failure\n");
    exit(3);
  }

  int i = 0;
  while (*(list + i) != ' ' && *(list + i) != '\n') {
    *(file + i) = *(list + i);
    i++;
  }
  *(file + i) = '\0';
  return file;
}

char **getargs(char *list) { //create vector of strings from input array
  char **args;
  
  args = malloc(1024);
  if (args == NULL) {
    printf("malloc failure\n");
    exit(3);
  }
  
  int i = 0;
  int j = 0;
  int k = 0;

  *(args + j) = malloc(10);
  if (*(args + j) == NULL) {
    printf("malloc() failure\n");
    exit(3);
  }
  
  while (*(list + i) != '\n') {
    while (*(list + i) != ' ' && *(list + i) != '\n') {
      *(*(args + j) + k) = *(list + i);
	i++;
	k++;
    }
    *(*(args + j) + k) = '\0';
    j++;
    k = 0;
   
    *(args + j) = malloc(10);
    if (*(args + j) == NULL) {
      printf("malloc() failure\n");
      exit(3);
    }
    
    while (*(list + i) == ' ') {     //skip extra whitespace      
      i++;
    }
  }
  free(*(args + j));
  *(args + j) = NULL;
  return args;
}

void freeup(char **arr) {     //free contents of 2-D array
  int i = 0;
  while (*(arr + i) != NULL) {
    free(*(arr + i));
    i++;
  }
}

