#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// builtin commands
const char* builtins[] = {"cd", "echo", "exit", "history", "pwd", NULL};

char* combined_generator(const char* text, int state) {
  static int stage;        // 0 = builtins, 1 = externals
  static int builtin_idx;  // builtin index

  static char* path_copy = NULL;
  static char* current_dir = NULL;

  static DIR* d = NULL;
  static struct dirent* entry = NULL;

  static char* saved_dir = NULL;

  if (state == 0) {
    stage = 0;
    builtin_idx = 0;

    if (path_copy) free(path_copy);

    char* envpath = getenv("PATH");
    path_copy = strdup(envpath ? envpath : "");

    current_dir = strtok(path_copy, ":");

    if (d) {
      closedir(d);
      d = NULL;
    }

    saved_dir = NULL;
  }

  if (stage == 0) {
    while (builtins[builtin_idx]) {
      const char* name = builtins[builtin_idx++];
      if (strncmp(name, text, strlen(text)) == 0) {
        return strdup(name);
      }
    }
    stage = 1;  // Move to external executables
  }

  while (current_dir || d) {
    if (!d) {
      saved_dir = current_dir;
      d = opendir(current_dir);
      current_dir = strtok(NULL, ":");
    }

    if (d) {
      while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, text, strlen(text)) == 0) {
          char full[PATH_MAX];
          snprintf(full, sizeof(full), "%s/%s", saved_dir, entry->d_name);

          if (access(full, X_OK) == 0) {
            return strdup(entry->d_name);
          }
        }
      }

      closedir(d);
      d = NULL;
      saved_dir = NULL;
    }
  }

  // No more matches
  return NULL;
}

// auto-cpmplete for builtins
/*
char* builtin_generator(const char* text, int state) {
  static int idx;
  const char* name;

  if (!state) idx = 0;

  while ((name = builtins[idx++])) {
    if (strncmp(name, text, strlen(text)) == 0) {
      char* match = strdup(name);
      return match;
    }
  }

  return NULL;
}
*/

int is_external_prefix(const char* text) {
  if (!text || text[0] == '\0') return 0;

  char* path = getenv("PATH");
  if (!path) return 0;

  char* p = strdup(path);
  char* dir = strtok(p, ":");

  while (dir) {
    DIR* d = opendir(dir);
    if (d) {
      struct dirent* entry;
      while ((entry = readdir(d)) != NULL) {
        // match prefix
        if (strncmp(entry->d_name, text, strlen(text)) == 0) {
          // build full path
          char full[PATH_MAX];
          snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name);

          // check if executable
          if (access(full, X_OK) == 0) {
            closedir(d);
            free(p);
            return 1;  // found at least one external match
          }
        }
      }
      closedir(d);
    }
    dir = strtok(NULL, ":");
  }

  free(p);
  return 0;
}

int is_builtin_prefix(const char* text) {
  for (int i = 0; builtins[i] != NULL; i++) {
    if (strncmp(builtins[i], text, strlen(text)) == 0) {
      return 1;
    }
  }
  return 0;
}

char** completion_hook(const char* text, int start, int end) {
  if (!is_builtin_prefix(text) && !is_external_prefix(text)) {
    // Bell character
    printf("\x07");
    fflush(stdout);

    return NULL;  // No autocomplete
  }

  char** matches = rl_completion_matches(text, combined_generator);

  if (matches && matches[0] != NULL) rl_insert_text(" ");

  return matches;
}

// Making a self tokeniser of ' ' adn " " and backslash escapes
void tokenize(char* line, char* args[], int* argc_out) {
  int i = 0;
  int len = strlen(line);
  int in_quotes = 0;
  int in_double_quotes = 0;
  int escape = 0;
  char current[200] = {0};
  int cur = 0;

  for (int j = 0; j < len; j++) {
    char c = line[j];

    if (!in_quotes && !in_double_quotes && c == '\\') {
      escape = 1;
      continue;
    }

    if (in_quotes) {
      if (c == '\'') {
        in_quotes = 0;
      } else {
        current[cur++] = c;
      }
      continue;
    }

    if (in_double_quotes) {
      if (escape) {
        if (c == '\"' || c == '\\') {
          current[cur++] = c;
        } else {
          current[cur++] = '\\';
          current[cur++] = c;
        }
        escape = 0;
        continue;
      }

      if (c == '\\') {
        escape = 1;
        continue;
      }

      if (c == '\"') {
        in_double_quotes = 0;
      } else {
        current[cur++] = c;
      }
      continue;
    }

    if (escape) {
      current[cur++] = c;
      escape = 0;
      continue;
    }

    // not in quotes
    if (c == '\'') {
      in_quotes = 1;
      continue;
    }

    if (c == '\"') {
      in_double_quotes = 1;
      continue;
    }

    if (c == ' ' || c == '\t') {
      if (cur > 0) {
        current[cur] = '\0';
        args[i] = strdup(current);
        i++;
        cur = 0;
        current[0] = '\0';
      }
      continue;
    }

    current[cur++] = c;
  }

  if (cur > 0) {
    current[cur] = '\0';
    args[i] = strdup(current);
    i++;
  }

  args[i] = NULL;
  *argc_out = i;
}

