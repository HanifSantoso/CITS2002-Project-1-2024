//  CITS2002 Project 1 2024
//  Student1:   23751927   Jenna Milford-1
//  Student2:   23970785   Hanif Santoso-2
//  Platform:   Linux

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

#define CFILENAME_SIZE 128
#define EXEFILENAME_SIZE 128
#define COMMANDLINE_SIZE 256
#define FUNCVAR_SIZE 50
#define MAX_ID_LENGTH 12
#define MAX_LINE_LENGTH 256
#define MAX_IDENTIFIERS 50

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    // open .ml file
    FILE *mlFile = fopen(argv[1], "r");
    if (mlFile == NULL) {
        fprintf(stderr, "Cannot find filepath:%s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    // validate the .ml program
    if (!validateSyntax(mlFile)) {
        return EXIT_FAILURE;
    }

    // generate temporary C file
    char temporaryC[CFILENAME_SIZE];
        snprintf(temporaryC, CFILENAME_SIZE, "ml-%d.c", getpid()); // snprintf used to prevent buffer overflows

    // translate .ml to C
    // put code here

    fclose(mlFile); // closing ml file

    // compile C file to executable
    char cExecutable[EXEFILENAME_SIZE];

    snprintf(cExecutable, EXEFILENAME_SIZE, "ml-%d", getpid()); // using spnrintf to prevent potential buffer overflows

    if (!compileProgram(temporaryC, cExecutable)) {
        return EXIT_FAILURE;
    }

    // execute program
    int executeResult = executeProgram(cExecutable, argc - 2, argv + 2);

    // clean up temporary
    remove(temporaryC);
    remove(cExecutable);

    return executeResult;
}

int executeProgram(const char *exeName, int argc, char *argv[]) {
    char command[COMMANDLINE_SIZE];
    // initializing command buffer with the filename
    snprintf(command, sizeof(COMMANDLINE_SIZE), "./%s", exeName);

    // append each argument to the command string
    for (int i = 0; i < argc; i++) {
        // safely append a space followed by the argument
        strncat(command, " ", sizeof(COMMANDLINE_SIZE) - strlen(command) - 1);
        strncat(command, argv[i], sizeof(COMMANDLINE_SIZE) - strlen(command) - 1);
    }

    // execute the command
    int status = system(command);
    if (status != 0) {
        // print an error message if execution fails
        fprintf(stderr, "Error: Execution of '%s' failed\n", exeName);
        return 0;
    }
    return 1;
}

int compileProgram(const char *programName, const char *exeName) {
    char command[COMMANDLINE_SIZE];
    
    // construct the gcc command
    snprintf(command, sizeof(COMMANDLINE_SIZE), "gcc -std=c11 %s -o %s", programName, exeName);

    // execute the gcc command
    int status = system(command);
    
    // check if the compilation worked
    if (status != 0) {
        fprintf(stderr, "Error: Compilation of '%s' failed\n", programName);
        return 0; // return 0 for failure
    }

    return 1; // return 1 for success
}

// function to check if a string is a valid identifier
bool isValidID(const char *str) {
    int len = strlen(str);
    if (len < 1 || len > MAX_ID_LENGTH) return false;

    // check if characters are lowercase alphabetic
    for (int i = 0; i < len; i++) {
        if (!islower(str[i])) return false;
    }
    return true;
}

// function to validate .ml file
int validateSyntax(FILE *mlFile) {
    char line[MAX_LINE_LENGTH];
    int identifierCount = 0;
    char identifiers[MAX_IDENTIFIERS][MAX_ID_LENGTH + 1]; // store up to 50 unique identifiers

    while (fgets(line, sizeof(line), mlFile)) {
        // strip newline character
        line[strcspn(line, "\n")] = 0;

        // skip empty lines
        if (strlen(line) == 0) {
            continue;
        }

        // check for comments
        if (line[0] == '#') {
            continue; // comment line, skip
        }

        // check for valid identifiers (1-12 lowercase characters)
        char first_word[MAX_ID_LENGTH + 1];
        sscanf(line, "%s", first_word);  // get the first word on the line

        if (!isValidID(first_word)) {
            fprintf(stderr, "Invalid identifier: %s\n", first_word);
            return 0;
        }

        // ensure no more than 50 unique identifiers
        bool found = false;
        for (int i = 0; i < identifierCount; i++) {
            if (strcmp(identifiers[i], first_word) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            if (identifierCount >= MAX_IDENTIFIERS) {
                fprintf(stderr, "Error: Exceeded maximum number of unique identifiers (50)\n");
                return 0;
            }
            strcpy(identifiers[identifierCount], first_word);
            identifierCount++;
        }

        // ensure statements are one per line (no semicolon at the end)
        if (line[strlen(line) - 1] == ';') {
            fprintf(stderr, "Error: Statements should not have a terminating semicolon\n");
            return 0;
        }
    }

    return 1;
}

bool isFloat(const char *str) {
    bool dotFound = false;

    // skip leading whitespace
    while (isspace(*str)) {
        str++;
    }

    // check if the string starts with a digit or a dot
    if (!isdigit(*str) && *str != '.') {
        return false; // must start with a digit or dot
    }

    // check the rest of the string
    while (*str) {
        if (*str == '.') {
            if (dotFound) {
                return false; // multiple dots are not allowed
            }
            dotFound = true;
        } else if (!isdigit(*str)) {
            return false; // invalid character for a float
        }
        str++;
    }

    return dotFound; // must contain at least one dot to be a valid float
}

int isInteger(const char *str) {
    while (*str) {
        if (!isdigit(*str)) {
            return 0; // not integer
        }
        str++;
    }
    return 1; // is integer
}

bool is_variable(const char *expression, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int var_count) {
    // simple check for the expression if it contains variables
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i], expression) == 0) {
            return true;
        }
    }
    return false;
}

