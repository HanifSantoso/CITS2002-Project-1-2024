//  CITS2002 Project 1 2024
//  Student1:   23751927   Jenna Milford-1
//  Student2:   23970785   Hanif Santoso-2
//  Platform:   Linux

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define CFILENAME_SIZE 128
#define EXEFILENAME_SIZE 128
#define COMMANDLINE_SIZE 256
#define FUNCVAR_SIZE 50

bool validateFile(FILE *mlFile);

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
    char line[COMMANDLINE_SIZE];
    while (fgets(line, sizeof(COMMANDLINE_SIZE), mlFile) != NULL) {
        // // put function somewhere here
    }

    // generate temporary C file
    char temporaryC[CFILENAME_SIZE];
        snprintf(temporaryC, CFILENAME_SIZE, "ml-%d.c", getpid()); // snprintf used to prevent buffer overflows

    // translate .ml to C
    if (!translateFunction(mlFile, temporaryC)) {
        return EXIT_FAILURE;
    }

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

bool validateFile(FILE *mlFile) {
    char line[COMMANDLINE_SIZE];
    char *functions[FUNCVAR_SIZE];
    char *variables[FUNCVAR_SIZE];
    while (fgets(line, sizeof(COMMANDLINE_SIZE), mlFile) != NULL) {
        // something cool
    }
    rewind(mlFile);
    return true;
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

int translateFunction(char **function, FILE *cFile) {
    int ch = 0;
    char *r_type = ""; // return type
    bool foundReturn = false; // flag to track if return was found

    // determine return type based on the function body
    while (function[2][ch] != '\0') {
        if (strncmp(&function[2][ch], "\treturn", 7) == 0) {
            r_type = "float";  // assuming all functions that return use float
            foundReturn = true;
            break;
        }
        ch++;
    }

    if (!foundReturn) {
        r_type = "void";  // default to void if no return statement is found
    }

    // reset ch to process the body
    ch = 0;

    // write function signature to the C file
    fprintf(cFile, "%s %s(%s) {\n", r_type, function[0], function[1]);

    // parse body of function
    while (function[2][ch] != '\0') {
        if (strncmp(&function[2][ch], "\treturn", 7) == 0) {
            char expression[128];
            int i = 7;  // offset for the start of the return expression

            // extract the return expression
            while (function[2][ch + i] != '\n' && function[2][ch + i] != '\0') {
                expression[i - 7] = function[2][ch + i];
                i++;
            }
            expression[i - 7] = '\0';  // null-terminate the expression

            // write the return statement to the C file
            fprintf(cFile, "    return %s;\n", expression);
        } 
        else if (strncmp(&function[2][ch], "\tprint", 6) == 0) {
            char expression[128];
            int i = 6;  // offset for the start of the print expression

            // extract the print expression
            while (function[2][ch + i] != '\n' && function[2][ch + i] != '\0') {
                expression[i - 6] = function[2][ch + i];
                i++;
            }
            expression[i - 6] = '\0';  // null-terminate the expression

            // printf based on type checking
            if (is_integer(expression)) { // implement a helper function for this
                fprintf(cFile, "    printf(\"%%d\", %s);\n", expression);
            } else {
                fprintf(cFile, "    printf(\"%%f\", %s);\n", expression);
            }
        } 
        else if (function[2][ch] == '<' && function[2][ch + 1] == '-') {
            // handle variable assignment, assuming the format is <var> <- <value>;
            char var_name[FUNCVAR_SIZE];
            char value[FUNCVAR_SIZE];
            sscanf(&function[2][ch], "%s <- %s", var_name, value);

            // write the variable declaration and assignment to the C file
            fprintf(cFile, "    float %s = %s;\n", var_name, value);

            // store variable name for future use or checking
            // store in an array for further processing if needed
        } 
        else {
            // handle syntax errors
            fprintf(stderr, "Syntax error: Unexpected token in function body: %s\n", &function[2][ch]);
            return 0;
        }
        ch++;
    }

    // close the function in the C file
    fprintf(cFile, "}\n");
    return 1;
}