#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

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
            char var_name[50];
            char value[50];
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


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ml_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *mlFile = fopen(argv[1], "r");
    if (mlFile == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    if (!validateSyntax(mlFile)) {
        fprintf(stderr, "Invalid ML file: %s\n", argv[1]);
        fclose(mlFile);
        return EXIT_FAILURE;
    }

    fclose(mlFile);
    printf("ML file is valid.\n");
    return EXIT_SUCCESS;
}
