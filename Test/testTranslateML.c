#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_ID_LENGTH 12
#define MAX_LINE_LENGTH 256
#define MAX_IDENTIFIERS 50

// Function prototypes
int parseML(FILE *mlFile, FILE *cFile);
void translate_function(char fName[3][MAX_ID_LENGTH], char *fParam[], char *fBody[], FILE *cFile);
int is_integer(const char *str);

int parseML(FILE *mlFile, FILE *cFile) {
    char line[MAX_LINE_LENGTH];
    char fName[3][MAX_ID_LENGTH];    // to store function name, number of parameters, and number of lines
    char *fParam[MAX_IDENTIFIERS] = {0};   // array to hold function parameters
    char *fBody[MAX_IDENTIFIERS] = {0};    // array to hold function body lines
    int bodyLineCount = 0;           // tracks the number of body lines

    while (fgets(line, sizeof(line), mlFile) != NULL) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;  // skip comments and empty lines
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
                while (line[i] == ' ') i++; // skipping any extra spaces between parameters
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
            sprintf(fName[2], "%d", bodyLineCount);  // store number of lines in body

            // call translate_function after parsing each function
            translate_function(fName, fParam, fBody, cFile);

            // free allocated memory for parameters and body lines
            for (int j = 0; j < paramCount; j++) {
                free(fParam[j]);
            }
            for (int j = 0; j < bodyLineCount; j++) {
                free(fBody[j]);
            }

            // reset body line count for next function
            bodyLineCount = 0;
        }
    }
    return 0;
}

void translate_function(char fName[3][MAX_ID_LENGTH], char *fParam[], char *fBody[], FILE *cFile) {
    int ch = 0;
    char *r_type = ""; // return type
    bool foundReturn = false; // flag to track if return was found

    // determine return type based on the function body
    while (fBody[ch] != NULL && fBody[ch][0] != '\0') {
        if (strncmp(fBody[ch], "\treturn", 7) == 0) {
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
    fprintf(cFile, "%s %s(%s) {\n", r_type, fName[0], fName[1]);

    // parse body of function
    while (fBody[ch] != NULL && fBody[ch][0] != '\0') {
        if (strncmp(fBody[ch], "\treturn", 7) == 0) {
            char expression[128];
            int i = 7;  // offset for the start of the return expression

            // extract the return expression
            int exprIndex = 0;
            while (fBody[ch][i] != '\n' && fBody[ch][i] != '\0' && exprIndex < sizeof(expression) - 1) {
                expression[exprIndex++] = fBody[ch][i++];
            }
            expression[exprIndex] = '\0';  // null-terminate the expression

            // write the return statement to the C file
            fprintf(cFile, "    return %s;\n", expression);
        } 
        else if (strncmp(fBody[ch], "\tprint", 6) == 0) {
            char expression[128];
            int i = 6;  // offset for the start of the print expression

            // extract the print expression
            int exprIndex = 0;
            while (fBody[ch][i] != '\n' && fBody[ch][i] != '\0' && exprIndex < sizeof(expression) - 1) {
                expression[exprIndex++] = fBody[ch][i++];
            }
            expression[exprIndex] = '\0';  // null-terminate the expression

            // printf based on type checking
            if (is_integer(expression)) {
                fprintf(cFile, "    printf(\"%%d\", %s);\n", expression);
            } else {
                fprintf(cFile, "    printf(\"%%f\", %s);\n", expression);
            }
        } 
        else if (fBody[ch][0] == '<' && fBody[ch][1] == '-') {
            // handle variable assignment, assuming the format is <var> <- <value>;
            char var_name[50];
            char value[50];
            if (sscanf(fBody[ch], "%49s <- %49s", var_name, value) == 2) {
                // write the variable declaration and assignment to the C file
                fprintf(cFile, "    float %s = %s;\n", var_name, value);
            } else {
                fprintf(stderr, "Syntax error in variable assignment: %s\n", fBody[ch]);
            }
        } 
        else {
            // handle syntax errors
            fprintf(stderr, "Syntax error: Unexpected token in function body: %s\n", fBody[ch]);
            return;
        }
        ch++;
    }

    // close the function in the C file
    fprintf(cFile, "}\n");
}

int is_integer(const char *str) {
    while (*str) {
        if (!isdigit(*str)) {
            return 0; // not an integer
        }
        str++;
    }
    return 1; // is an integer
}

// Translates variable assignment and manages variable declarations
void translate_variable_assignment(const char *line, FILE *cFile, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int *var_count) {
    char var_name[MAX_ID_LENGTH];
    char value[MAX_ID_LENGTH];
    
    if (sscanf(line, "%s <- %s", var_name, value) == 2) {
        // Store the variable name if not already present
        bool already_declared = false;
        for (int i = 0; i < *var_count; i++) {
            if (strcmp(variables[i], var_name) == 0) {
                already_declared = true;
                break;
            }
        }
        if (!already_declared) {
            // Declare the variable
            fprintf(cFile, "    float %s;\n", var_name);
            strcpy(variables[*var_count], var_name);
            (*var_count)++;
        }
        // Initialize the variable
        fprintf(cFile, "    %s = %s;\n", var_name, value);
    } else {
        fprintf(stderr, "Syntax error in variable assignment: %s\n", line);
    }
}

// Translates print statement and handles variable usage
void translate_print_statement(const char *line, FILE *cFile, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int var_count) {
    char expression[MAX_LINE_LENGTH];
    
    if (sscanf(line, "print %s", expression) == 1) {
        // Check if expression is a known variable
        bool is_var = false;
        for (int i = 0; i < var_count; i++) {
            if (strcmp(variables[i], expression) == 0) {
                is_var = true;
                break;
            }
        }

        if (is_var || is_integer(expression)) {
            fprintf(cFile, "    printf(\"%s\", %s);\n", is_integer(expression) ? "%d" : "%f", expression);
        } else {
            fprintf(stderr, "Unknown variable or invalid expression in print statement: %s\n", expression);
        }
    } else {
        fprintf(stderr, "Syntax error in print statement: %s\n", line);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_ml_file> <output_c_file>\n", argv[0]);
        return 1;
    }

    FILE *mlFile = fopen(argv[1], "r");
    if (mlFile == NULL) {
        perror("Error opening input file");
        return 1;
    }

    FILE *cFile = fopen(argv[2], "w");
    if (cFile == NULL) {
        perror("Error opening output file");
        fclose(mlFile);
        return 1;
    }

    int result = parseML(mlFile, cFile);

    fclose(mlFile);
    fclose(cFile);

    return result;
}
