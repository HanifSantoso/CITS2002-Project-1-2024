//  CITS2002 Project 1 2024
//  Student1:   23751927   Jenna Milford-1
//  Student2:   23970785   Hanif Santoso-2
//  Platform:   Linux

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define CFILENAME 128
#define EXEFILENAME 128
#define COMMANDLINE 256
#define FUNCVAR 50
#define MAX_ID_LENGTH 12
#define MAX_LINE_LENGTH 256

bool validateFile(FILE *mlFile);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    // Open the .ml file
    FILE *mlFile = fopen(argv[1], "r");
    if (mlFile == NULL) {
        fprintf(stderr, "Cannot find filepath:%s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    // Validate the .ml program
    char line[COMMANDLINE];
    while (fgets(line, sizeof(line), mlFile) != NULL) {
        // // put function somewhere here
    }

    // Generate temporary C file
    char temporaryC[CFILENAME];
        sprintf(temporaryC, "ml-%d.c", getpid()); // sprintf assumed to be sufficient for buffer size

    // Translate .ml to C
    // // Put function here

    fclose(mlFile); // closing ml file

    // Compile C file to executable
    char cExecutable[EXEFILENAME];

    sprintf(cExecutable, "ml-%d", getpid()); // using sprintf since the buffer size is assumed to be sufficient

    if (!compile_c_program(temporaryC, cExecutable)) {
        return EXIT_FAILURE;
    }

    // Execute
    // Clean up temporary
}

bool validateFile(FILE *mlFile) {
    char line[COMMANDLINE];
    char *functions[FUNCVAR];
    char *variables[FUNCVAR];
    while (fgets(line, sizeof(line), mlFile) != NULL) {
        // something cool
    }
    rewind(mlFile);
    return true;
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

int executeProgram(const char *exeName, int argc, char *argv[]) {
    char command[COMMANDLINE];
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
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int compileProgram(const char *programName, const char *exeName) {
    char command[COMMANDLINE];
    
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