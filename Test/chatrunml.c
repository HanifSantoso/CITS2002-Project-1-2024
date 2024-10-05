#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function prototypes
void print_usage(const char *prog_name);
int validate_ml_program(FILE *ml_file);
void translate_ml_to_c(FILE *ml_file, const char *c_filename);
int compile_c_program(const char *c_filename, const char *output_filename);
int execute_program(const char *output_filename, int argc, char *argv[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Open the .ml file
    FILE *ml_file = fopen(argv[1], "r");
    if (ml_file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Validate the .ml program
    if (!validate_ml_program(ml_file)) {
        fclose(ml_file);
        return EXIT_FAILURE;
    }

    // Generate a temporary C file name based on the process ID
    char c_filename[64];
    snprintf(c_filename, sizeof(c_filename), "ml-%d.c", getpid());

    // Translate .ml to C
    translate_ml_to_c(ml_file, c_filename);

    fclose(ml_file); // Close the .ml file

    // Compile the C file to an executable
    char output_filename[64];
    snprintf(output_filename, sizeof(output_filename), "ml-%d", getpid());
    if (!compile_c_program(c_filename, output_filename)) {
        return EXIT_FAILURE;
    }

    // Execute the compiled program
    int result = execute_program(output_filename, argc - 2, argv + 2);

    // Clean up temporary files
    remove(c_filename);
    remove(output_filename);

    return result;
}

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s <file.ml> [args...]\n", prog_name);
}

int validate_ml_program(FILE *ml_file) {
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), ml_file)) {
        line_num++;
        // Example: Check for some basic syntax issues (e.g., no tab character for indentation)
        if (line[0] != '\t' && line[0] != '\n' && line[0] != '#') {
            fprintf(stderr, "! Syntax error at line %d\n", line_num);
            return 0;
        }
    }

    // Rewind the file pointer to the start of the file
    rewind(ml_file);
    return 1;
}

void translate_ml_to_c(FILE *ml_file, const char *c_filename) {
    FILE *c_file = fopen(c_filename, "w");
    if (c_file == NULL) {
        fprintf(stderr, "Error: Could not create file %s\n", c_filename);
        exit(EXIT_FAILURE);
    }

    fprintf(c_file, "#include <stdio.h>\n\n");
    fprintf(c_file, "int main(int argc, char *argv[]) {\n");

    char line[256];
    while (fgets(line, sizeof(line), ml_file)) {
        if (line[0] == '\t') {
            fprintf(c_file, "%s", line + 1); // Translate .ml code to C (simplified)
        }
    }

    fprintf(c_file, "    return 0;\n");
    fprintf(c_file, "}\n");

    fclose(c_file);
}

int compile_c_program(const char *c_filename, const char *output_filename) {
    char command[256];
    snprintf(command, sizeof(command), "gcc -std=c11 %s -o %s", c_filename, output_filename);
    int status = system(command);
    if (status != 0) {
        fprintf(stderr, "Error: Compilation failed\n");
        return 0;
    }
    return 1;
}

int execute_program(const char *output_filename, int argc, char *argv[]) {
    char command[256];
    snprintf(command, sizeof(command), "./%s", output_filename);
    for (int i = 0; i < argc; i++) {
        strcat(command, " ");
        strcat(command, argv[i]);
    }

    int status = system(command);
    if (status != 0) {
        fprintf(stderr, "Error: Execution failed\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
