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

void save_history_to_file(const char* filepath) {
  if (filepath == NULL) return;
  
  FILE* fp = fopen(filepath, "w");
  if (!fp) return;
   
  
  HIST_ENTRY** hist_list = history_list();
  if (hist_list) {
    for (int i = 0; hist_list[i] != NULL; i++) {
      fprintf(fp, "%s\n", hist_list[i]->line);
    }
  }
  
  fclose(fp);
}

int is_builtin(const char* cmd) {
  return (strcmp(cmd, "echo") == 0 || 
          strcmp(cmd, "type") == 0 || 
          strcmp(cmd, "pwd") == 0 ||
          strcmp(cmd, "cd") == 0 ||
          strcmp(cmd, "exit") == 0 ||
          strcmp(cmd, "history") == 0);
}

void execute_builtin_in_child(char* args[]) {
  char* cmd = args[0];
  
  if (strcmp(cmd, "echo") == 0) {
    // Count args
    int count = 0;
    while (args[count] != NULL) count++;
    
    for (int i = 1; i < count; i++) {
      printf("%s", args[i]);
      if (i < count - 1) printf(" ");
    }
    printf("\n");
  }
  else if (strcmp(cmd, "type") == 0) {
    if (args[1] == NULL) {
      exit(0);
    }
    char* target = args[1];
    
    if (is_builtin(target)) {
      printf("%s is a shell builtin\n", target);
    } else {
      // Check PATH
      char* path = getenv("PATH");
      if (path == NULL) path = "";
      char path_cpy[1000];
      strncpy(path_cpy, path, sizeof(path_cpy));
      path_cpy[sizeof(path_cpy) - 1] = '\0';
      
      char* dir = strtok(path_cpy, ":");
      int found = 0;
      
      while (dir != NULL) {
        char fullpath[1200];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, target);
        
        if (access(fullpath, X_OK) == 0) {
          printf("%s is %s\n", target, fullpath);
          found = 1;
          break;
        }
        dir = strtok(NULL, ":");
      }
      if (!found) {
        printf("%s: not found\n", target);
      }
    }
  }
  else if (strcmp(cmd, "pwd") == 0) {
    char cwd[1000];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s\n", cwd);
    }
  }
}

