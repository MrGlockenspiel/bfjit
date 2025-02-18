#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "jit.h"
#include "types.h"

char *load_file(const char *filename) {
    FILE *fp;
    char *buffer;

    fp = fopen(filename, "rb");
    if (!fp) {
        printf("failed to load file '%s'\n", filename);
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    u64 size = ftell(fp);
    rewind(fp);

    buffer = malloc(size + 1);
    if (!buffer) {
        printf("failed to allocate memory for file\n");
        fclose(fp);
        exit(1);
    }

    if (fread(buffer, size, 1, fp) != 1) {
        printf("failed to read file\n");
        fclose(fp);
        free(buffer);
        exit(1);
    }

    // null termination
    buffer[size] = 0;

    fclose(fp);
    return buffer;
}

i32 main(i32 argc, char *argv[]) {
    u8 interpreted = 0;

    if (argc < 2) {
        printf("Usage: \n\t%s [-i / --interpreted] 'file.bf'\n\n", argv[0]);
        exit(1);
    } else if (argc > 2) {
        if (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--interpreted")) {
            interpreted = 1;
            printf("Running in interpreted mode\n");
        } else {
            printf("Invalid argument '%s'\n", argv[1]);
            printf("Usage: \n\t%s [-i / --interpreted] 'file.bf'\n\n", argv[0]);
            exit(1);
        }
    }

    char *bf_code = load_file(argv[argc - 1]);

    program_t program = bf_parse_string(bf_code);

    if (interpreted) {
        bf_interpret(program);
    } else {
        bf_compile(program);
    }

    free_program(program);
    free(bf_code);

    return 0;
}
