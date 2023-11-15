#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "bf.h"
#include "jit.h"
#include "types.h"

program_t *bf_parse_string(const char *str) {
  program_t *program = malloc(sizeof(program_t));

  size_t program_size = strlen(str);

  opcode_t *bytecode = malloc(sizeof(opcode_t) * program_size);
  memset(bytecode, 0, sizeof(opcode_t) * program_size);

  u64 *stack = malloc(program_size);

  u32 bytecode_index = 0;
  u32 str_index = 0;
  u32 stack_index = 0;

  while (str_index < program_size) {
    char ins = str[str_index];

    if (ins == '[') {
      bytecode[bytecode_index].op = '[';
      bytecode[bytecode_index].count = 0;

      stack[stack_index++] = bytecode_index;

      bytecode_index++;
      str_index++;
    } else if (ins == ']') {
      u64 offset = stack[--stack_index];

      bytecode[offset].jmp_addr = bytecode_index;

      bytecode[bytecode_index].op = ']';
      bytecode[bytecode_index].jmp_addr = offset;

      bytecode_index++;
      str_index++;
    } else {
      u32 start = str_index++;

      while (str_index < program_size && str[str_index] == ins) {
        str_index++;
      }

      switch (ins) {
      case '>':
        bytecode[bytecode_index].op = '>';
        break;
      case '<':
        bytecode[bytecode_index].op = '<';
        break;
      case '+':
        bytecode[bytecode_index].op = '+';
        break;
      case '-':
        bytecode[bytecode_index].op = '-';
        break;
      case ',':
        bytecode[bytecode_index].op = ',';
        break;
      case '.':
        bytecode[bytecode_index].op = '.';
        break;
      default:
        break;
      }

      bytecode[bytecode_index].count = str_index - start;
      bytecode_index++;
    }
  }

  program->bytecode = bytecode;
  program->size = bytecode_index;

  free(stack);

  return program;
}

void bf_interpret(const program_t *program) {
  u8 *memory = malloc(30000);
  memset(memory, 0, 30000);

  u64 dataptr = 0;

  u64 index = 0;
  u64 program_size = program->size;

  while (index < program_size) {
    char op = program->bytecode[index].op;
    u32 count = program->bytecode[index].count;

    switch (op) {
    case '>':
      dataptr += count;
      break;
    case '<':
      dataptr -= count;
      break;
    case '+':
      memory[dataptr] += count;
      break;
    case '-':
      memory[dataptr] -= count;
      break;
    case '.':
      for (u64 j = 0; j < count; j++) {
        putchar(memory[dataptr]);
      }
      break;
    case ',':
      for (u64 j = 0; j < count; j++) {
        memory[dataptr] = getchar();
      }
      break;
    case '[':
      if (memory[dataptr] == 0) {
        index = count;
      }
      break;
    case ']':
      if (memory[dataptr] != 0) {
        index = count;
      }
      break;
    default:
      break;
    }
    index++;
  }
  free(memory);
  return;
}

void bf_compile(const program_t *program) {
  // size_t fn_memory_size = 4096 * 1000;

  // in some edge cases this may not be enough
  // program_size * 8 rounded to nearest 4KB
  size_t fn_memory_size = ((program->size * 8 + 4096) / 4096) * 4096;

  u8 *fn_memory =
      mmap(NULL,           // address
           fn_memory_size, // size
           PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS,
           -1, // fd (not used here)
           0); // offset (not used here)

  u64 fn_index = 0;
  u64 program_index = 0;
  u64 program_size = program->size;

  u64 *bracket_stack = malloc(program_size);
  u64 bracket_index = 0;

  // load argument 1 into r13
  asm_mov_r13_arg1(fn_memory, &fn_index);

  while (program_index < program_size) {
    opcode_t opcode = program->bytecode[program_index];

    switch (opcode.op) {
    case '>':
      if (opcode.count > 1) {
        asm_add_r13(fn_memory, &fn_index, opcode.count);
      } else {
        asm_inc_r13(fn_memory, &fn_index);
      }
      break;
    case '<':
      if (opcode.count > 1) {
        asm_sub_r13(fn_memory, &fn_index, opcode.count);
      } else {
        asm_dec_r13(fn_memory, &fn_index);
      }
      break;
    case '+':
      asm_add_ptr_r13(fn_memory, &fn_index, opcode.count);
      break;
    case '-':
      asm_sub_ptr_r13(fn_memory, &fn_index, opcode.count);
      break;
    case '.':
      for (u64 i = 0; i < opcode.count; i++) {
        asm_putchar(fn_memory, &fn_index);
      }
      break;
    case ',':
      for (u64 i = 0; i < opcode.count; i++) {
        asm_syscall_read(fn_memory, &fn_index);
      }
      break;
    case '[':
      asm_cmp_byte_r13(fn_memory, &fn_index, 0);
      asm_jz(fn_memory, &fn_index, 0);

      // push address to stack
      bracket_stack[bracket_index++] = fn_index;
      break;
    case ']':
      asm_cmp_byte_r13(fn_memory, &fn_index, 0);

      // pop [ address off stack
      u64 bracket_offset = bracket_stack[--bracket_index];

      u64 jump_back_from = fn_index + 6;
      u64 jump_back_to = bracket_offset;

      u32 rel_offset_back = (u32)(jump_back_to - jump_back_from);

      asm_jnz(fn_memory, &fn_index, rel_offset_back);

      // fix address of previous jz
      size_t jump_forward_from = bracket_offset;
      size_t jump_forward_to = fn_index;

      u32 rel_offset_forward = (u32)(jump_forward_to - jump_forward_from);

      fn_index = bracket_offset - sizeof(u32);
      push_u32(fn_memory, &fn_index, rel_offset_forward);
      fn_index = jump_forward_to;

      break;
    default:
      break;
    }
    program_index++;
  }

  asm_ret(fn_memory, &fn_index);

  // dump binary to file
  // dump_to_file("out.bin", fn_memory, fn_index);

  u8 *cell_memory = malloc(300000);
  memset(cell_memory, 0, 300000);

// ISO C doesnt allow this cast
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

  typedef void (*fn)(u8 *cells);
  fn bf_func = (fn)fn_memory;
  bf_func(cell_memory);

#pragma GCC diagnostic pop
  free(cell_memory);

  munmap(fn_memory, fn_memory_size);
  free(bracket_stack);

  return;
}

void free_program(program_t *program) {
  free(program->bytecode);
  free(program);
}

void dump_cells(u8 *cell_memory, u64 n) {
  printf("CELL DUMP: \n");
  for (u64 i = 0; i < n; i++) {
    printf("[%02X] ", cell_memory[i]);
    if (i % 16 == 15) {
      printf("\n");
    }
  }
}

void dump_to_file(const char *filename, const void *data, size_t size) {
  FILE *file = fopen(filename, "wb");

  if (file == NULL) {
    perror("Error opening file");
    return;
  }

  size_t elements_written = fwrite(data, 1, size, file);

  if (elements_written != size) {
    perror("Error writing to file");
  } else {
    printf("Binary data successfully written to %s\n", filename);
  }

  fclose(file);
}
