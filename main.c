
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT 2048
#define MAX_ARGS 512

/**
 * 1-2: Command Prompt
 */
void commandPrompt(char *input, char *args[], char **inputFile, char **outputFile, int *background) {
 
  // Reset variables
  *inputFile = NULL;
  *outputFile = NULL;
  *background = 0;

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
  int count = 0;
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
          if (count < MAX_ARGS - 1) {
              args[count++] = token;
          } else {
              printf("Error: Too many arguments!!! \n");
              return;  
          }
      }

      prevToken = token;
      token = strtok(NULL, " ");
  }

  args[count] = NULL;  // set null character for strings
}


/**
 * 3: Commands 
 */
int commands(char *args[]) {

  if(args[0] == NULL) {
    return 0;
  }

  if(strcmp(args[0], "exit") == 0) {
    printf("Exiting Shell");
    exit(0);
  }
 
}

int main() {

  char input[MAX_INPUT];
  char* args[MAX_ARGS]; 
  char* inputFile = NULL;
  char* outputFile = NULL; 
  int background = 0; 


  while(1) {

    commandPrompt(input, args, &inputFile, &outputFile, &background);

  }

  return 0; 

}

