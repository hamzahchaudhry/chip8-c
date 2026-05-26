#include "chip8.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static const unsigned char chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

unsigned char key[16];

unsigned short opcode;
unsigned char memory[4096];
unsigned char V[16];

unsigned short pc;
unsigned short I;

unsigned short stack[16];
unsigned short sp;

unsigned char gfx[64 * 32];
bool drawFlag = true;

unsigned char delay_timer;
unsigned char sound_timer;

void initialize(void) {
  pc = 0x200;
  opcode = 0;
  I = 0;
  sp = 0;
  for (int i = 0; i < (64 * 32); i++) gfx[i] = 0;
  for (int i = 0; i < 16; i++) stack[i] = 0;
  sp = 0;
  for (int i = 0; i < 16; i++) V[i] = 0;
  for (int i = 0; i < 80; i++) memory[i] = chip8_fontset[i];
  delay_timer = 0;
  sound_timer = 0;
}

int loadGame(const char* path) {
  FILE* fp = fopen(path, "rb");
  if (!fp) {
    perror(path);
    return -1;
  }

  size_t max_size = 4096 - 0x200;
  size_t n = fread(&memory[0x200], 1, max_size, fp);

  if (ferror(fp)) {
    fclose(fp);
    return -1;
  }

  fclose(fp);
  return n > 0 ? 0 : -1;
}

