#include <stdio.h>
#include <stdlib.h>
#include<string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  while (1){

  printf("$ ");

  char *line[100];
  getline(stdin, 100, line);

  if(strcmp(line, "exit 0") == 0 || strcmp(line, "exit 1")==0) {
      exit(0);
  }
  if (line){
    printf("%s: command not found\n", line);
  }

  return 0;
}