int main(int argc, char* argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  rl_bind_key('\t', rl_complete);
  rl_attempted_completion_function = completion_hook;

  int last_appended_index = 0;  // Track last appended history entry

  //Loading history from HISTFILE as real OS shell does, same as history -r done later
  char* histfile = getenv("HISTFILE");
  if (histfile != NULL) {
    FILE* fp = fopen(histfile, "r");
    if (fp) {
      char buf[1024];
      while (fgets(buf, sizeof(buf), fp)) {
        // Remove newline
        buf[strcspn(buf, "\n")] = 0;
        
        // Skip empty lines
        if (strlen(buf) == 0) continue;
        
        add_history(buf);
      }
      fclose(fp);
    }
  }

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
      save_history_to_file(histfile);
      exit(0);
    }

    // ECHO
    else if (strncmp(line, "echo", 4) == 0) {
      char* args2[20];
      int argc_echo = 0;
      tokenize(line + 5, args2, &argc_echo);

      // Check if there's a pipe - if so, skip this handler and let pipeline handle it
      int has_pipe = 0;
      for (int r = 0; r < argc_echo; r++) {
        if (strcmp(args2[r], "|") == 0) {
          has_pipe = 1;
          break;
        }
      }
      
      if (!has_pipe) {
        // Handle echo without pipeline
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
          strcmp(cmd, "pwd") == 0 || strncmp(cmd, "cd", 2) == 0 ||
          strncmp(cmd, "history", 7) == 0) {
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
        if (path[1] == '\0') {
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

    // HISTORY
    else if (strncmp(line, "history", 7) == 0) {

      // add history to a file (append basically )
      if (strncmp(line, "history -a ", 11) == 0) {
        char* filepath = line + 11;

        FILE* fp = fopen(filepath, "a");
        if (!fp) {
          printf("history: cannot open %s\n", filepath);
          continue;
        }

        HIST_ENTRY** hist_list = history_list();
        if (hist_list) {
          int total = 0;
          while (hist_list[total] != NULL) {
            total++;
          }
          for (int i = last_appended_index; i < total; i++) {
            fprintf(fp, "%s\n", hist_list[i]->line);
          }
          
          
          last_appended_index = total;
        }

        fclose(fp);
        continue;
      }

      // write history to file
      if (strncmp(line, "history -w ", 11) == 0) {
        char* filepath = line + 11;

        FILE* fp = fopen(filepath, "w");
        if (!fp) {
          printf("history: cannot open %s\n", filepath);
          continue;
        }

        HIST_ENTRY** hist_list = history_list();
        if (hist_list) {
          int total = 0;
          while (hist_list[total] != NULL) {
            total++;
          }

          for (int i = 0; i < total; i++) {
            fprintf(fp, "%s\n", hist_list[i]->line);
          }
        }

        fclose(fp);
        continue;
      }

      // Case: history -r <file>
      if (strncmp(line, "history -r ", 11) == 0) {
        char* filepath = line + 11;

        FILE* fp = fopen(filepath, "r");
        if (!fp) {
          printf("history: cannot open %s\n", filepath);
          continue;
        }

        char buf[1024];
        while (fgets(buf, sizeof(buf), fp)) {
          // remove newline
          buf[strcspn(buf, "\n")] = 0;

          // skip empty lines
          if (strlen(buf) == 0) continue;

          add_history(buf);
        }

        fclose(fp);
        continue;
      }

      int limit = -1;  // -1 means show all

    
      if (strlen(line) > 7 && line[7] == ' ' && line[8] >= '0' && line[8] <= '9') {
        char* arg = line + 8;
        limit = atoi(arg);
      }

      HIST_ENTRY** hist_list = history_list();
      if (hist_list) {
        int total = 0;
        while (hist_list[total] != NULL) {
          total++;
        }

        // Calculate starting index
        int start = 0;
        if (limit > 0 && limit < total) {
          start = total - limit;
        }

        for (int i = start; i < total; i++) {
          printf("%5d  %s\n", i + 1, hist_list[i]->line);
        }
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

      // Check for pipeline
      int pipe_index = -1;
      for (int k = 0; k < argc2; k++) {
        if (strcmp(args[k], "|") == 0) {
          pipe_index = k;
          break;
        }
      }

      // Handle pipeline with two commands
      if (pipe_index != -1) {
        // Split commands
        char* cmd1_args[20];
        char* cmd2_args[20];

        for (int i = 0; i < pipe_index; i++) {
          cmd1_args[i] = args[i];
        }
        cmd1_args[pipe_index] = NULL;

        int cmd2_argc = 0;
        for (int i = pipe_index + 1; i < argc2; i++) {
          cmd2_args[cmd2_argc++] = args[i];
        }
        cmd2_args[cmd2_argc] = NULL;

        // Check if commands are built-ins or external
        int cmd1_is_builtin = is_builtin(cmd1_args[0]);
        int cmd2_is_builtin = is_builtin(cmd2_args[0]);

        // Find external executables if needed
        char exec_path1[1200] = {0};
        char exec_path2[1200] = {0};
        
        if (!cmd1_is_builtin) {
          char* path = getenv("PATH");
          if (path == NULL) path = "";
          char path_cpy1[1000];
          strncpy(path_cpy1, path, sizeof(path_cpy1));
          path_cpy1[sizeof(path_cpy1) - 1] = '\0';
          
          int found1 = 0;
          char* dir = strtok(path_cpy1, ":");
          while (dir != NULL) {
            snprintf(exec_path1, sizeof(exec_path1), "%s/%s", dir, cmd1_args[0]);
            if (access(exec_path1, X_OK) == 0) {
              found1 = 1;
              break;
            }
            dir = strtok(NULL, ":");
          }
          
          if (!found1) {
            printf("%s: command not found\n", cmd1_args[0]);
            continue;
          }
        }

        if (!cmd2_is_builtin) {
          char* path = getenv("PATH");
          if (path == NULL) path = "";
          char path_cpy2[1000];
          strncpy(path_cpy2, path, sizeof(path_cpy2));
          path_cpy2[sizeof(path_cpy2) - 1] = '\0';
          
          int found2 = 0;
          char* dir = strtok(path_cpy2, ":");
          while (dir != NULL) {
            snprintf(exec_path2, sizeof(exec_path2), "%s/%s", dir, cmd2_args[0]);
            if (access(exec_path2, X_OK) == 0) {
              found2 = 1;
              break;
            }
            dir = strtok(NULL, ":");
          }
          
          if (!found2) {
            printf("%s: command not found\n", cmd2_args[0]);
            continue;
          }
        }

        // Create pipe
        int pipefd[2];
        if (pipe(pipefd) == -1) {
          perror("pipe");
          continue;
        }

        // Fork first command
        pid_t pid1 = fork();
        if (pid1 < 0) {
          perror("fork");
          close(pipefd[0]);
          close(pipefd[1]);
          continue;
        }

        if (pid1 == 0) {
          // First child: write to pipe
          close(pipefd[0]);    // Close read end
          dup2(pipefd[1], 1);  // Redirect stdout to pipe write end
          close(pipefd[1]);
          
          if (cmd1_is_builtin) {
            execute_builtin_in_child(cmd1_args);
            exit(0);
          } else {
            execv(exec_path1, cmd1_args);
            perror("execv failed");
            exit(1);
          }
        }

        // Fork second command
        pid_t pid2 = fork();
        if (pid2 < 0) {
          perror("fork");
          close(pipefd[0]);
          close(pipefd[1]);
          waitpid(pid1, NULL, 0);
          continue;
        }

        if (pid2 == 0) {
          // Second child: read from pipe
          close(pipefd[1]);    // Close write end
          dup2(pipefd[0], 0);  // Redirect stdin to pipe read end
          close(pipefd[0]);
          
          if (cmd2_is_builtin) {
            execute_builtin_in_child(cmd2_args);
            exit(0);
          } else {
            execv(exec_path2, cmd2_args);
            perror("execv failed");
            exit(1);
          }
        }

        // Parent: close both ends and wait for both children
        close(pipefd[0]);
        close(pipefd[1]);

        int status1, status2;
        waitpid(pid1, &status1, 0);
        waitpid(pid2, &status2, 0);

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
  
  // Save history before exiting
  save_history_to_file(histfile);
  
  return 0;
}
