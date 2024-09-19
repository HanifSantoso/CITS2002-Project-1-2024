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
#define COMMAND_SIZE 1024
#define FUNCVAR_SIZE 50
#define MAX_ID_LENGTH 12
#define MAX_LINE_LENGTH 256
#define MAX_IDENTIFIERS 50

int executeProgram(const char *exeName, int argc, char *argv[]);
int compileProgram(const char *programName, const char *exeName);
void trimTrailingWhitespace(char *str);
bool isValidID(const char *str);
bool validateSyntax(FILE *mlFile);
bool isFloat(const char *str);
int isInteger(const char *str);
bool is_variable(const char *expression, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int varCount);
bool is_function_call(const char *expression);
void translate_function(char fName[3][MAX_ID_LENGTH], char *fParam[], char *fBody[], char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int *varCount, FILE *cFile);
int parseML(FILE *mlFile, FILE *cFile);
void printUsage(const char *prog_name);

// function to print details of the c program and how to use it.
void printUsage(const char *prog_name) {
    printf("Usage: %s <input_file> <output_file>\n", prog_name);
    printf("    <input_file>   The input file in custom markup language format.\n");
    printf("    <output_file>  The output file where the C code will be generated.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
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
    
    FILE *cFile = fopen(temporaryC, "w");
    
    if (cFile == NULL) {
        fprintf(stderr, "Cannot create temporary file: %s\n", temporaryC);
        fclose(mlFile);
        return EXIT_FAILURE;
    }
    
    // translate .ml to C
    int translateResult = parseML(mlFile, cFile);
    printf("Translation Result: %i\n", translateResult);  // print translation result (0 for success, non-zero for failure)

    fclose(mlFile); // closing ml file
    fclose(cFile); // closing temporary c file

    // compile C file to executable
    char cExecutable[EXEFILENAME_SIZE];

    snprintf(cExecutable, EXEFILENAME_SIZE, "ml-%d", getpid()); // using spnrintf to prevent potential buffer overflows

    if (!compileProgram(temporaryC, cExecutable)) {
        return EXIT_FAILURE;
    }

    // execute program
    int executeResult = executeProgram(cExecutable, argc - 2, argv + 2);

    // clean up temporary
    remove(cExecutable);
    remove(temporaryC);

    return executeResult;
}

// function to execute the program
int executeProgram(const char *exeName, int argc, char *argv[]) {
    char command[COMMAND_SIZE];
    // initializing command buffer with the filename
    snprintf(command, sizeof(command), "./%s", exeName);

    // append each argument to the command string
    for (int i = 0; i < argc; i++) {
        // safely append a space followed by the argument
        strncat(command, " ", sizeof(command) - strlen(command) - 1);
        strncat(command, argv[i], sizeof(command) - strlen(command) - 1);
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

// function to compile the executable
int compileProgram(const char *programName, const char *exeName) {
    char command[COMMAND_SIZE];
    
    // construct the gcc command
    snprintf(command, sizeof(command), "gcc -std=c11 %s -o %s", programName, exeName);

    // execute the gcc command
    int status = system(command);
    
    // check if the compilation worked
    if (status != 0) {
        fprintf(stderr, "Error: Compilation of '%s' failed\n", programName);
        return 0; // return 0 for failure
    }

    return 1; // return 1 for success
}

// function to remove trailing white spaces
void trimTrailingWhitespace(char *str) {
    int len = strlen(str);
    while (len > 0 && isspace(str[len - 1])) {
        str[--len] = '\0';
    }
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
bool validateSyntax(FILE *mlFile) {
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

        // skip comments
        if (line[0] == '#') {
            continue; // comment line, skip
        }

        // trim trailing spaces
        trimTrailingWhitespace(line);

        // check for valid identifiers (1-12 lowercase characters)
        char first_word[MAX_ID_LENGTH + 1];
        sscanf(line, "%s", first_word);  // get the first word on the line

        if (!isValidID(first_word)) {
            fprintf(stderr, "Invalid identifier: %s\n", first_word);
            return false;
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
                return false;
            }
            strcpy(identifiers[identifierCount], first_word);
            identifierCount++;
        }

        // ensure statements are one per line and do not end with a semicolon
        int len = strlen(line);
        if (len > 0 && line[len - 1] == ';') {
            fprintf(stderr, "Error: Statements should not have a terminating semicolon\n");
            return false;
        }
    }

    return true;
}

// function to check float numbers in expression
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

// function to check integers in expression
int isInteger(const char *str) {
    while (*str) {
        if (!isdigit(*str)) {
            return 0; // not integer
        }
        str++;
    }
    return 1; // is integer
}

// function to check variables in expression
bool isVariable(const char *expression, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int varCount) {
    // simple check for the expression if it contains variables
    for (int i = 0; i < varCount; i++) {
        if (strcmp(variables[i], expression) == 0) {
            return true;
        }
    }
    return false;
}

// function to check function calls in expression
bool isFunctionCall(const char *expression) {
    // a simple check to see if the expression contains a function call like `function_name(args)`
    return (strchr(expression, '(') && strchr(expression, ')'));
}

// function to translate .ml functions to c functions.
void translate_function(char fName[3][MAX_ID_LENGTH], char *fParam[], char *fBody[], char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int *varCount, FILE *cFile) {
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
            bool isVar = false;
            for (int i = 0; i < (*varCount); i++) {
                if (strcmp(variables[i], expression) == 0) {
                    isVar = true;
                }
            }

            // print based on type checking
            if (isVar || isInteger(expression)) {
                fprintf(cFile, "    printf(\"%s\", %s);\n", isInteger(expression) ? "%d" : "%f", expression);
            } else {
                fprintf(stderr, "Unknown variable or invalid expression in print statement: %s\n", expression);
            }
        } 
        else if (sscanf(fBody[ch], "%s <- %[^\n]", varName, expression) == 2) {
            // Check if the variable has already been declared
            bool alreadyDeclared = false;
            for (int i = 0; i < *varCount; i++) {
                if (strcmp(variables[i], varName) == 0) {
                    alreadyDeclared = true;
                }
            }

            if (!alreadyDeclared) {
                // Declare the variable if it's not already declared
                fprintf(cFile, "    float %s;\n", varName);
                strcpy(variables[*varCount], varName);
                (*varCount)++;
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

// function to parse and translate .ml file to c file.
int parseML(FILE *mlFile, FILE *cFile) {
    char line[MAX_LINE_LENGTH];
    char fName[3][MAX_ID_LENGTH];    // array to store function name, number of parameters, and number of lines
    char *fParam[MAX_IDENTIFIERS] = {0};   // array to hold function parameters
    char *fBody[MAX_IDENTIFIERS] = {0};    // array to hold function body lines
    int bodyLineCount = 0;           // tracks the number of lines in the function body
    char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH]; // array to store variable names
    int varCount = 0;               // counter for the number of variables
    char varName[MAX_ID_LENGTH];   // buffer to hold the variable name
    char value[MAX_ID_LENGTH];      // buffer to hold the value (not used in current code)
    char expression[MAX_LINE_LENGTH]; // buffer to hold expressions

    // Write the main function header
    fprintf(cFile, "#include <stdio.h>\n\n");
    fprintf(cFile, "int main() {\n");

    while (fgets(line, sizeof(line), mlFile) != NULL) {
        if (line[0] == '#' || line[0] == '\n') {
            // skip comments and empty lines
            continue;
        } else if (strncmp(line, "function ", 9) == 0) {
            int i = 9;  // start reading function name after "function "
            int fNameIndex = 0;

            // parse function name
            while (line[i] != ' ' && line[i] != '\n' && fNameIndex < MAX_ID_LENGTH - 1) {
                fName[0][fNameIndex++] = line[i++];
            }
            fName[0][fNameIndex] = '\0';

            // skip spaces between function name and parameters
            while (line[i] == ' ') i++;

            // parse function parameters
            int paramCount = 0;
            while (line[i] != '\n' && line[i] != '\0' && paramCount < MAX_IDENTIFIERS) {
                while (line[i] == ' ') i++; // skip extra spaces between parameters
                if (isalpha(line[i])) {
                    fParam[paramCount] = malloc(MAX_ID_LENGTH);
                    if (fParam[paramCount] == NULL) {
                        perror("Failed to allocate memory");
                        exit(EXIT_FAILURE);
                    }
                    int paramIndex = 0;
                    while (isalpha(line[i]) && paramIndex < MAX_ID_LENGTH - 1) {
                        fParam[paramCount][paramIndex++] = line[i++];
                    }
                    fParam[paramCount][paramIndex] = '\0';
                    paramCount++;
                } else {
                    i++;
                }
            }
            sprintf(fName[1], "%d", paramCount);  // store number of parameters

            // parse function body (assume body starts after function declaration)
            while (fgets(line, sizeof(line), mlFile) != NULL && line[0] != '\n' && line[0] != '#') {
                if (bodyLineCount >= MAX_IDENTIFIERS) {
                    fprintf(stderr, "Error: Exceeded maximum number of function body lines\n");
                    return EXIT_FAILURE;
                }
                fBody[bodyLineCount] = malloc(MAX_LINE_LENGTH);
                if (fBody[bodyLineCount] == NULL) {
                    perror("Failed to allocate memory");
                    exit(EXIT_FAILURE);
                }
                strcpy(fBody[bodyLineCount++], line);
            }
            sprintf(fName[2], "%d", bodyLineCount);  // store number of lines in the body

            // call translate_function after parsing each function
            translate_function(fName, fParam, fBody, variables, &varCount, cFile);

            // free allocated memory for parameters and body lines
            for (int j = 0; j < paramCount; j++) {
                free(fParam[j]);
            }
            for (int j = 0; j < bodyLineCount; j++) {
                free(fBody[j]);
            }

            // reset body line count for the next function
            bodyLineCount = 0;
        } else if (sscanf(line, "print %[^\n]", expression) == 1) {
            // check if the expression contains function calls or complex operations
            bool is_var = isVariable(expression, variables, varCount);
            bool is_int = isInteger(expression);
            bool is_func_call = isFunctionCall(expression);
            bool is_flo = isFloat(expression);
        
            // handle the expression (either variable, integer, function call, or float)
            if (is_var || is_int || is_func_call || is_flo) {
                // print as float (can be adjusted to handle different types if needed)
                fprintf(cFile, "printf(\"%%f\", (float)(%s));\n", expression);
            } else {
                fprintf(stderr, "Unknown variable or invalid expression in print statement: %s\n", expression);
            }
        } else if (sscanf(line, "%s <- %[^\n]", varName, expression) == 2) {
            // store the variable name if not already present
            bool already_declared = false;
            for (int i = 0; i < varCount; i++) {
                if (strcmp(variables[i], varName) == 0) {
                    already_declared = true;
                }
            }
            if (!already_declared) {
                // declare the variable
                fprintf(cFile, "float %s;\n", varName);
                strcpy(variables[varCount], varName);
                (varCount)++;
            }
            // write the full expression to the C file
            fprintf(cFile, "%s = %s;\n", varName, expression);
        }
    }
    
    // Close the main function
    fprintf(cFile, "    return 0;\n");
    fprintf(cFile, "}\n");

    return 0;
}
