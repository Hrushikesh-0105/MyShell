# Custom Shell Implementation in C

A lightweight Unix-like command-line shell implemented in C using Linux system calls. This project demonstrates core operating system concepts such as process creation, process management, inter-process communication (IPC), file descriptors, I/O redirection, and signal handling.

## Features

- **Command Execution:** Execute standard Linux commands using `fork()` and `execvp()`.
- **Sequential Execution (`##`):** Execute multiple commands one after another, waiting for each command to finish before starting the next.
- **Parallel Execution (`&&`):** Execute multiple commands concurrently by creating separate child processes and waiting for all of them to complete.
- **Piping (`|`):** Connect multiple commands using pipes, allowing the output of one command to serve as the input of the next.
- **Output Redirection (`>`):** Redirect command output to a file using file descriptor manipulation.
- **Directory Navigation (`cd`):** Change the current working directory using `chdir()`.
- **Signal Handling:** Handle `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z) to prevent the shell itself from being interrupted or suspended.
- **Quoted Arguments:** Support double-quoted arguments, including arguments containing spaces.
- **Dynamic Prompt:** Display the current working directory in the shell prompt.
- **Exit Handling:** Exit the shell using the `exit` command or Ctrl+D.

## Tech Stack

- **Language:** C
- **Platform:** Linux / Unix-like systems
- **System Calls & APIs:**
  - `fork()` – Create child processes.
  - `execvp()` – Execute external commands.
  - `wait()` / `waitpid()` – Synchronize processes.
  - `pipe()` – Establish communication between processes.
  - `dup2()` – Redirect standard input and output.
  - `open()` / `close()` – File handling and descriptor management.
  - `chdir()` – Change working directory.
  - `signal()` – Handle system signals.
  - `getcwd()` – Retrieve the current working directory.

## Getting Started

### Prerequisites

- GCC compiler
- Linux or a Unix-like operating system
- Terminal access

### Compilation

Clone the repository:

```bash
git clone <repository-url>
cd <repository-name>
```

Compile the source code:

```bash
gcc shell.c -o shell
```

Run the shell:

```bash
./shell
```

## Usage

Once started, the shell displays the current working directory followed by `$`, where commands can be entered.

### 1. Single Command Execution

Execute standard Linux commands:

```bash
ls -la
pwd
date
whoami
```

### 2. Sequential Command Execution (`##`)

Use `##` to execute commands sequentially. The next command starts after the previous one completes.

```bash
pwd ## ls ## date
```

### 3. Parallel Command Execution (`&&`)

Use `&&` to execute multiple commands concurrently.

```bash
sleep 3 && ls && date
```

All commands are started before the shell waits for their completion.

### 4. Piping (`|`)

Use `|` to pass the output of one command as the input of another.

```bash
ls | wc -l
```

Multiple pipes are supported:

```bash
cat file.txt | grep "hello" | wc -l
```

### 5. Output Redirection (`>`)

Redirect standard output to a file. If the file does not exist, it is created. If it already exists, its contents are overwritten.

```bash
ls > output.txt
```

File paths containing spaces can be enclosed in double quotes:

```bash
ls > "output file.txt"
```

### 6. Directory Navigation

Change the current working directory:

```bash
cd /home/user/Documents
```

### 7. Exit

```bash
exit
```

Alternatively, press `Ctrl+D`.

## Implementation Overview

The shell follows a simple command-processing architecture:

1. **Input Handling:** Read user input using `getline()` and remove unnecessary whitespace.
2. **Command Parsing:** Identify operators (`##`, `&&`, `|`, `>`) and tokenize commands and their arguments.
3. **Command Classification:** Store parsed information in a `ParsedCommand` structure, identifying the operation type and number of commands.
4. **Process Creation:** Use `fork()` to create child processes for executing external commands.
5. **Command Execution:** Use `execvp()` to replace child processes with the requested programs.
6. **Process Synchronization:** Use `wait()` and `waitpid()` to manage child process completion.
7. **Resource Management:** Manage pipes, file descriptors, dynamically allocated memory, and signals.

## Project Structure

```text
.
├── shell.c       # Main shell implementation
└── README.md     # Project documentation
```

## Operating System Concepts Demonstrated

This project provides practical implementation experience with:

- Process creation and execution
- Parent-child process relationships
- Process synchronization and waiting
- Concurrent and sequential execution
- Inter-process communication using pipes
- File descriptors and I/O redirection
- Standard input/output manipulation
- Signal handling
- System calls and process management

## Limitations

- Supports a limited set of shell operators: `##`, `&&`, `|`, and `>`.
- Does not support shell features such as background execution (`&`), input redirection (`<`), or command chaining with parentheses.
- Parsing is simplified and does not implement full shell syntax or escaping rules.
- Environment variable expansion, wildcard expansion, and command history are not implemented.
- Maximum limits are defined for the number of commands and arguments.

## Future Improvements

- Implement input redirection (`<`).
- Add background process execution.
- Support environment variables and wildcard expansion.
- Implement command history.
- Improve parsing to support more complex shell syntax.
- Add built-in commands such as `export`, `unset`, and `history`.

---

**Author:** Hrushikesh Musaloj

**Language:** C

**Project:** Custom Unix Shell Implementation
