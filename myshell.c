#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

#define MAX_COMMANDS 10 // Max number of commands separated by a delimiter: cmd1 && cmd2 && ..
#define MAX_ARGS 10     // Max number of arguments per command: ls -l -a

typedef enum {
    SINGLE,
    SEQUENTIAL, // ##
    PARALLEL,   // &&
    PIPE,       // |
    REDIRECTION // >
} CommandType;

// structure to hold all the parsed information from the user input
typedef struct {
    char* commands[MAX_COMMANDS][MAX_ARGS]; // array of commands, each with its arguments
    int num_commands;                       // total number of commands found
    CommandType type;                       // the type of operation
    int redirection;                       // is output redirection present or not
    char* redirection_file;                 // filename for redirection
} ParsedCommand;


//!problems
// cd with file name with spaces not working: cd "os lab"

void executeCommand(char* command[]);
void executeSingleCommand(ParsedCommand* cmd);
void executeParallelCommands(ParsedCommand* cmd);
void executeSequentialCommands(ParsedCommand* cmd);
void executeCommandRedirection(ParsedCommand* cmd);

char* trimWhitespace(char* str) {
    if (!str) return NULL;
    char* end;
    //trimming starting space
    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }

    if (*str == 0) { // if all are spaces the return
        return str;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) {
        end--;
    }

    //ending with null char
    *(end + 1) = 0;

    return str;
}

//Ex: "  ls -a -l  " => {"ls", "-a", "-l"}
void parseSingleCommand(char* command_str, char* command_args[MAX_ARGS]) {
    int arg_index = 0;
    char* current_pos = command_str;

    while (*current_pos != '\0' && arg_index < MAX_ARGS - 1) {
        //skip leading whitespace before the next arg
        while (*current_pos == ' ' || *current_pos == '\t') {
            current_pos++;
        }

        // If at the end of the string, break
        if (*current_pos == '\0') {
            break;
        }

        //Check if the argument is quoted
        if (*current_pos == '"') {
            // This is a quoted argument
            char* token_start = current_pos + 1; // The argument starts after the quote
            command_args[arg_index++] = token_start;
            
            // Find the closing quote
            char* end_quote = strchr(token_start, '"');
            if (end_quote != NULL) {
                // Found the closing quote. Terminate the string there.
                *end_quote = '\0';
                current_pos = end_quote + 1; // Continue parsing after the closing quote
            } else {
                // no closing quote found, treat the rest of the line as the argument
                break;
            }
        } else {
            // this is an unquoted arg
            char* token_start = current_pos;
            command_args[arg_index++] = token_start;

            // find the next space or the end of the string
            while (*current_pos != '\0' && *current_pos != ' ' && *current_pos != '\t') {
                current_pos++;
            }

            // if we found a space,end the string there
            if (*current_pos != '\0') {
                *current_pos = '\0';
                current_pos++; // for next loop
            }
        }
    }
    command_args[arg_index] = NULL; // arg list ends with NULL
}


