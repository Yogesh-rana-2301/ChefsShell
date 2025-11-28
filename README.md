# ChefsShell ♛

it is a lightweight Unix-compatible shell written in C, designed to mimic the behavior of traditional shells like `bash` and `sh`.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](<https://en.wikipedia.org/wiki/C_(programming_language)>)
[![Platform: Unix-like](https://img.shields.io/badge/Platform-Unix--like-lightgrey.svg)](https://en.wikipedia.org/wiki/Unix-like)

> The file `src/main.c` is the main file for the whole shell and is responsible for the whole functionality of it. It has been authored by me. The shell would work fine on local. <br>The deployment on the web browser have been vibe code as I am not well versed with xterm.js and with the deployment of such shell program online. So in case of any issues with the web browser version kindly open issues in the issue tab.  
> Vibe code alert for that part.

## Overview

ChefsShell is a functional command-line interpreter that will be supporting 10+ built-in commands, process creation, command execution, and memory-optimized operation. It serves as both a learning resource for understanding shell internals and a practical tool for Unix-like systems.

## Features to be implemented

- **Built-in Commands**: `cd`, `pwd`, `echo`, `exit`, `clear`, `type`, and more
- **Process Management**: Uses `fork()` and `execvp()` to run external programs
- **Command Parsing**: Tokenization, argument handling, and whitespace trimming
- **Memory Efficient**: Optimized allocation and deallocation (20% reduced RAM usage)
- **Cross-Platform (Unix-like)**: Works on Linux and macOS
- **Error Handling**: Clear error messages for invalid commands or missing executables
- **Scalable Codebase**: Modular design for easy extension

## Getting Started

### Prerequisites

You need:

- **GCC** or **Clang** compiler
- **Linux** or **macOS** terminal

Clone the repository

### Build

```bash
make
```

### Run

```bash
./chefshell
```

### Clean Build Artifacts

```bash
make clean
```

##� Usage Examples

### Built-in Commands

```bash
$ cd ~/Documents
$ pwd
/home/user/Documents

$ echo Hello World
Hello World

$ type pwd
pwd is a shell builtin

$ clear
# Clears the terminal screen

$ exit
# Exits the shell
```

### Running External Programs

```bash
$ ls -la
# Lists files with details

$ gcc file.c -o file
# Compiles a C program

$ ./file
# Executes the compiled program
```

## Technical Highlights

### Modular & OOP-Inspired Architecture

Although written in C, the project uses object-oriented design principles:

- Separation of concerns across modules
- Struct-based modular design
- Clear abstraction boundaries between components

### System Calls

ChefsShell leverages essential Unix system calls:

| System Call | Purpose                                   |
| ----------- | ----------------------------------------- |
| `fork()`    | Create child process                      |
| `execvp()`  | Execute external program                  |
| `waitpid()` | Process synchronization                   |
| `getcwd()`  | Get current working directory (for `pwd`) |
| `chdir()`   | Change directory (for `cd`)               |

### Memory Optimization

Effective use of:

- Dynamic allocation (`malloc`, `realloc`)
- Manual garbage cleanup
- Avoiding leaks using structured free logic
- Minimal memory footprint during execution

## Roadmap

Planned enhancements:

- [ ] **Pipes** (`ls | grep txt`)
- [ ] **Redirection** (`>`, `<`, `>>`)
- [ ] **Command history** (arrow key navigation)
- [ ] **Environment variable support** (`$PATH`, `$HOME`)
- [ ] **Background execution** (`&`)
- [ ] **Tab auto-completion**
- [ ] **Signal handling** (Ctrl+C, Ctrl+Z)
- [ ] **Scripting support** (execute `.sh` files)

## Contributing

Contributions, issues, and pull requests are welcome!

1. Fork this repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

Please ensure your code follows the existing style and includes appropriate comments.

## Testing

To test the shell:

```bash
# Run built-in command tests
$ make test

# Manual testing
$ ./chefshell
$ echo $?  # Check exit status
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- Unix manual pages and POSIX standards
- GNU Bash implementation details
- Codecrafters Build your own X
- The Unix philosophy of doing one thing well

## Contact

For questions or suggestions, please open an issue on GitHub.