void emulateCycle(void) {
  opcode = memory[pc] << 8 | memory[pc + 1];
  // printf("pc=%03X opcode: 0x%04X\n", pc, opcode);

  unsigned char x = (opcode & 0x0F00) >> 8;
  unsigned char y = (opcode & 0x00F0) >> 4;
  unsigned char kk = opcode & 0x00FF;
  unsigned short nnn = opcode & 0x0FFF;

  switch (opcode & 0xF000) {
    case 0x0000:
      switch (opcode & 0x000F) {
        case 0x0000:  // 0x00E0 - CLS
          // clear screen
          for (int i = 0; i < (64 * 32); i++) gfx[i] = 0;
          pc += 2;
          break;

        case 0x000E:  // 0x00EE - RET
          // return from subroutine
          pc = stack[--sp];
          break;

        default:
          printf("unknown opcode: 0x%04X\n", opcode);
          pc += 2;
          break;
      }
      break;

    case 0x1000:  // 0x1nnn - JP addr
      // jump to nnn
      pc = nnn;
      break;

    case 0x2000:  // 0x2nnn - CALL addr
      // call subroutine at nnn
      stack[sp++] = pc + 2;
      pc = nnn;
      break;

    case 0x3000:  // 0x3xkk - SE Vx, byte
      // skip next intr if Vx = kk
      if (V[x] == kk) pc += 4;
      else pc += 2;
      break;

    case 0x4000:  // 0x4xkk - SNE Vx, byte
      // skip next intr if Vx != kk
      if (V[x] != kk) pc += 4;
      else pc += 2;
      break;

    case 0x5000:  // 0x5xy0 - SE Vx, Vy
      // skip next intr if Vx = Vy
      if (V[x] == V[y]) pc += 4;
      else pc += 2;
      break;

    case 0x6000:  // 0x6xkk - LD Vx, byte
      // set Vx = kk
      V[x] = kk;
      pc += 2;
      break;

    case 0x7000:  // 0x7xkk - ADD Vx, byte
      // set Vx = Vx + kk
      V[x] += kk;
      pc += 2;
      break;

    case 0x8000:
      switch (opcode & 0x000F) {
        case 0x0000:  // 0x8xy0 - LD Vx, Vy
          // set Vx = Vy
          V[x] = V[y];
          pc += 2;
          break;

        case 0x0001:  // 0x8xy1 - OR Vx, Vy
          // set Vx = Vx OR Vy
          V[x] |= V[y];
          pc += 2;
          break;

        case 0x0002:  // 0x8xy2 - AND Vx, Vy
          // set Vx = Vx AND Vy
          V[x] &= V[y];
          pc += 2;
          break;

        case 0x0003:  // 0x8xy3 - XOR Vx, Vy
          // set Vx = Vx XOR Vy
          V[x] ^= V[y];
          pc += 2;
          break;

        case 0x0004:  // 0x8xy4 - ADD Vx, Vy
          // set Vx = Vx + Vy, set VF = carry
          V[0xF] = ((V[x] + V[y]) > 0xFF) ? 1 : 0;
          V[x] += V[y];
          pc += 2;
          break;

        case 0x0005:  // 0x8xy5 - SUB Vx, Vy
          // set Vx = Vx - Vy, set VF = NOT borrow
          V[0xF] = (V[x] >= V[y]) ? 1 : 0;
          V[x] -= V[y];
          pc += 2;
          break;

        case 0x0006:  // 0x8xy6 - SHR Vx {, Vy}
          // set Vx = Vx SHR 1
          V[0xF] = V[x] & 1;
          V[x] >>= 1;
          pc += 2;
          break;

        case 0x0007:  // 0x8xy7 - SUBN Vx, Vy
          // set Vx = Vy - Vx, set VF = NOT borrow
          V[0xF] = (V[y] >= V[x]) ? 1 : 0;
          V[x] = V[y] - V[x];
          pc += 2;
          break;

        case 0x000E:  // 0x8xyE - SHL Vx {, Vy}
          // set Vx = Vx SHR 1
          V[0xF] = (V[x] & 0x80) != 0;
          V[x] <<= 1;
          pc += 2;
          break;

        default:
          printf("unknown opcode: 0x%04X\n", opcode);
          pc += 2;
          break;
      }
      break;

    case 0x9000:  // 0x9xy0 - SNE Vx, Vy
      // skip next instr if Vx != Vy
      if (V[x] != V[y]) pc += 4;
      else pc += 2;
      break;

    case 0xA000:  // 0xAnnn - LD I, addr
      // set I = nnn
      I = nnn;
      pc += 2;
      break;

    case 0xB000:  // 0xBnnn - JP V0, addr
      // jump to nnn + V0
      pc = nnn + V[0];
      break;

    case 0xC000:  // 0xCxkk - RND Vx, byte
      // set Vx = random byte AND kk
      V[x] = rand() & kk;
      pc += 2;
      break;

    case 0xD000:  // 0xDxyn - DRW Vx, Vy, nibble
      // display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
      unsigned short vx = V[x];
      unsigned short vy = V[y];
      unsigned short height = opcode & 0x000F;
      unsigned short pixel;

      V[0xF] = 0;
      for (int yline = 0; yline < height; yline++) {
        pixel = memory[I + yline];
        for (int xline = 0; xline < 8; xline++) {
          if ((pixel & (0x80 >> xline)) != 0) {
            int px = (vx + xline) % 64;
            int py = (vy + yline) % 32;
            int idx = px + py * 64;
            if (gfx[idx] == 1) V[0xF] = 1;
            gfx[idx] ^= 1;
          }
        }
      }
      drawFlag = true;
      pc += 2;
      break;

    case 0xE000:
      switch (opcode & 0x000F) {
        case 0x000E:  // 0xEx9E - SKP Vx
          // skip next instr if key with value of Vx pressed
          pc += (key[V[x]]) ? 4 : 2;
          break;

        case 0x0001:  // 0xExA1 - SKNP Vx
          // skip next instr if key with value of Vx not pressed
          pc += (!key[V[x]]) ? 4 : 2;
          break;

        default:
          printf("unknown opcode: 0x%04X\n", opcode);
          pc += 2;
          break;
      }
      break;

    case 0xF000:
      switch (opcode & 0x000F) {
        case 0x0007:  // 0xFx07 - LD Vx, DT
          // set Vx = delay timer value
          V[x] = delay_timer;
          pc += 2;
          break;

        case 0x000A:  // 0xFx0A - LD Vx, K
          // wait for key press, store value of key in Vx
          for (int i = 0; i < 16; i++) {
            if (key[i]) {
              V[x] = i;
              pc += 2;
              break;
            }
          }
          break;

        case 0x0005:
          switch (opcode & 0x00F0) {
            case 0x0010:  // 0xFx15 - LD DT, Vx
              // set delay timer = Vx
              delay_timer = V[x];
              pc += 2;
              break;

            case 0x0050:  // 0xFx55 - LD [I], Vx
              // store regs V0 through Vx in memory starting at location I
              for (int i = 0; i <= x; i++) memory[I + i] = V[i];
              pc += 2;
              break;

            case 0x0060:  // 0xFx65 - LD [I], Vx
              // read regs V0 through Vx from memory starting at location I
              for (int i = 0; i <= x; i++) V[i] = memory[I + i];
              pc += 2;
              break;

            default:
              printf("unknown opcode: 0x%04X\n", opcode);
              pc += 2;
              break;
          }
          break;

        case 0x0008:  // 0xFx18 - LD ST, Vx
          // set sound timer = Vx
          sound_timer = V[x];
          pc += 2;
          break;

        case 0x000E:  // 0xFx1E - ADD I, Vx
          // set I = I + Vx
          I += V[x];
          pc += 2;
          break;

        case 0x0009:  // 0xFx29 - LD F, Vx
          // set I = location of sprite for digit Vx
          I = (V[x] & 0x0F) * 5;
          pc += 2;
          break;

        case 0x0003:  // 0xFx33 - LD B, Vx
          // store BCD representation of Vx in memory locations I, I+1, and I+2
          memory[I] = V[x] / 100;
          memory[I + 1] = (V[x] / 10) % 10;
          memory[I + 2] = (V[x] % 100) % 10;
          pc += 2;
          break;

        default:
          printf("unknown opcode: 0x%04X\n", opcode);
          pc += 2;
          break;
      }
      break;

    default:
      printf("unknown opcode: 0x%04X\n", opcode);
      pc += 2;
      break;
  }
}

void tickTimers(void) {
  if (delay_timer > 0) --delay_timer;
  if (sound_timer > 0) {
    if (sound_timer == 1) printf("BEEEEEEP\n");
    --sound_timer;
  }
}