int main(int argc, char* argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  rl_bind_key('\t', rl_complete);
  rl_attempted_completion_function = completion_hook;

  while (1) {
    char* line = readline("$ ");
    if (line == NULL) {
      printf("\n");
      break;  // EOF
    }
    if (strlen(line) > 0) {
      add_history(line);
    }

    line[strcspn(line, "\n")] = 0;  // Remove newline character

    // EXIT
    if (strcmp(line, "exit 0") == 0 || strcmp(line, "exit 1") == 0 || strcmp(line, "exit") == 0) {
      exit(0);
    }

    // ECHO
    else if (strncmp(line, "echo", 4) == 0) {
      char* args2[20];
      int argc_echo = 0;
      tokenize(line + 5, args2, &argc_echo);

      int redirect_stdout_index = -1;
      int redirect_stderr_index = -1;
      int append_stdout_index = -1;
      int append_stderr_index = -1;
      char* append_stderr_file = NULL;
      char* append_stdout_file = NULL;
      char* outfile = NULL;
      char* errfile = NULL;

      for (int r = 0; r < argc_echo; r++) {
        if (strcmp(args2[r], ">") == 0 || strcmp(args2[r], "1>") == 0) {
          redirect_stdout_index = r;
          outfile = args2[r + 1];
          break;
        } else if (strcmp(args2[r], "2>") == 0) {
          redirect_stderr_index = r;
          errfile = args2[r + 1];
          break;
        } else if (strcmp(args2[r], "2>>") == 0) {
          append_stderr_index = r;
          append_stderr_file = args2[r + 1];
          break;
        } else if (strcmp(args2[r], ">>") == 0 || strcmp(args2[r], "1>>") == 0) {
          append_stdout_index = r;
          append_stdout_file = args2[r + 1];
          break;
        }
      }

      // Prepare for temporary redirection
      int saved_stdout = -1;
      int fd = -1;

      if (redirect_stdout_index != -1) {
        fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
          perror("open");
          continue;
        }

        saved_stdout = dup(1);

        dup2(fd, 1);
        close(fd);

        args2[redirect_stdout_index] = NULL;
        argc_echo = redirect_stdout_index;
      }

      if (redirect_stderr_index != -1) {
        int fd_err = open(errfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd_err < 0) {
          perror("open");
          continue;
        }

        saved_stdout = dup(1);

        dup2(fd_err, 2);
        close(fd_err);

        args2[redirect_stderr_index] = NULL;
        argc_echo = redirect_stderr_index;
      }
      if (append_stdout_index != -1) {
        fd = open(append_stdout_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd < 0) {
          perror("open");
          continue;
        }

        saved_stdout = dup(1);

        dup2(fd, 1);
        close(fd);

        args2[append_stdout_index] = NULL;
        argc_echo = append_stdout_index;
      }
      if (append_stderr_index != -1) {
        int fd_err = open(append_stderr_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd_err < 0) {
          perror("open");
          continue;
        }

        saved_stdout = dup(1);

        dup2(fd_err, 2);
        close(fd_err);

        args2[append_stderr_index] = NULL;
        argc_echo = append_stderr_index;
      }

      for (int k = 0; k < argc_echo; k++) {
        printf("%s", args2[k]);
        if (k < argc_echo - 1) printf(" ");
      }
      printf("\n");

      if (saved_stdout != -1) {
        dup2(saved_stdout, 1);
        close(saved_stdout);
      }

      continue;
    }

    /*
    else if (strncmp(line, "echo ", 5) == 0) {
      printf("%s\n", line + 5);
      continue;
    } else if (strcmp(line, "echo") == 0) {
      printf("\n");
      continue;
    }
    */
    // TYPE
    else if (strncmp(line, "type ", 5) == 0) {
      char* cmd = line + 5;
      // check for builtins
      if (strcmp(cmd, "echo") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "type") == 0 ||
          strcmp(cmd, "pwd") == 0 || strncmp(cmd, "cd", 2) == 0) {
        printf("%s is a shell builtin\n", cmd);
        continue;
      }
      // check for executable in PATH
      else {
        char* path = getenv("PATH");
        if (path == NULL) path = "";
        char path_cpy[1000];
        strncpy(path_cpy, path, sizeof(path_cpy));
        path_cpy[sizeof(path_cpy) - 1] = '\0';  // Ensure null-termination

        char* dir = strtok(path_cpy, ":");

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

    // PWD
    else if (strncmp(line, "pwd", 3) == 0) {
      char cwd[1000];
      if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
      } else {
        perror("getcwd() error");
      }
      continue;

    }

    // absolute path handling for cd
    else if (strncmp(line, "cd", 2) == 0) {
      if (strcmp(line, "cd") == 0) {
        continue;
      }
      if (line[2] != ' ') {
        continue;
      }
      char* path = line + 3;
      if (path[0] == '/') {
        if (chdir(path) != 0) {
          printf("cd: %s: No such file or directory\n", path);
        }
        continue;
      }
      // home directory handling for cd
      if (path[0] == '~') {
        char* home = getenv("HOME");
        if (home == NULL) {
          printf("cd: Home is not set\n");
          continue;
        }
        if (path[1] == "\0") {
          if (chdir(home) != 0) {
            printf("cd: %s: No such file or directory\n", home);
          }
          continue;
        }
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", home, path + 2);
        if (chdir(fullpath) != 0) {
          printf("cd: %s: No such file or directory\n", fullpath);
        }
        continue;
      }

      // relative path handling for cd
      if (chdir(path) != 0) {
        printf("cd: %s: No such file or directory\n", path);
      }
      continue;
    }

    // EXTERNAL COMMANDS FOR RUNNIGN A PROGRAM
    {
      char* args[20];
      int argc2 = 0;
      tokenize(line, args, &argc2);
      if (argc2 == 0) {
        continue;
      }

      // Alternative tokenization
      /*
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
      */

      // Redirect Stdout and stderr if needed
      int redirect_stdout_index = -1;
      int redirect_stderr_index = -1;
      int append_stdout_index = -1;
      int append_stderr_index = -1;
      char* append_stdout_file = NULL;
      char* append_stderr_file = NULL;
      char* filename = NULL;
      char* errfile = NULL;
      for (int k = 0; k < argc2; k++) {
        if (strcmp(args[k], ">") == 0 || strcmp(args[k], "1>") == 0) {
          redirect_stdout_index = k;
          filename = args[k + 1];
          break;
        } else if (strcmp(args[k], "2>") == 0) {
          redirect_stderr_index = k;
          errfile = args[k + 1];
          break;
        } else if (strcmp(args[k], ">>") == 0 || strcmp(args[k], "1>>") == 0) {
          append_stdout_index = k;
          append_stdout_file = args[k + 1];
          break;
        } else if (strcmp(args[k], "2>>") == 0) {
          append_stderr_index = k;
          append_stderr_file = args[k + 1];
          break;
        }
      }
      if (redirect_stdout_index != -1) {
        args[redirect_stdout_index] = NULL;
      }
      if (redirect_stderr_index != -1) {
        args[redirect_stderr_index] = NULL;
      }
      if (append_stdout_index != -1) {
        args[append_stdout_index] = NULL;
      }
      if (append_stderr_index != -1) {
        args[append_stderr_index] = NULL;
      }
      // Find the command in PATH
      char* cmd = args[0];

      // Search PATH for executable
      char* path = getenv("PATH");
      if (path == NULL) path = "";

      char path_cpy[1000];
      strncpy(path_cpy, path, sizeof(path_cpy));
      path_cpy[sizeof(path_cpy) - 1] = '\0';

      char* dir = strtok(path_cpy, ":");
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
        if (redirect_stdout_index != -1) {
          int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
          if (fd < 0) {
            perror("open");
            exit(1);
          }
          dup2(fd, 1);
          close(fd);
        }
        if (redirect_stderr_index != -1) {
          int fd = open(errfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
          if (fd < 0) {
            perror("open");
            exit(1);
          }
          dup2(fd, 2);
          close(fd);
        }
        if (append_stdout_index != -1) {
          int fd = open(append_stdout_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
          if (fd < 0) {
            perror("open");
            exit(1);
          }
          dup2(fd, 1);
          close(fd);
        }
        if (append_stderr_index != -1) {
          int fd = open(append_stderr_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
          if (fd < 0) {
            perror("open");
            exit(1);
          }
          dup2(fd, 2);
          close(fd);
        }
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
    free(line);
  }
  return 0;
}
