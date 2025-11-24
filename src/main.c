#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  while (1) {
    printf("$ ");

    char line[100];
    if (fgets(line, sizeof(line), stdin) == NULL) {
      continue;
    }

    line[strcspn(line, "\n")] = 0;  // Remove newline character

    // EXIT
    if (strcmp(line, "exit 0") == 0 || strcmp(line, "exit 1") == 0 || strcmp(line, "exit") == 0) {
      exit(0);
    }

    // ECHO
    else if (strncmp(line, "echo ", 5) == 0) {
      printf("%s\n", line + 5);
      continue;
    } else if (strcmp(line, "echo") == 0) {
      printf("\n");
      continue;
    }

    else if (strncmp(line, "type ", 5) == 0) {
      if (strcmp(line + 5, "echo") == 0 || strcmp(line + 5, "exit") == 0 ||
          strcmp(line + 5, "type") == 0) {
        printf("%s is a shell builtin\n", line + 5);
        continue;
      } else {
        printf("%s: not found\n", line + 5);
        continue;
      }
    }

    printf("%s: command not found\n", line);
  }
  return 0;
}
