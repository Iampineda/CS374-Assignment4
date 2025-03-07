#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT 2048
#define MAX_ARGS 512
#define MAX_BG_PROCS 100

pid_t backgroundPIDS[MAX_BG_PROCS];
int backgroundCount = 0;
int lastExitStatus = 0;
int foregroundOnlyMode = 0;

/**
 * 1-2: Command Prompt
 */
int commandPrompt(char *input, char *args[], char **inputFile, char **outputFile, int *background, int *argc)
{

  // Reset variables
  *inputFile = NULL;
  *outputFile = NULL;
  *background = 0;
  *argc = 0;

  // Reset input buffer
  memset(input, 0, sizeof(char) * MAX_INPUT);

  // Reset arguments array
  for (int i = 0; i < MAX_ARGS; i++)
  {
    args[i] = NULL;
  }

  // Display prompt
  printf(": ");
  fflush(stdout);

  // Read input
  if (fgets(input, MAX_INPUT, stdin) == NULL)
  {
    printf("Exiting\n");
    exit(0);
  }

  // Handle fgets new line character
  input[strcspn(input, "\n")] = '\0';

  // Ignore blank lines and comments (#)
  if (!strlen(input) || input[0] == '#')
  {
    return 1;
  }

  // Parse arguments
  char *token = strtok(input, " ");
  char *prevToken = NULL;

  while (token != NULL)
  {
    if (strcmp(token, "<") == 0)
    { // Input redirection
      token = strtok(NULL, " ");
      if (token == NULL)
      {
        printf("Error: Missing filename after `<`.\n");
        return 1;
      }
      *inputFile = token;
    }
    else if (strcmp(token, ">") == 0)
    { // Output redirection
      token = strtok(NULL, " ");
      if (token == NULL)
      {
        printf("Error: Missing filename after `>`.\n");
        return 1;
      }
      *outputFile = token;
    }
    else if (strcmp(token, "&") == 0)
    { // Background execution
      token = strtok(NULL, " ");
      if (token != NULL)
      {
        args[(*argc)++] = "&";
      }

      else
      {
        *background = (foregroundOnlyMode == 1) ? 0 : 1;
      }
    }
    else
    { // Normal argument
      if (*argc < MAX_ARGS - 1)
      {
        args[(*argc)++] = token;
      }
      else
      {
        printf("Error: Too many arguments!\n");
        return 1;
      }
    }
    prevToken = token;
    token = strtok(NULL, " ");
  }

  args[*argc] = NULL; // Null-terminate argument list
  return 0;          
}

/**
 * 3:  Built in Commands
 */
int commands(char *args[])
{

  if (args[0] == NULL)
  {
    return 0;
  }

  // EXIT Logic
  if (strcmp(args[0], "exit") == 0)
  {
    printf("Exiting Shell... \n");

    // KILL PROCESSED HERE
    for (int i = 0; i < backgroundCount; i++)
    {
      kill(backgroundPIDS[i], SIGTERM);
    }

    backgroundCount = 0;
    exit(0);
  }

  // CD Logic
  else if (strcmp(args[0], "cd") == 0)
  {

    char *dir = args[1];

    // Ignore Background execution
    if (dir && strcmp(dir, "&") == 0)
    {
      dir = NULL;
    }

    // Defualt to home dir
    if (dir == NULL)
    {
      dir = getenv("HOME");
    }

    // attempt changing directory
    if (chdir(dir) == -1)
    {
      perror("cd");
    }

    return 1;
  }

  // STATUS Logic
  else if (strcmp(args[0], "status") == 0)
  {
      if (WIFEXITED(lastExitStatus))
      {
          printf("exit value %d\n", WEXITSTATUS(lastExitStatus));
      }
      else if (WIFSIGNALED(lastExitStatus))
      {
          printf("terminated by signal %d\n", WTERMSIG(lastExitStatus));
      }
      fflush(stdout);
      return 1;
  }


  return 0;
}

/**
 * 5: Input & Output Redirection
 */
 void handleInputRedirection(char *inputFile)
{
    if (inputFile)
    {
        int inpFD = open(inputFile, O_RDONLY); 
        if (inpFD == -1)
        {
            perror("Error opening input file"); 
            exit(1);
        }

        // Redirect stdin to input file
        if (dup2(inpFD, STDIN_FILENO) == -1)
        {
            perror("dup2 (input)");
            close(inpFD); 
            exit(1);
        }

        close(inpFD); 
    }
}

 
 void handleOutputRedirection(char *outputFile)
 {
     if (outputFile)
     {
         int outFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
         if (outFD == -1)
         {
             perror("Error opening output file"); 
             fflush(stderr);
             exit(1);
         }
 
         // Redirect stdout to output file
         if (dup2(outFD, STDOUT_FILENO) == -1)
         {
             perror("dup2 (output)");
             close(outFD);
             exit(1);
         }
 
         // Redirect stderr to output file (optional, keep if needed)
         if (dup2(outFD, STDERR_FILENO) == -1)
         {
             perror("dup2 (stderr)");
             close(outFD);
             exit(1);
         }
 
         close(outFD); 
     }
 }
 

/**
 * 6. Executing Commands in Foreground & Background
 */
