#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_IDENTIFIERS 50
#define MAX_IDENTIFIER_LENGTH 12
#define MAX_LINE_LENGTH 256

// function to check if a string is a valid identifier
bool isValidID(const char *str) {
    int len = strlen(str);
    if (len < 1 || len > MAX_IDENTIFIER_LENGTH) return false;

    // check if characters are lowercase alphabetic
    for (int i = 0; i < len; i++) {
        if (!islower(str[i])) return false;
    }
    return true;
}

// function to trim trailing whitespace
void trimTrailingWhitespace(char *str) {
    int len = strlen(str);
    while (len > 0 && isspace(str[len - 1])) {
        str[--len] = '\0';
    }
}

// function to validate .ml file
bool validateSyntax(FILE *mlFile) {
    char line[MAX_LINE_LENGTH];
    int identifierCount = 0;
    char identifiers[MAX_IDENTIFIERS][MAX_IDENTIFIER_LENGTH + 1]; // store up to 50 unique identifiers

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
        char first_word[MAX_IDENTIFIER_LENGTH + 1];
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

        // check for proper indentation (inside function bodies)
        if (strchr(line, '\t') != NULL) {
            if (line[0] != '\t') {
                fprintf(stderr, "Error: Expected tab indentation inside function body\n");
                return false;
            }
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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.ml>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Open the input .ml file
    FILE *mlFile = fopen(argv[1], "r");
    if (!mlFile) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Call the validateSyntax function to check the .ml file
    if (validateSyntax(mlFile)) {
        printf("The syntax is valid.\n");
    } else {
        printf("Syntax errors were found.\n");
    }

    // Close the file
    fclose(mlFile);

    return 0;
}