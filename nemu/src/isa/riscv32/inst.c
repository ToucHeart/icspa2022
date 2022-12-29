/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum
{
  TYPE_I,
  TYPE_U,
  TYPE_S,
  TYPE_N, // none
  TYPE_J,
  TYPE_R,
  TYPE_B,
};

#define src1R()     \
  do                \
  {                 \
    *src1 = R(rs1); \
  } while (0)
#define src2R()     \
  do                \
  {                 \
    *src2 = R(rs2); \
  } while (0)
#define immI()                        \
  do                                  \
  {                                   \
    *imm = SEXT(BITS(i, 31, 20), 12); \
  } while (0)
#define immU()                              \
  do                                        \
  {                                         \
    *imm = SEXT(BITS(i, 31, 12), 20) << 12; \
  } while (0)
#define immS()                                               \
  do                                                         \
  {                                                          \
    *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); \
  } while (0)
#define immJ()                                                                                                       \
  do                                                                                                                 \
  {                                                                                                                  \
    *imm = SEXT((BITS(i, 31, 31) << 19 | BITS(i, 19, 12) << 11 | BITS(i, 20, 20) << 10 | BITS(i, 30, 21)) << 1, 20); \
  } while (0)
#define immB()                                                                                                   \
  do                                                                                                             \
  {                                                                                                              \
    *imm = SEXT((BITS(i, 31, 31) << 11 | BITS(i, 7, 7) << 10 | BITS(i, 30, 25) << 4 | BITS(i, 11, 8)) << 1, 12); \
  } while (0)

static void decode_operand(Decode *s, int *dest, word_t *src1, word_t *src2, word_t *imm, int type)
{
  uint32_t i = s->isa.inst.val;
  int rd = BITS(i, 11, 7);
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *dest = rd;
  switch (type)
  {
  case TYPE_I:
    src1R();
    immI();
    break;
  case TYPE_U:
    immU();
    break;
  case TYPE_S:
    src1R();
    src2R();
    immS();
    break;
  case TYPE_J:
    immJ();
    break;
  case TYPE_R:
    src1R();
    src2R();
    break;
  case TYPE_B:
    src1R();
    src2R();
    immB();
    break;
  }
#if 0
  printf("dest is %d\n", *dest);
  printf("rs1 is %d\n", rs1);
  printf("rs2 is %d\n", rs2);
  printf("src1 is %x\n", *src1);
  printf("src2 is %x\n", *src2);
  printf("imm is %d\n", *imm);
#endif
}

static int decode_exec(Decode *s)
{
  int dest = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc; // dnpc==snpc==pc+4
#if 1
  printf("pc is %x\n", s->pc);
#endif

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)           \
  {                                                                    \
    decode_operand(s, &dest, &src1, &src2, &imm, concat(TYPE_, type)); \
    __VA_ARGS__;                                                       \
  }

  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R(dest) = imm);
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(dest) = imm + s->pc);

  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, src1 == src2 ? s->dnpc = s->dnpc - 4 + imm : 1);
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B, src1 != src2 ? s->dnpc = s->dnpc - 4 + imm : 1);
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, (int)src1 >= (int)src2 ? s->dnpc = s->dnpc - 4 + imm : 1);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, (int)src1 < (int)src2 ? s->dnpc = s->dnpc - 4 + imm : 1);

  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, R(dest) = s->dnpc, s->dnpc = s->dnpc + imm - 4);

  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, src2));

  // INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R(dest) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, word_t a = Mr(src1 + imm, 2); R(dest) = (a & 0x8000) ? (a | 0xffff0000) : a);
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R(dest) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(dest) = src1 + imm);
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, R(dest) = s->dnpc, s->dnpc = src1 + imm);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, R(dest) = src1 < imm ? 1 : 0);
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai, I, R(dest) = (int)src1 >> imm);
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli, I, R(dest) = src1 >> imm);
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli, I, R(dest) = src1 << imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(dest) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R(dest) = Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, R(dest) = src1 & imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, R(dest) = src1 | imm);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, R(dest) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, R(dest) = (int)src1 < (int)imm ? 1 : 0);

  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(dest) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R(dest) = src1 - src2);
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul, R, R(dest) = src1 * src2);
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div, R, R(dest) = src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem, R, R(dest) = src1 % src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R, R(dest) = src1 << src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, R(dest) = src1 & src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, R(dest) = src1 | src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, R(dest) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, R(dest) = (src1 < src2 ? 1 : 0));
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, R(dest) = ((int)src1 < (int)src2 ? 1 : 0));

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s)
{
  s->isa.inst.val = inst_fetch(&s->snpc, 4); // 取指令存在val中，并将snpc+4
  return decode_exec(s);                     // 译码并执行
}
