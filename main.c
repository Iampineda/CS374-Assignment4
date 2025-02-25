#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT 2048
#define MAX_ARGS 512
#define MAX_BG_PROCS 100

pid_t backgroundPIDS[MAX_BG_PROCS];
int backgroundCount = 0; 
int lastExitStatus = 0; 
  

/**
 * 1-2: Command Prompt
 */
void commandPrompt(char *input, char *args[], char **inputFile, char **outputFile, int *background, int *argc) {
 
  // Reset variables
  *inputFile = NULL;
  *outputFile = NULL;
  *background = 0;
  *argc = 0; 

   // Reset input buffer
   memset(input, 0, sizeof(char) * MAX_INPUT);  

   // Reset arguments array
   for (int i = 0; i < MAX_ARGS; i++) {
       args[i] = NULL;
   }

  // Display prompt
  printf(": ");
  fflush(stdout);

  // Read input
  if (fgets(input, MAX_INPUT, stdin) == NULL) {
      printf("Exiting");
      exit(0);  
  }

  // handle fgets new line character 
  input[strcspn(input, "\n")] = '\0';

  // Ignore blank lines and #
  if (!strlen(input) || input[0] == '#') return;

  // Parse arguments
  char *token = strtok(input, " ");
  char *prevToken = NULL; 

  while (token != NULL) {

      if (strcmp(token, "<") == 0) {  // Input redirection
          if (prevToken == NULL) { 
              printf("Error: `<` must be between words... \n");
              return;
          }

          token = strtok(NULL, " ");
          if (token == NULL) { 
              printf("Error: Missing filename after `<`... \n");
              return;
          }

          if(access(token, R_OK) == -1) {
            printf("Error: cannot open %s as an input \n", token);
          }

          *inputFile = token;
      } 

      else if (strcmp(token, ">") == 0) {  // Output redirection
          if (prevToken == NULL) { 
              printf("Error: `>` must be between words... \n");
              return;
          }
          token = strtok(NULL, " ");
          if (token == NULL) {
              printf("Error: Missing filename after `>`... \n");
              return;
          }
        
          *outputFile = token;

      } 
      else if (strcmp(token, "&") == 0) {  // Background execution
          token = strtok(NULL, " ");
          if (token != NULL) { 
              printf("Error: `&` must be the last word in the command.\n");
              return;
          }
          *background = 1;
      } 
      else {  // Normal argument
          if (*argc < MAX_ARGS - 1) {
              args[(*argc)++] = token;
          } else {
              printf("Error: Too many arguments!!! \n");
              return;  
          }
      }

      prevToken = token;
      token = strtok(NULL, " ");
  }

  
  args[*argc] = NULL;  // set null character for strings
}


/**
 * 3:  Built in Commands 
 */
int commands(char *args[]) {

  if(args[0] == NULL) {
    return 0;
  }

  // EXIT Logic 
  if(strcmp(args[0], "exit") == 0) {
    printf("Exiting Shell... \n");

    //KILL PROCESSED HERE
    for (int i = 0; i < backgroundCount; i++) {
        kill(backgroundPIDS[i], SIGTERM);  // Send terminate signal
    }

    backgroundCount = 0; 
    exit(0);

  }

  // CD Logic 
  else if(strcmp(args[0], "cd") == 0) {

    char* dir = args[1];

    // Ignore Background execution 
    if(dir && strcmp(dir, "&") == 0) {
      dir = NULL;
    }

    // Defualt to home dir
    if(dir == NULL) {
      dir = getenv("HOME");  
    }

    // attempt changing directory
    if(chdir(dir) == -1){
      perror("cd");
    } 

    return 1; 
  }

  // STATUS Logic 
  else if (strcmp(args[0], "status") == 0) {

    if (WIFEXITED(lastExitStatus)) {
        printf("Exit status: %d \n", WEXITSTATUS(lastExitStatus));

    } else if (WIFSIGNALED(lastExitStatus)) {
        printf("Terminated by signal: %d \n", WTERMSIG(lastExitStatus));
    }

    return 1;
  }

  return 0; 

}


/**
 * 4: Execute Other Commands 
 */
void otherCommands(char *args[]) {

  pid_t spawnPid = fork();
  int childStatus;

  switch (spawnPid) {
      case -1:  // Fork failed
          perror("fork() failed!");
          exit(1);
          break;

      case 0:  // Child process
          printf("Child process started, PID: %d\n", getpid());
          fflush(stdout);

          sleep(10);  

          if (execvp(args[0], args) == -1) {
              perror(args[0]);  
              exit(1);  
          }
          break;

      default:  // Parent process
          printf("Parent waiting for child with PID: %d\n", spawnPid);
          fflush(stdout);
        
          waitpid(spawnPid, &childStatus, 0);  

          printf("Child process %d terminated\n", spawnPid);
          fflush(stdout);

          
          if (WIFEXITED(childStatus)) {  
              lastExitStatus = WEXITSTATUS(childStatus); 
          } 
          else if (WIFSIGNALED(childStatus)) {
              lastExitStatus = WTERMSIG(childStatus);  
          }
          break;
  }
}


int main() {

  char input[MAX_INPUT];
  char* args[MAX_ARGS]; 
  char* inputFile = NULL;
  char* outputFile = NULL; 
  int background = 0; 
  int argc = 0;


  while(1) {

    lastExitStatus = 0; 

    // Handle Inputs 
    commandPrompt(input, args, &inputFile, &outputFile, &background, &argc);

    // Skip if input was blank or a comment
    if (args[0] == NULL) { 
      continue;  
    }

    // Handle built in commands 
    if(commands(args)) { continue; }
    otherCommands(args); 

  
    // Current Fail checks
    printf("%s: command not found\n", args[0]);  
    fflush(stdout);
  

  }

  return 0; 

}

