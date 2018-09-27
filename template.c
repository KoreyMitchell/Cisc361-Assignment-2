#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUFFERSIZE 128
char *prm = NULL;

/*
  Function Declarations for builtin shell commands:
 */
int blu_cd(char **args);
int blu_help(char **args);
int blu_exit(char **args);
int blu_prompt(char **args);
char *getcwd(char *buf, size_t size);



/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "prompt"
};

int (*builtin_func[]) (char **) = {
  &blu_cd,
  &blu_help,
  &blu_exit,
  &blu_prompt,
};

// int blu_prompt(){
//   printf("Input prompt prefix: ");
//   char prefix[10];
//   fgets(prefix,10,stdin);
//   int len = strlen(prefix);
//   prefix[len-1] = '\0';
//   strcpy(prm,prefix);
//   return 1;
// }

int blu_prompt(char **args){
  if (args[1] == NULL) {
    printf("Input prompt prefix: ");
    char prefix[10];
    fgets(prefix,10,stdin);
    int len = strlen(prefix);
    prefix[len-1] = '\0';
    strcpy(prm,prefix);
    return 1;
  } else {
    strcpy(prm,args[1]);
    return 1;
  }

}











int blu_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int blu_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "blu: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("blu");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int blu_help(char **args)
{
  int i;
  printf("Stephen Brennan's BLU\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < blu_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int blu_exit(char **args)
{
  return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int blu_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("blu");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("blu");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int blu_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < blu_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return blu_launch(args);
}

#define BLU_RL_BUFSIZE 1024
/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *blu_read_line(void)
{
  int bufsize = BLU_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "blu: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += BLU_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "blu: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define BLU_TOK_BUFSIZE 64
#define BLU_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **blu_split_line(char *line)
{
  int bufsize = BLU_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "blu: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, BLU_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += BLU_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "blu: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, BLU_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void blu_loop(void)
{

  char *line;
  char **args;
  int status;
  int flip = 1;

  do {

    char cwd[50];
    if(getcwd(cwd, sizeof(cwd)) != NULL){

      printf("%s [%s]> ", prm,cwd);
    }else{
      perror("Error printing current directory");

    }

    line = blu_read_line();
    args = blu_split_line(line);
    status = blu_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  prm = malloc(sizeof(char) * BUFFERSIZE);
  char gp[5] = " ";
  strcpy(prm,gp);
  // Load config files, if any.

  // Run command loop.
  blu_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