bool is_function_call(const char *expression) {
    // a simple check to see if the expression contains a function call like `function_name(args)`
    return (strchr(expression, '(') && strchr(expression, ')'));
}

void translate_function(char fName[3][MAX_ID_LENGTH], char *fParam[], char *fBody[], char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int *var_count, FILE *cFile) {
    int ch = 0;
    char *r_type = ""; // return type
    bool foundReturn = false; // flag to track if return was found
    char varName[MAX_ID_LENGTH];
    char value[MAX_ID_LENGTH];
    char paramList[MAX_LINE_LENGTH] = "";
    char expression[MAX_LINE_LENGTH];

    // determine return type based on the function body
    while (fBody[ch] != NULL && fBody[ch][0] != '\0') {
        if (strncmp(fBody[ch], "\treturn", 7) == 0) {
            r_type = "float";  // assuming all functions that return use float
            foundReturn = true;
        }
        ch++;
    }

    if (!foundReturn) {
        r_type = "void";  // default to void if no return statement is found
    }

    // reset ch to process the body
    ch = 0;

    // write function signature to the C file

    int paramCount = atoi(fName[1]);  // convert fName[1] to an integer (number of parameters)

    // Safeguard: ensure paramCount does not exceed the size of fParam
    if (paramCount < 0 || paramCount > MAX_IDENTIFIERS) {
        fprintf(stderr, "Invalid number of parameters: %d\n", paramCount);
        return;  // Exit if parameter count is invalid
    }

    // Safeguard: ensure paramList has enough space
    for (int i = 0; i < paramCount; i++) {
        // check if the current parameter is valid (not NULL)
        if (fParam[i] == NULL) {
            fprintf(stderr, "Invalid parameter at index %d\n", i);
            return;
        }
    
        // ensure there's enough space in paramList to concatenate the parameter and a comma
        if (strlen(paramList) + strlen(fParam[i]) + 3 > MAX_LINE_LENGTH) {  // +3 for ", " and null-terminator
            fprintf(stderr, "Parameter list too long to fit in buffer\n");
            return;
        }
    
        strcat(paramList, fParam[i]);  // concatenate the parameter name
    
        if (i < paramCount - 1) {
            strcat(paramList, ", ");  // add a comma and space between parameters, except after the last one
        }
    }

    // Using paramList in the function declaration
    fprintf(cFile, "function %s(%s) {\n", fName[0], paramList);  // using fName[0] (the function name) and paramList

    // parse body of function
    while (fBody[ch] != NULL && fBody[ch][0] != '\0') {
        if (strncmp(fBody[ch], "\treturn", 7) == 0) {
            int i = 7;  // offset for the start of the return expression

            // extract the return expression
            int exprIndex = 0;
            while (fBody[ch][i] != '\n' && fBody[ch][i] != '\0' && exprIndex < sizeof(expression) - 1) {
                expression[exprIndex++] = fBody[ch][i++];
            }
            expression[exprIndex] = '\0';  // null-terminate the expression

            // write the return statement to the C file
            fprintf(cFile, "    return%s;\n", expression);
        } 
        else if (strncmp(fBody[ch], "\tprint", 6) == 0) {
            int i = 6;  // offset for the start of the print expression

            // extract the print expression
            int exprIndex = 0;
            while (fBody[ch][i] != '\n' && fBody[ch][i] != '\0' && exprIndex < sizeof(expression) - 1) {
                expression[exprIndex++] = fBody[ch][i++];
            }
            expression[exprIndex] = '\0';  // null-terminate the expression

            // check if the expression is a known variable
            bool is_var = false;
            for (int i = 0; i < (*var_count); i++) {
                if (strcmp(variables[i], expression) == 0) {
                    is_var = true;
                }
            }

            // print based on type checking
            if (is_var || is_integer(expression)) {
                fprintf(cFile, "    printf(\"%s\", %s);\n", is_integer(expression) ? "%d" : "%f", expression);
            } else {
                fprintf(stderr, "Unknown variable or invalid expression in print statement: %s\n", expression);
            }
        } 
        else if (sscanf(fBody[ch], "%s <- %[^\n]", varName, expression) == 2) {
            // Check if the variable has already been declared
            bool alreadyDeclared = false;
            for (int i = 0; i < *var_count; i++) {
                if (strcmp(variables[i], varName) == 0) {
                    alreadyDeclared = true;
                }
            }

            if (!alreadyDeclared) {
                // Declare the variable if it's not already declared
                fprintf(cFile, "    float %s;\n", varName);
                strcpy(variables[*var_count], varName);
                (*var_count)++;
            }

            // Output the full expression to the C file
            fprintf(cFile, "    %s = %s;\n", varName, expression);
        } else {
            fprintf(stderr, "Syntax error in variable assignment: %s\n", fBody[ch]);
        }
        ch++;
    }

    // close the function in the C file
    fprintf(cFile, "}\n");
}