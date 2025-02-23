
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 2048
#define MAX_ARGS 512

/**
 * 1: Command Prompt
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

