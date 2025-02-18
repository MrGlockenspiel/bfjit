#pragma once

#include "jit.h"

program_t bf_parse_string(const char *str);
void bf_interpret(program_t program);
void bf_compile(program_t program);

void free_program(program_t program);

void dump_cells(u8 *cell_memory, u64 n);
void dump_to_file(const char *filename, const void *data, size_t size);
