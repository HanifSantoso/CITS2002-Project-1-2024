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
void translate_function(char fName[3][MAX_ID_LENGTH], char *fParam[], char *fBody[], char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int *var_count, FILE *cFile);
int is_integer(const char *str);
void translate_variable_assignment(const char *line, FILE *cFile, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int *var_count);
void translate_print_statement(const char *line, FILE *cFile, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int var_count);
bool is_variable(const char *expression, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int var_count);
bool is_function_call(const char *expression);
bool is_float(const char *str);

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
    fprintf(cFile, "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n");
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
            bool is_var = is_variable(expression, variables, varCount);
            bool is_int = is_integer(expression);
            bool is_func_call = is_function_call(expression);
            bool is_flo = is_float(expression);
        
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

void translate_function(char fName[3][MAX_ID_LENGTH], char *fParam[], char *fBody[], char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int *var_count, FILE *cFile) {
    int ch = 0;
    char *r_type = ""; // return type
    bool foundReturn = false; // flag to track if return was found
    char var_name[MAX_ID_LENGTH];
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

    int paramCount = atoi(fName[1]);  // Convert fName[1] to an integer (number of parameters)

    // Safeguard: Ensure paramCount does not exceed the size of fParam
    if (paramCount < 0 || paramCount > MAX_IDENTIFIERS) {
        fprintf(stderr, "Invalid number of parameters: %d\n", paramCount);
        return;  // Exit if parameter count is invalid
    }

    // Safeguard: Ensure paramList has enough space
    for (int i = 0; i < paramCount; i++) {
        // Check if the current parameter is valid (not NULL)
        if (fParam[i] == NULL) {
            fprintf(stderr, "Invalid parameter at index %d\n", i);
            return;
        }
    
        // Ensure there's enough space in paramList to concatenate the parameter and a comma
        if (strlen(paramList) + strlen(fParam[i]) + 3 > MAX_LINE_LENGTH) {  // +3 for ", " and null-terminator
            fprintf(stderr, "Parameter list too long to fit in buffer\n");
            return;
        }
    
        strcat(paramList, fParam[i]);  // Concatenate the parameter name
    
        if (i < paramCount - 1) {
            strcat(paramList, ", ");  // Add a comma and space between parameters, except after the last one
        }
    }

    // Now use paramList in the function declaration
    fprintf(cFile, "function %s(%s) {\n", fName[0], paramList);  // Use fName[0] (the function name) and paramList

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
        else if (sscanf(fBody[ch], "%s <- %[^\n]", var_name, expression) == 2) {
            // Check if the variable has already been declared
            bool already_declared = false;
            for (int i = 0; i < *var_count; i++) {
                if (strcmp(variables[i], var_name) == 0) {
                    already_declared = true;
                }
            }

            if (!already_declared) {
                // Declare the variable if it's not already declared
                fprintf(cFile, "    float %s;\n", var_name);
                strcpy(variables[*var_count], var_name);
                (*var_count)++;
            }

            // Output the full expression to the C file
            fprintf(cFile, "    %s = %s;\n", var_name, expression);
        } else {
            fprintf(stderr, "Syntax error in variable assignment: %s\n", fBody[ch]);
        }
        ch++;
    }

    // close the function in the C file
    fprintf(cFile, "}\n");
}

bool is_float(const char *str) {
    bool dot_found = false;

    // Skip leading whitespace
    while (isspace(*str)) {
        str++;
    }

    // Check if the string starts with a digit or a dot
    if (!isdigit(*str) && *str != '.') {
        return false; // Must start with a digit or dot
    }

    // Check the rest of the string
    while (*str) {
        if (*str == '.') {
            if (dot_found) {
                return false; // Multiple dots are not allowed
            }
            dot_found = true;
        } else if (!isdigit(*str)) {
            return false; // Invalid character for a float
        }
        str++;
    }

    return dot_found; // Must contain at least one dot to be a valid float
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

bool is_variable(const char *expression, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int var_count) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i], expression) == 0) {
            return true;
        }
    }
    return false;
}

bool is_function_call(const char *expression) {
    // A simple check to see if the expression contains a function call like `function_name(args)`
    return (strchr(expression, '(') && strchr(expression, ')'));
}

// Translates variable assignment and manages variable declarations
void translate_variable_assignment(const char *line, FILE *cFile, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int *var_count) {
    char var_name[MAX_ID_LENGTH];
    char expression[MAX_LINE_LENGTH];
    
    // Use sscanf to parse the variable name and the rest of the line as the expression
    if (sscanf(line, "%s <- %[^\n]", var_name, expression) == 2) {
        // Check if the variable has already been declared
        bool already_declared = false;
        for (int i = 0; i < *var_count; i++) {
            if (strcmp(variables[i], var_name) == 0) {
                already_declared = true;
            }
        }

        if (!already_declared) {
            // Declare the variable if it's not already declared
            fprintf(cFile, "    float %s;\n", var_name);
            strcpy(variables[*var_count], var_name);
            (*var_count)++;
        }

        // Output the full expression to the C file
        fprintf(cFile, "    %s = %s;\n", var_name, expression);
    } else {
        fprintf(stderr, "Syntax error in variable assignment: %s\n", line);
    }
}

// Translates print statement and handles variable usage
void translate_print_statement(const char *line, FILE *cFile, char variables[MAX_IDENTIFIERS][MAX_ID_LENGTH], int var_count) {
    char expression[MAX_LINE_LENGTH];
    
    // Extract the print expression (everything after "print ")
    if (sscanf(line, "print %[^\n]", expression) == 1) {
        // Check if the expression contains function calls or complex operations
        bool is_var = is_variable(expression, variables, var_count);
        bool is_int = is_integer(expression);
        bool is_func_call = is_function_call(expression);
        
        // Handle the expression (either variable, integer, or function call)
        if (is_var || is_int || is_func_call) {
            // Print as float (you can adjust this to handle different types if needed)
            fprintf(cFile, "    printf(\"%%f\", (float)(%s));\n", expression);
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
