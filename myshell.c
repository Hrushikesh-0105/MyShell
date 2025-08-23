#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()

#define MAX_COMMANDS 10 // Max number of commands separated by a delimiter (e.g., cmd1 && cmd2 && ...)
#define MAX_ARGS 10     // Max number of arguments per command (e.g., ls -l -a)

// Enum to represent the type of operation
typedef enum {
    SINGLE,
    SEQUENTIAL, // ##
    PARALLEL,   // &&
    PIPE,       // |
    REDIRECTION // >
} CommandType;

// Structure to hold all the parsed information from the user input
typedef struct {
    char* commands[MAX_COMMANDS][MAX_ARGS]; // Array of commands, each with its arguments
    int num_commands;                       // Total number of commands found
    CommandType type;                       // The type of operation
    int redirection;                       // Is output redirection present or not
    char* redirection_file;                 // Filename for redirection
} ParsedCommand;


// Forward declarations for execution functions
void executeCommand(ParsedCommand* cmd);
void executeParallelCommands(ParsedCommand* cmd);
void executeSequentialCommands(ParsedCommand* cmd);
void executeCommandRedirection(ParsedCommand* cmd);
// Note: Pipe execution would need its own function, e.g., executePipeCommands(ParsedCommand* cmd);

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
    int i = 0;
    char* token;
    command_str = trimWhitespace(command_str);

    //split the string by spaces
    while ((token = strsep(&command_str, " ")) != NULL && i < MAX_ARGS - 1) {
        if (strlen(token) > 0) {
            command_args[i] = token;
            i++;
        }
    }
    command_args[i] = NULL; // The command list must be terminated with null for execvp
}


// Main parsing function, takes raw input and makes a struct out of it
// Returns 1 on success, 0 on parsing error
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
        if (cmd->commands[0][0] == NULL) { // Handle empty input
            cmd->num_commands = 0;
        }
    } else if (cmd->type == REDIRECTION) {
        //redirection should have one command then one output file
        char* command_part = strsep(&input, ">");
        char* file_part = input; // The rest of the string after the first >

        //trim both the parts
        command_part = trimWhitespace(command_part);
        file_part = trimWhitespace(file_part);

        if (strlen(command_part) == 0 || strlen(file_part) == 0 || strstr(file_part, " ") != NULL) {
            // no command or file name, or there is a space in the file part, so error
            return 0;
        }
        
        //checking for other special characters in command part
        if (strstr(command_part, "##") || strstr(command_part, "&&") || strstr(command_part, "|")) {
            return 0; 
        }

        cmd->redirection = 1;
        cmd->redirection_file = file_part;
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
                // Found a delimiter. The token is the string before it.
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
                // Found an empty command between delimiters, e.g., "ls && && pwd"
                return 0;
            }
            
            parseSingleCommand(token, cmd->commands[i]);
            i++;
        }
        
        cmd->num_commands = i;
        if (cmd->num_commands <= 1) {
            // If we detected a delimiter but only parsed one command, it's an error
            return 0;
        }
    }
    return 1;
}


void printParsedCommand(ParsedCommand* cmd) {
    if (cmd->num_commands == 0) {
        return;
    }

    printf("--- Parsed Data ---\n");
    switch(cmd->type) {
        case SINGLE: printf("Type: SINGLE\n"); break;
        case SEQUENTIAL: printf("Type: SEQUENTIAL (##)\n"); break;
        case PARALLEL: printf("Type: PARALLEL (&&)\n"); break;
        case PIPE: printf("Type: PIPE (|)\n"); break;
        case REDIRECTION: printf("Type: REDIRECTION (>)\n"); break;
    }

    printf("Number of commands: %d\n", cmd->num_commands);
    for (int i = 0; i < cmd->num_commands; i++) {
        printf("  Command %d: ", i + 1);
        for (int j = 0; cmd->commands[i][j] != NULL; j++) {
            printf("\"%s\" ", cmd->commands[i][j]);
        }
        printf("\n");
    }

    if (cmd->redirection) {
        printf("Redirection File: \"%s\"\n", cmd->redirection_file);
    }
    printf("-------------------\n");
}

void printError(){
    printf("Shell: Incorrect command\n");
}

void executeCommand(ParsedCommand* cmd){

	printf("Executing single command: %s\n", cmd->commands[0][0]);
    //executing the command
    if(strcmp(cmd->commands[0][0],"cd")==0){
        // then the command is cd and it has to be handled separately
        if(cmd->commands[0][1]==NULL){
            // then args of cd are missing
            printError();
        }
        else{
            if(chdir(cmd->commands[0][1])!=0){
                //error
                perror(""); 
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
            //child process
            if(execvp(cmd->commands[0][0], cmd->commands[0])<0){
                perror(cmd->commands[0][0]);
                exit(EXIT_FAILURE);
            }
        }
        else{
            wait(NULL);//wait for child to complete
        }
    }
}

void executeParallelCommands(ParsedCommand* cmd)
{
	// This function will run multiple commands in parallel
    // The commands are in cmd->commands[0], cmd->commands[1], etc.
    // The number of commands is cmd->num_commands
	printf("Executing %d parallel commands.\n", cmd->num_commands);
    printParsedCommand(cmd);
}

void executeSequentialCommands(ParsedCommand* cmd)
{	
	// This function will run multiple commands sequentially
    // The commands are in cmd->commands[0], cmd->commands[1], etc.
    // The number of commands is cmd->num_commands
	printf("Executing %d sequential commands.\n", cmd->num_commands);
    printParsedCommand(cmd);
}

void executeCommandRedirection(ParsedCommand* cmd)
{
	// This function will run a single command with output redirected
    // The command is in cmd->commands[0]
    // The output file is in cmd->redirection_file
	printf("Executing command '%s' with output redirected to '%s'\n", cmd->commands[0][0], cmd->redirection_file);
    printParsedCommand(cmd);
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

    // Note: The new parser handles leading/trailing whitespace, so this is not strictly needed anymore,
    // but it's good practice to keep it.
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

        // If parsing is successful but there are no commands (e.g., empty line), just show the prompt again.
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
		
        // Use the parsed command type to call the correct execution function
        printf("Command parsed\n");
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
                // executePipeCommands(&cmd_data); // You would implement this for the bonus part
                printf("Pipes are not implemented yet.\n");
                break;
            case SINGLE:
                executeCommand(&cmd_data);
                break;
            default:{
                printError();
            }
        }
        free(input); // Free the input string for the next loop
	}
	
	return 0;
}