// main parsing function, takes raw input and makes a struct out of it
// returns 1 on success, 0 on parsing error
int parseInput(char* input, ParsedCommand* cmd) {
    // filling the struct with zeros
    memset(cmd, 0, sizeof(ParsedCommand));
    cmd->type = SINGLE; // Default type

    char* delimiter = NULL;

    //detecting type of command
    if (strstr(input, "##")) {
        cmd->type = SEQUENTIAL;
        delimiter = "##";
    } else if (strstr(input, "&&")) {
        cmd->type = PARALLEL;
        delimiter = "&&";
    } else if (strstr(input, "|")) {
        cmd->type = PIPE;
        delimiter = "|";
    } else if (strstr(input, ">")) {
        cmd->type = REDIRECTION;
        delimiter = ">";
    }

    //split string based on the special charcters
    if (cmd->type == SINGLE) {
        //if only single command, then just parse it
        cmd->num_commands = 1;
        parseSingleCommand(input, cmd->commands[0]);
        if (cmd->commands[0][0] == NULL) { // handle empty input
            cmd->num_commands = 0;
        }
    } else if (cmd->type == REDIRECTION) {
        // breaking it down into command and file path
        char* command_part = strsep(&input, ">");
        char* file_part = input;

        //trim whitespace of both parts
        command_part = trimWhitespace(command_part);
        file_part = trimWhitespace(file_part);

        // initial validation neither part can be empty
        if (strlen(command_part) == 0 || strlen(file_part) == 0) {
            return 0; //missing command or file name
        }
        
        //no mixing of operators
        if (strstr(command_part, "##") || strstr(command_part, "&&") || strstr(command_part, "|")) {
            return 0; 
        }

        int file_len = strlen(file_part);
        if (file_part[0] == '"' && file_part[file_len - 1] == '"') {
            // if the file path starts with quotes, as the names may have spaces inside
            file_part[file_len - 1] = '\0';
            cmd->redirection_file = file_part + 1;
        } else {
            // if the name doesnt have any quotes, but has spaces 
            if (strstr(file_part, " ") != NULL) {
                return 0;
            }
            cmd->redirection_file = file_part;
        }

        // ensure the resulting filename is not empty ex: ls > ""
        if (strlen(cmd->redirection_file) == 0) {
            return 0;
        }
        
        cmd->redirection = 1;
        cmd->num_commands = 1;
        parseSingleCommand(command_part, cmd->commands[0]);
    } else {
        // This handles ##, &&, and |
        char* current_pos = input;
        int i = 0;
        
        while(current_pos != NULL && i < MAX_COMMANDS) {
            char* delimiter_pos = strstr(current_pos, delimiter);
            char* token;

            if (delimiter_pos != NULL) {
                // Found a delimiter,  The token is the string before it.
                *delimiter_pos = '\0'; // Manually terminate the token string
                token = current_pos;
                // Move current_pos past the delimiter and the null terminator
                current_pos = delimiter_pos + strlen(delimiter);
            } else {
                // No more delimiters found, the rest of the string is the last token
                token = current_pos;
                current_pos = NULL; // This will end the loop
            }

            token = trimWhitespace(token);
            if (strlen(token) == 0) {
                // found an empty command between delimiters, ex: "ls && && pwd"
                return 0;
            }
            
            parseSingleCommand(token, cmd->commands[i]);
            i++;
        }
        
        cmd->num_commands = i;
        if (cmd->num_commands <= 1) {
            // if we detected a delimiter but only parsed one command, error
            return 0;
        }
    }
    return 1;
}

void printError(){
    printf("Shell: Incorrect command\n");
}

void resetSignalHandlers(){
    // Reset signal handlers to their default behavior for the child.
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
}

void ignoreSignals(){
    // SIGINT-> interrupt signal ctrl+c
    // SIGTSTP-> stop signal, ctrl+z, we ignore this in parent
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
}


//different exection functions
void executeCommand(char* command[]){
    if(strcmp(command[0],"cd")==0){
        // then the command is cd and it has to be handled separately
        if(command[1]==NULL){
            // then args of cd are missing
            printError();
        }
        else{
            if(chdir(command[1])!=0){
                //error occured while changing dir
                // perror(command[0]); 
                // printError();
            }
        }
    }
    else{
        pid_t pid =fork();
        
        if(pid<0){
            //fork failed
            return;
        }
        if(pid==0){
            resetSignalHandlers();
            //child process
            if(execvp(command[0], command)<0){
                // perror(command[0]);
                exit(EXIT_FAILURE);
            }
        }
        else{
            wait(NULL);//wait for child to complete
        }
    }
}


void executeSingleCommand(ParsedCommand* cmd){
	// printf("Executing single command: %s\n", cmd->commands[0][0]);
    executeCommand(cmd->commands[0]);
}

void executeSequentialCommands(ParsedCommand* cmd){	
	// printf("Executing %d sequential commands.\n", cmd->num_commands);
    int n=cmd->num_commands;
    for(int i=0;i<n;i++){
        executeCommand(cmd->commands[i]);
    }
}