void redirectInputOutput(char *inputFile, char *outputFile, int background)
{

  // 🔹 Handle input redirection
  if (inputFile)
  {
    handleInputRedirection(inputFile);
  }
  else if (background)
  {

    // Redirect background input to /dev/null
    int devNull = open("/dev/null", O_RDONLY);
    dup2(devNull, STDIN_FILENO);
    close(devNull);
  }

  // 🔹 Handle output redirection
  if (outputFile)
  {
    handleOutputRedirection(outputFile);
  }
  else if (background)
  {

    // Redirect background output to /dev/null
    int devNull = open("/dev/null", O_WRONLY);
    dup2(devNull, STDOUT_FILENO);
    dup2(devNull, STDERR_FILENO);
    close(devNull);
  }
}

void runForegroundProcess(pid_t spawnPid)
{
    int childStatus;
    waitpid(spawnPid, &childStatus, 0);

    // Fix: Store exit code properly
    if (WIFEXITED(childStatus))
    {
        lastExitStatus = childStatus;  
    }
    else if (WIFSIGNALED(childStatus))
    {
        lastExitStatus = childStatus; 
        printf("terminated by signal %d\n", WTERMSIG(childStatus));
        fflush(stdout);
    }
}



void runBackgroundProcess(pid_t spawnPid)
{

  printf("background pid is %d\n", spawnPid);
  fflush(stdout);

  backgroundPIDS[backgroundCount++] = spawnPid;
}

void checkBackgroundProcesses()
{
    int childStatus;
    int newCount = 0;

    for (int i = 0; i < backgroundCount; i++)
    {
        pid_t bgPid = waitpid(backgroundPIDS[i], &childStatus, WNOHANG);

        if (bgPid > 0)
        { 
            if (WIFEXITED(childStatus))
            {
                int exitStatus = WEXITSTATUS(childStatus);
                printf("background process %d done. exit value: %d\n", bgPid, exitStatus);
                fflush(stdout);
            }
            else if (WIFSIGNALED(childStatus))
            {
                int termSignal = WTERMSIG(childStatus);
                printf("background process %d terminated by signal %d\n", bgPid, termSignal);
                fflush(stdout);
            }
        }
        else
        {
            backgroundPIDS[newCount++] = backgroundPIDS[i]; 
        }
    }

    backgroundCount = newCount; // Remove terminated processes
}


/**
 * 7: Signals SIGINT & SIGTSTP
 */
void handle_SIGINT(int signo)
{
  char *message = "Caught SIGINT, sleeping for 10 seconds\n";
  write(STDOUT_FILENO, message, 34);
}

void handle_SIGTSTP(int signo)
{
  if (foregroundOnlyMode == 0)
  {
    char *message = "\nEntering foreground-only mode (& is now ignored)\n";
    write(STDOUT_FILENO, message, 50);
    foregroundOnlyMode = 1;
  }
  else
  {
    char *message = "\nExiting foreground-only mode\n";
    write(STDOUT_FILENO, message, 30);
    foregroundOnlyMode = 0;
  }

  fflush(stdout);
}

/**
 * 4: Execute Other Commands with Input/Output Redirection
 */
int otherCommands(char *args[], char *inputFile, char *outputFile, int background)
{

  pid_t spawnPid = fork();
  int childStatus;

  switch (spawnPid)
  {

  case -1: // Fork failed
    perror("fork() failed!");
    return 0; 

  case 0: // Child process
    signal(SIGINT, SIG_DFL);
    redirectInputOutput(inputFile, outputFile, background);

    // Execute command using execvp()
    if (execvp(args[0], args) == -1)
    {
      perror(args[0]);
      exit(1); // Exit child process if execvp fails
    }
    break;

  default: // Parent process
    signal(SIGINT, SIG_IGN);
    if (background)
    {
      runBackgroundProcess(spawnPid);
      return 1; // Assumse background success
    }
    else
    {
      runForegroundProcess(spawnPid);

      // Check if foreground exited successfully
      if (WIFEXITED(lastExitStatus) && WEXITSTATUS(lastExitStatus) == 0)
      {
        return 1; 
      }
      else
      {
        return 0; 
      }
    }
  }

  return 0; // Failed execution 
}

int main()
{

  // Set up SIGTSTP handling
  struct sigaction SIGTSTP_action = {0};
  SIGTSTP_action.sa_handler = handle_SIGTSTP;
  sigfillset(&SIGTSTP_action.sa_mask);
  SIGTSTP_action.sa_flags = SA_RESTART;

  sigaction(SIGTSTP, &SIGTSTP_action, NULL);

  // Set up SIGINT handling
  struct sigaction SIGINT_action = {0};

  SIGINT_action.sa_handler = handle_SIGINT;
  sigfillset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0;

  sigaction(SIGINT, &SIGINT_action, NULL);

  char input[MAX_INPUT];
  char *args[MAX_ARGS];
  char *inputFile = NULL;
  char *outputFile = NULL;
  int background = 0;
  int argc = 0;
  int childStatus;

  while (1)
  {

    // Background process management
    checkBackgroundProcesses();

    // Handle Inputs
    if (commandPrompt(input, args, &inputFile, &outputFile, &background, &argc))
    {
      continue;
    }

    // Handle built in commands
    if (commands(args))
    {
      continue;
    }

    // Background process management
    checkBackgroundProcesses();

    // Handle other commands
    if (otherCommands(args, inputFile, outputFile, background))
    {
      continue;
    }
  }

  return 0;
}
