#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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

    // TYPE
    else if (strncmp(line, "type ", 5) == 0) {
      char *cmd = line + 5;
      // check for builtins
      if (strcmp(cmd, "echo") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "type") == 0) {
        printf("%s is a shell builtin\n", cmd);
        continue;
      }
      // check for executable in PATH
      else {
        char *path = getenv("PATH");
        if (path == NULL) path = "";
        char path_cpy[1000];
        strncpy(path_cpy, path, sizeof(path_cpy));
        path_cpy[sizeof(path_cpy) - 1] = '\0';  // Ensure null-termination

        char *dir = strtok(path_cpy, ":");

        int found = 0;

        while (dir != NULL) {
          char fullpath[1200];
          snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd);

          if (access(fullpath, X_OK) == 0) {
            printf("%s is %s\n", cmd, fullpath);
            found = 1;
            break;
          }
          dir = strtok(NULL, ":");
        }
        if (!found) {
          printf("%s: not found\n", cmd);
        }
        continue;
      }
    }

    // EXTERNAL COMMANDS FOR RUNNIGN A PROGRAM
    {
      char *args[20];
      char *token = strtok(line, " ");
      int i = 0;

      while (token != NULL && i < 19) {
        args[i++] = token;
        token = strtok(NULL, " ");
      }
      args[i] = NULL;

      if (args[0] == NULL) {
        continue;  // empty command
      }

      char *cmd = args[0];

      // Search PATH for executable
      char *path = getenv("PATH");
      if (path == NULL) path = "";

      char path_cpy[1000];
      strncpy(path_cpy, path, sizeof(path_cpy));
      path_cpy[sizeof(path_cpy) - 1] = '\0';

      char *dir = strtok(path_cpy, ":");
      int found = 0;

      char exec_path[1200];
 
      while (dir != NULL) {
        snprintf(exec_path, sizeof(exec_path), "%s/%s", dir, cmd);

        if (access(exec_path, X_OK) == 0) {
          found = 1;
          break;
        }
        dir = strtok(NULL, ":");
      }

      if (!found) {
        printf("%s: command not found\n", cmd); 
        continue;
      }

      // Run the executable
      pid_t pid = fork();

      if (pid < 0) {
        perror("fork failed");
      } else if (pid == 0) {
        // Child process
        execv(exec_path, args);
        perror("execv failed");
        exit(1);
      } else {
        // Parent
        int status;
        waitpid(pid, &status, 0);
      }

      continue;
    }

    printf("%s: command not found\n", line);
  }
  return 0;
}
