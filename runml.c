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
#define MAX_ID_LENGTH 12
#define MAX_LINE_LENGTH 256
#define MAX_IDENTIFIERS 50

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

int translate_function(char **fName, char **fParam, char **fBody, FILE *cFile) {
	char *rType = "";
	bool foundReturn = false;
	int param = 0;
	int numParam = atoi(fName[1]);
	int line = 0;
	int numLines = atoi(fName[2]);
	char parameters[50] = "";

	while (param+1 < numParam) {
		strcat(parameters, "float ");
		strcat(parameters, fParam[param]);
		strcat(parameters, ", ");
		param++;
	}

	if (numParam != 0) {
		strcat(parameters, "float ");
		strcat(parameters, fParam[param]);
	}
	else {
		strcat(parameters, "void");
	}


	while (line < numLines) {
		if (strstr(fBody[line], "\treturn") != NULL) {
			rType = "float";
			foundReturn = true;
			break;
		}
		line++;
	}

	if (!foundReturn) {
		rType = "void";
	}

	fprintf(cFile, "%s %s(%s) {\n", rType, fName[0], parameters);

	line = 0;
	while (line < numLines) {
		if (fBody[line] != '\t') {
            fprintf(stderr, "! Syntax error on line %d. Lines in the function body must be indented.\n", line);
        }
		else if (strstr(fBody[line], "\treturn ") != NULL) {
            char expression[MAX_LINE_LENGTH];
            int i = 7;
            while (fBody[line][i] != '\n') {
                expression[i-7] = fBody[line][i];
                i++;
            }
            expression[i-7] = '\0';
            fprintf(cFile, "\treturn %s;\n", expression);
            //need to implement variable checking for expressions
        }
        else if (strstr(fBody[line], "\tprint ") != NULL) {
            char expression[MAX_LINE_LENGTH];
            int i = 6;
            while (fBody[line][i] != '\n') {
                expression[i-6] = fBody[line][i];
                i++;
            }
            expression[i-6] = '\0';
            //add rounding check if ends in .0 print to .0 places
            fprintf(cFile, "\tprintf(\"%%.6f\\n\", %s);\n", expression);
        }
        else if (strstr(fBody[line], "<-") != NULL) {
            char identifier[MAX_ID_LENGTH];
            char expression[MAX_LINE_LENGTH];
            int i = 0;

            while (fBody[line][i] != '<') {
                identifier[i] = fBody[line][i];
                i++;
            }
            identifier[i] = '\0';

            i += 3;
            int j = 0;
            while (fBody[line][i] != '\n') {
                expression[j] = fBody[line][i];
                i++;
                j++;
            }
            expression[j] = '\0';

            fprintf(cFile, "\tfloat %s = %s;\n", identifier, expression);
            //add variable name to array for variable checking
        }
        else {
            fprintf(stderr, "! Syntax error on line: %d\n", line);
            exit(EXIT_FAILURE);
        }
        line++;
    }
    fprintf(cFile, "}\n");
    exit(EXIT_SUCCESS);
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

        // check for comments
        if (line[0] == '#') {
            continue; // comment line, skip
        }

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

        // ensure statements are one per line (no semicolon at the end)
        if (line[strlen(line) - 1] == ';') {
            fprintf(stderr, "Error: Statements should not have a terminating semicolon\n");
            return false;
        }
    }

    return true;
}