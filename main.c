#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_INPUT 2048
#define MAX_ARGS 512
#define MAX_BG_PROCS 100

pid_t backgroundPIDS[MAX_BG_PROCS];
int backgroundCount = 0; 
int lastExitStatus = 0; 
  

/**
 * 1-2: Command Prompt
 */
int commandPrompt(char *input, char *args[], char **inputFile, char **outputFile, int *background, int *argc) {
 
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
      printf("Exiting\n");
      exit(0);  
  }

  // Handle fgets new line character 
  input[strcspn(input, "\n")] = '\0';

  // Ignore blank lines and comments (#)
  if (!strlen(input) || input[0] == '#') {
      return 1; 
  }

  // Parse arguments
  char *token = strtok(input, " ");
  char *prevToken = NULL; 

  while (token != NULL) {
      if (strcmp(token, "<") == 0) {  // Input redirection
          token = strtok(NULL, " ");
          if (token == NULL) { 
              printf("Error: Missing filename after `<`.\n");
              return 1; 
          }
          *inputFile = token;
      } 
      else if (strcmp(token, ">") == 0) {  // Output redirection
          token = strtok(NULL, " ");
          if (token == NULL) {
              printf("Error: Missing filename after `>`.\n");
              return 1;
          }
          *outputFile = token;
      } 
      else if (strcmp(token, "&") == 0) {  // Background execution
          token = strtok(NULL, " ");
          if (token != NULL) { 
              printf("Error: `&` must be the last word in the command.\n");
              return 1;
          }
          *background = 1;
      } 
      else {  // Normal argument
          if (*argc < MAX_ARGS - 1) {
              args[(*argc)++] = token;
          } else {
              printf("Error: Too many arguments!\n");
              return 1;  
          }
      }
      prevToken = token;
      token = strtok(NULL, " ");
  }

  args[*argc] = NULL;  // Null-terminate argument list
  return 0;  // Successfully parsed command
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
        kill(backgroundPIDS[i], SIGTERM);  
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

    fflush(stdout); 

    return 1;
  }

  return 0; 

}


/**
 * 5: Input & Output Redirection
 */
void handleInputRedirection(char *inputFile) {

  if (inputFile) {

      int inpFD = open(inputFile, O_RDONLY);  // Open file in read mode
      if (inpFD == -1) {
          fprintf(stderr, "Error: cannot open %s for input file\n", inputFile);
          fflush(stderr);
          exit(1);
      }

      dup2(inpFD, STDIN_FILENO);
      close(inpFD);
  }
}

void handleOutputRedirection(char *outputFile) {

  if (outputFile) {

      int outFD = open(outputFile, O_WRONLY | O_CREAT, O_TRUNC, 0644);  // Open file in write mode
      if (outFD == -1) {
          fprintf(stderr, "Error: cannot open %s for output file \n", outputFile);
          fflush(stderr);
          exit(1);
      }

      dup2(outFD, STDOUT_FILENO);
      close(outFD);
  }
}


/**
 * 6. Executing Commands in Foreground & Background
 */
void redirectInputOutput(char *inputFile, char *outputFile, int background) {

  // 🔹 Handle input redirection
  if (inputFile) {
      handleInputRedirection(inputFile);

  } else if (background) {  

      // Redirect background input to /dev/null
      int devNull = open("/dev/null", O_RDONLY);
      dup2(devNull, STDIN_FILENO);
      close(devNull);
  }

  // 🔹 Handle output redirection
  if (outputFile) {
      handleOutputRedirection(outputFile);

  } else if (background) {  

      // Redirect background output to /dev/null
      int devNull = open("/dev/null", O_WRONLY);
      dup2(devNull, STDOUT_FILENO);
      dup2(devNull, STDERR_FILENO);
      close(devNull);
  }
}

void runForegroundProcess(pid_t spawnPid) {

  int childStatus;
  waitpid(spawnPid, &childStatus, 0);

  // Store exit status of foreground process
  if (WIFEXITED(childStatus)) {  
      lastExitStatus = WEXITSTATUS(childStatus);

  } else if (WIFSIGNALED(childStatus)) {
      lastExitStatus = WTERMSIG(childStatus);
  }
}

void runBackgroundProcess(pid_t spawnPid) {
  
  printf("background pid is %d \n", spawnPid);
  fflush(stdout);

  backgroundPIDS[backgroundCount++] = spawnPid;
  lastExitStatus = 0; 
}

void checkBackgroundProcesses() {

  int childStatus;
  for (int i = 0; i < backgroundCount; i++) {

      pid_t bgPid = waitpid(backgroundPIDS[i], &childStatus, WNOHANG);
      
      if (bgPid > 0) {  // Background process has finished
          if (WIFEXITED(childStatus)) {

              printf("Background process %d terminated. Exit status: %d \n", bgPid, WEXITSTATUS(childStatus));
          } 
          else if (WIFSIGNALED(childStatus)) {
              printf("Background process %d terminated by signal %d \n", bgPid, WTERMSIG(childStatus));
          }

          fflush(stdout);

          // Remove the finished process from the list
          for (int j = i; j < backgroundCount - 1; j++) {
              backgroundPIDS[j] = backgroundPIDS[j + 1];
          }

          backgroundCount--; 
          i--;  
      }
  }
}

/**
 * 4: Execute Other Commands with Input/Output Redirection 
 */
void otherCommands(char *args[], char *inputFile, char *outputFile, int background) {
 
  pid_t spawnPid = fork();
  int childStatus;

  switch (spawnPid) {

      case -1:  // Fork failed
          perror("fork() failed!");
          exit(1);
          break;

      case 0:  // Child process

      redirectInputOutput(inputFile, outputFile, background);

          // Execute command using execvp()
          if (execvp(args[0], args) == -1) {
              perror(args[0]);  
              lastExitStatus = 1;
              exit(1);  
          }
          break;

      default:  // Parent process

        if (background) {  
          runBackgroundProcess(spawnPid);
        } else {  
          runForegroundProcess(spawnPid);
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
  int childStatus; 

  while(1) {

    // Handle Inputs 
    if(commandPrompt(input, args, &inputFile, &outputFile, &background, &argc)) { continue; }

    // Handle built in commands 
    if(commands(args)) { continue; }


    // Handle other commands 
    otherCommands(args, inputFile, outputFile, background); 

    // Background process management
    checkBackgroundProcesses(); 


    // Current Fail checks
    printf("%s: command not found\n", args[0]);  
    fflush(stdout);
  
  }

  return 0; 
}

