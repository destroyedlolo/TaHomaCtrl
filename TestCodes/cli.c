#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// Define a type for the function pointer (takes no arguments and returns nothing)
typedef void (*command_func_t)(void);

// --- Command Execution Functions ---

/**
 * @brief Executes the 'hi' command.
 */
void do_hi(void) {
    printf("hello\n");
}

/**
 * @brief Executes the 'quit' command (handles exit internally).
 * * NOTE: We handle the actual exit in the main loop for proper readline cleanup.
 */
void do_quit(void) {
    // This function is defined mainly for structure, but the actual 'break' 
    // from the loop happens in 'main' after checking the command name.
}

// --- Command Dispatch Table Structure ---

/**
 * @brief Structure to define a command entry.
 * * This is our 2-dimensional table: name (string) and function pointer.
 */
typedef struct {
    const char *name;
    command_func_t function;
} Command;

// The array (our dispatch table)
Command command_table[] = {
    {"hi", do_hi},
    {"quit", do_quit},
    {NULL, NULL} // Termination marker
};

// --- Readline Autocompletion Functions ---

/**
 * @brief Generator function for completion matches.
 */
char *command_generator(const char *text, int state) {
    static int list_index, len;
    const char *name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    // Iterate through the command_table for names
    while ((name = command_table[list_index].name)) {
        list_index++;
        if (strncmp(name, text, len) == 0) {
            return (strdup(name));
        }
    }

    return ((char *)NULL);
}

/**
 * @brief Custom completion function for Readline.
 */
char **command_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1;
    if (start == 0) {
        // Use the command_table names for completion
        return rl_completion_matches(text, command_generator);
    }
    return ((char **)NULL);
}

// --- Main Execution Loop ---

/**
 * @brief Main execution loop of the interactive CLI.
 */
int main() {
    char *line;
    int i;

    // Set the custom completion function for TAB key
    rl_attempted_completion_function = command_completion;

    printf("Simple CLI Tool (Type 'hi' or 'quit')\n");

    while (1) {
        line = readline("CLI> "); 

        // 1. Check for EOF (Ctrl-D) or error
        if (line == NULL) {
            printf("\nExiting CLI (EOF).\n");
            break;
        }

        // 2. Add to history if not empty
        if (*line) {
            add_history(line);
        }

        // 3. Command processing: Find and execute the command function
        if (strcmp(line, "") == 0) {
            // Do nothing for an empty line
        } else {
            // Loop through the command table
            int found = 0;
            for (i = 0; command_table[i].name != NULL; i++) {
                if (strcmp(line, command_table[i].name) == 0) {
                    found = 1;

                    // If it's "quit", we break the main loop after execution
                    if (strcmp(line, "quit") == 0) {
                        printf("Exiting CLI.\n");
                        // We still call the function, although do_quit is empty
                        command_table[i].function(); 
                        goto end_loop; // Use goto to exit the main while loop gracefully
                    }

                    // Execute the function pointer!
                    command_table[i].function(); 
                    break; 
                }
            }

            if (!found) {
                printf("Unknown command: %s\n", line);
            }
        }

        // 4. Free the memory allocated by readline
        free(line);
    }

end_loop:
    // If we exit via 'quit' (using goto), we ensure 'line' is freed
    if (line != NULL) {
        free(line);
    }
    
    return 0;
}
