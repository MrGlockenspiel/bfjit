#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jit.h"
#include "types.h"

void push_ins(u8 *memory, u64 *index, i32 count, ...) {
  va_list args;
  va_start(args, count);

  for (i32 i = 0; i < count; i++) {
    u8 ins = (u8)va_arg(args, i32);
    memory[*index] = ins;
    ++*index;
  }
  va_end(args);

  return;
}

void push_u32(u8 *memory, u64 *index, u32 value) {
  push_ins(memory, index, 4, value & 0xFF, (value >> 8) & 0xFF,
           (value >> 16) & 0xFF, (value >> 24) & 0xFF);

  return;
}

void push_u64(u8 *memory, u64 *index, u64 value) {
  push_ins(memory, index, 8, (u8)(value & 0xFF), (u8)((value >> 8) & 0xFF),
           (u8)((value >> 16) & 0xFF), (u8)((value >> 24) & 0xFF),
           (u8)((value >> 32) & 0xFF), (u8)((value >> 40) & 0xFF),
           (u8)((value >> 48) & 0xFF), (u8)((value >> 56) & 0xFF));

  return;
}

void push_addr(u8 *memory, u64 *index, void *address) {
  u64 value = (u64)address;
  push_ins(memory, index, 8, (u8)(value & 0xFF), (u8)((value >> 8) & 0xFF),
           (u8)((value >> 16) & 0xFF), (u8)((value >> 24) & 0xFF),
           (u8)((value >> 32) & 0xFF), (u8)((value >> 40) & 0xFF),
           (u8)((value >> 48) & 0xFF), (u8)((value >> 56) & 0xFF));

  return;
}

void asm_add_r13(u8 *memory, u64 *index, u32 val) {
  // add r13, val
  push_ins(memory, index, 3, 0x49, 0x81, 0xC5);
  push_u32(memory, index, val);

  return;
}

void asm_sub_r13(u8 *memory, u64 *index, u32 val) {
  // sub r13, val
  push_ins(memory, index, 3, 0x49, 0x81, 0xED);
  push_u32(memory, index, val);

  return;
}

void asm_inc_r13(u8 *memory, u64 *index) {
  // inc r13
  push_ins(memory, index, 3, 0x49, 0xFF, 0xC5);

  return;
}

void asm_dec_r13(u8 *memory, u64 *index) {
  // dec r13
  push_ins(memory, index, 3, 0x49, 0xFF, 0xCD);

  return;
}

void asm_add_ptr_r13(u8 *memory, u64 *index, u8 val) {
  // add byte [r13], val
  push_ins(memory, index, 5, 0x41, 0x80, 0x45, 0x00, val);

  return;
}

void asm_sub_ptr_r13(u8 *memory, u64 *index, u8 val) {
  // sub byte [r13], val
  push_ins(memory, index, 5, 0x41, 0x80, 0x6D, 0x00, val);

  return;
}

void asm_mov_r13_qword(u8 *memory, u64 *index, u64 val) {
  // mov r13, qword val
  push_ins(memory, index, 2, 0x49, 0xBD);
  push_u64(memory, index, val);
  return;
}

void asm_mov_r13_arg1(u8 *memory, u64 *index) {
  push_ins(memory, index, 3, 0x49, 0x89, 0xFD);
  return;
}

void asm_putchar(u8 *memory, u64 *index) {
  // mov rax, qword function
  push_ins(memory, index, 2, 0x48, 0xB8);
  push_u64(memory, index, (u64)putchar);

  // mov dil, byte [r13]
  push_ins(memory, index, 4, 0x41, 0x8A, 0x7D, 0x00);

  // call rax
  push_ins(memory, index, 2, 0xFF, 0xD0);

  // pop rbp
  // push_ins(memory, index, 1, 0x5D);

  return;
}

void asm_syscall_read(u8 *memory, u64 *index) {
  // mov rax, 0    ; read syscall number
  push_ins(memory, index, 2, 0x48, 0xB8);
  push_u64(memory, index, 0);

  // mov rdi, 0    ; fd of stdin
  push_ins(memory, index, 2, 0x48, 0xBF);
  push_u64(memory, index, 0);

  // mov rsi, r13  ; char*
  push_ins(memory, index, 3, 0x4C, 0x89, 0xEE);

  // mov rdx, 1    ; count
  push_ins(memory, index, 2, 0x48, 0xBA);
  push_u64(memory, index, 1);

  // syscall
  push_ins(memory, index, 2, 0x0F, 0x05);

  return;
}

void asm_cmp_byte_r13(u8 *memory, u64 *index, u8 value) {
  // cmp byte [r13], 0
  push_ins(memory, index, 4, 0x41, 0x80, 0x7D, 0x00);
  push_ins(memory, index, 1, value);

  return;
}

void asm_jz(u8 *memory, u64 *index, u32 address) {
  // jz near address
  push_ins(memory, index, 2, 0x0F, 0x84);
  push_u32(memory, index, address);

  return;
}

void asm_jnz(u8 *memory, u64 *index, u32 address) {
  // jnz near address
  push_ins(memory, index, 2, 0x0F, 0x85);
  push_u32(memory, index, address);

  return;
}

void asm_ret(u8 *memory, u64 *index) {
  // ret
  push_ins(memory, index, 1, 0xC3);

  return;
}
