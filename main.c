
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT 2048
#define MAX_ARGS 512
  

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
 * 3: Commands 
 */
int commands(char *args[]) {

  if(args[0] == NULL) {
    return 0;
  }

  if(strcmp(args[0], "exit") == 0) {
    printf("Exiting Shell...");

    //KILL PROCESSED HERE
    exit(0);
  }

  else if(strcmp(args[0], "cd") == 0) {
    if(args[1] == NULL) {
      printf(" HOME"); 
    }
  }
 
  return 1; 
}



int main() {

  char input[MAX_INPUT];
  char* args[MAX_ARGS]; 
  char* inputFile = NULL;
  char* outputFile = NULL; 
  int background = 0; 
  int argc = 0;


  while(1) {

    // Handle Inputs 
    commandPrompt(input, args, &inputFile, &outputFile, &background, &argc);

    // Handle built in commands 
    if(commands(args)) { continue; }

  }

  return 0; 

}