void executeParallelCommands(ParsedCommand* cmd){
	// printf("Executing %d parallel commands.\n", cmd->num_commands);
    // start all child processes first in one loop, then wait for all in a loop
    int n=cmd->num_commands;
    pid_t* pids = (pid_t*) malloc(sizeof(pid_t)*n);
    
    for(int i=0;i<n;i++){
        if(strcmp(cmd->commands[i][0],"cd")==0){
            // then the command is cd and it has to be handled separately
            if(cmd->commands[i][1]==NULL){
                // then args of cd are missing
                printError();
            }
            else{
                if(chdir(cmd->commands[i][1])!=0){
                    //error occured while changing dir
                    // perror(cmd->commands[i][0]); 
                }
            }
            pids[i] = 0; // Mark this as not a forked process
            continue;
        }
        pids[i]=fork();

        if(pids[i]<0){
            //fork failed
            // perror("");
            return;
        }
        
        if (pids[i] == 0) {
            // child process
            resetSignalHandlers();
            if (execvp(cmd->commands[i][0], cmd->commands[i]) < 0) {
                // perror(cmd->commands[i][0]);
                exit(EXIT_FAILURE);
            }
        }
    }
    // now wait for all the processes in parent
    for (int i = 0; i < n; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], NULL, 0);
        }
    }
    free(pids);
    pids=NULL;
}


void executeCommandRedirection(ParsedCommand* cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        // perror("");
        return;
    }
    if (pid == 0) {
        resetSignalHandlers();
        int fd = open(cmd->redirection_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            // If open fails, raise error
            // perror("");
            exit(EXIT_FAILURE);
        }
        //changing the stdout to file fd
        if (dup2(fd, STDOUT_FILENO) < 0) {
            // perror("");
            exit(EXIT_FAILURE);
        }

        
        //STDOUT_FILENO is now a copy of fd, so we dont need fd
        close(fd);

        if (execvp(cmd->commands[0][0], cmd->commands[0]) < 0) {
            // perror(cmd->commands[0][0]);
            exit(EXIT_FAILURE);
        }
    } else {
        //parent
        wait(NULL);
    }
}


void print_CWD(){
	char* path=NULL;
	path=getcwd(NULL,0);
	printf("%s$",path);
	free(path);
}

int read_input(char **out_str) {
    char *input = NULL;
    size_t len = 0;
    ssize_t nread;

    nread = getline(&input, &len, stdin);
    if (nread == -1) {
        free(input);
        return -1; // error or EOF, ctrl+d is pressed
    }

    // remove trailing newline
    if (nread > 0 && input[nread - 1] == '\n') {
        input[nread - 1] = '\0';
        nread--;
    }

    //removing white spaces at start and end
    char *start = input;
    while (*start && *start==' ') {
        start++;
    }
    char *end = input + strlen(input) - 1;
    while (end >= start && *end==' ') {
        *end-- = '\0';
    }

    char *trimmed = strdup(start);
    free(input);

    if (!trimmed) return -1; // malloc fail

    *out_str = trimmed;
    return strlen(trimmed);
}

int main()
{
	char *input = NULL;
    ParsedCommand cmd_data;

    // ignore signals , ctrl+c and ctrl+z in parent
    ignoreSignals();

	while(1)
	{
		print_CWD();
		
		int length = read_input(&input);

        if (length < 0) { // Handle Ctrl+D (EOF)
            printf("Exiting shell...\n");
            break;
        }

        // parse Input returns -1 on incorrect op
        if (parseInput(input, &cmd_data)==0) {
            printError();
            free(input);
            continue;
        }

        //if input is empty
        if (cmd_data.num_commands == 0) {
            free(input);
            continue;
        }
		
		// Check for the exit command
        if (strcmp(cmd_data.commands[0][0], "exit") == 0) {
            printf("Exiting shell...\n");
            free(input);
            break;
        }
		
        //acc to type of symbols execute particular functions
        switch(cmd_data.type) {
            case PARALLEL:
                executeParallelCommands(&cmd_data);
                break;
            case SEQUENTIAL:
                executeSequentialCommands(&cmd_data);
                break;
            case REDIRECTION:
                executeCommandRedirection(&cmd_data);
                break;
            case PIPE:
                //yet to implement pipes
                break;
            case SINGLE:
                executeSingleCommand(&cmd_data);
                break;
            default:{
                printError();
            }
        }
        free(input);
	}
	
	return 0;
}