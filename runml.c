//  CITS2002 Project 1 2024
//  Student1:   23751927   Jenna Milford-1
//  Student2:   23970785   Hanif Santoso-2
//  Platform:   Linux

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool validateFile(FILE *mlFile);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    // Open the .ml file
    FILE *mlFile = fopen(filepath, "r");
    if (mlFile == NULL) {
        fprintf(stderr, "Cannot find filepath:%s\n", argv1);
        exit(EXIT_FAILURE);
    }
    // Validate the .ml program
    char line[256];
    while (fgets(line, sizeof(line), mlFile) != NULL) {
        
    }

    // Generate temporary C file
    // Translate .ml to C
    // Compile C file to executable
    // Execute
    // Clean up temporary
}

bool validateFile(FILE *mlFile) {
    char line[256];
    while (fgets(line, sizeof(line), mlFile) != NULL) {
        // something cool
    }
    rewind(mlFile);
    return true;
}