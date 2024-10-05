#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

int executeProgram(const char *exeName, int argc, char *argv[]) {
    char command[256];
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
    char command[256];
    
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source file> <executable name> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *sourceFile = argv[1];
    const char *exeName = argv[2];

    // Compile the source file
    if (!compileProgram(sourceFile, exeName)) {
        return EXIT_FAILURE;
    }

    // If there are additional arguments, execute the compiled program
    if (argc > 3) {
        return executeProgram(exeName, argc - 3, &argv[3]);
    }

    return EXIT_SUCCESS;
}