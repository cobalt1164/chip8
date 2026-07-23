#include "chip8.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>

void chip8::initialize() {
  opcode = 0;
  pc = 0x200; // Memory starts at 0x200, all below is used for font data
  I = 0;
  sp = 0;
  sound_timer = 0;
  delay_timer = 0;

  // Clear memory
  for (int i = 0; i < 4096; i++)
    memory[i] = 0;

  // Clear display
  for (int i = 0; i < 2048; i++)
    gfx[i] = 0;

  // Clear flag registers and stack
  for (int i = 0; i < 16; i++) {
    V[i] = 0;
    stack[i] = 0;
    key[i] = 0;
  }

  // Load font into first area of memory
  for (int i = 0; i < 80; i++)
    memory[i] = chip8_fontset[i];
}

void chip8::loadGame(const char fname[]) {
  const int bufferStart = 0x200;
  const int bufferSize = 4096 - bufferStart; 
  char buffer[bufferSize];
  FILE *f = fopen(fname, "rb");

  if (!f) {
    printf("Could not read file");
    return;
  }

  size_t bytesRead = fread(buffer, sizeof(char), bufferSize, f);

  for (size_t i = 0; i < bytesRead; i++) {
    memory[i + bufferStart] = buffer[i]; // Start at 0x200=512
  }

  fclose(f);
}

void chip8::emulateCycle() {
    // Fetch (opcode at pc + opcode at (pc+1) )
    opcode = memory[pc] << 8 | memory[pc + 1];

    // Immediately go to next opcode (better to do it this way than to +=2 in every single instruction)
    pc += 2;

    unsigned char first_nibble = (opcode & 0xF000) >> 12; // first nibble denotes instruction type  
    unsigned char X = (opcode & 0x0F00) >> 8; // X - used to lookup VX reg from V0-VF
    unsigned char Y = (opcode & 0x00F0) >> 4; // Y - used to lookup VY reg from V0-VF
    unsigned char N = opcode & 0x000F; // N - a 4bit number

    unsigned char NN = opcode & 0x00FF; // NN - 8 bit
    unsigned short NNN = opcode & 0x0FFF; // NNN - 12 bit used for memory accesses

    unsigned char VX = V[X];
    unsigned char VY = V[Y];

  // Decode (check first 4 bits)
  switch (first_nibble) { 
    case 0x0: // For opcodes that aren't dependent on first 4
      switch (N) {
        case 0xE: // 00EE: Return from subroutine
          // Set pc to sp-1
          pc = stack[--sp];
        break;

        case 0x0: // 00E0: Clear the screen
          memset(gfx, 0, sizeof(gfx));
        break;
      }
    break;

    case 0xA: // ANNN: Sets I to the address NNN.
        I = NNN;
    break;

    case 0x1: // 1NNN: Jumps to address NNN.
        pc = NNN;
    break;

    case 0x2: // 2NNN: Calls subroutine at NNN.
      stack[sp++] = pc;
      pc = NNN;
    break;

    case 0x3: // 3XNN: Skips the next instruction if VX equals NN 
      if (VX == NN)
        pc += 2;
    break;

    case 0x4: // 4XNN: Skips the next instruction if VX does not equal NN
      if (VX != NN)
        pc += 2;
    break;

    case 0x5: // 5XY0: Skips the next instruction if VX equals VY
      if (VX == VY)
        pc += 2;
    break;

    case 0x6: // 6XNN: Sets VX to NN.
        V[X] = NN;
    break;

    case 0x7: // 7XNN: Adds NN to VX (dont change carry flag)
        V[X] += NN;
    break;

    case 0x8: 
      switch (N) {
        case 0x0: // 8XY0: Set VX to VY
            V[X] = VY;
        break;

        case 0x1: // 8XY1: Set VX to VX or VY
            V[X] |= VY;
        break;

        case 0x2: // 8XY2: Set VX to VX and VY
            V[X] &= VY;
        break;

        case 0x3: // 8XY3: Set VX to VX xor VY
            V[X] ^= VY;
        break;

        case 0x4: // 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
          if (VX > 0xFF - VY)
            V[0xF] = 1;
          else
            V[0xF] = 0;
          V[X] += VY;
        break;

        case 0x5: // 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not. 
          if (VY > VX)
            V[0xF] = 0;
          else
            V[0xF] = 1;
          V[X] -= VY;
        break;

        case 0x6: // 8XY6: Stores the least significant bit of VX in VF and then shifts VX to the right by 1. 
          V[0xF] = VX & 0x1;
          V[X] >>= 1;
        break;

        case 0x7: // 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
          if (VX > VY)
            V[0xF] = 0;
          else
            V[0xF] = 1;
          V[X] = VY - VX;
        break;

        default: // 8XYE: Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
          V[0xF] = (VX & 0x80) >> 7;
          V[X] <<= 1;
        break;
      }
    break;

    case 0x9: // 9XY0: Skips the next instruction if VX does not equal VY.
      if (VX != VY)
        pc += 2;
    break;

    case 0xB: // BNNN: Jumps to the address NNN plus V0.
      pc = V[0] + NNN;
    break;

    case 0xC: // CXNN: Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
      V[X] = (rand() % 256) & NN;
    break;

    case 0xD: { // DXYN: 
      V[0xF] = 0; // reset VF
      for (int j = 0; j < N; j++) {
        for (int i = 0; i < 8; i++) {
          unsigned char sprite = memory[I + j] << i;
          if (gfx[(64 * j) + (64 * VY) + i + VX] == 1 && (sprite & 0x80) == 1)
            V[0xF] = 1; // carry flag, collision detection
          gfx[(64 * j) + (64 * VY) + i + VX] ^= (sprite & 0x80);
        }
      }
      drawFlag = true;
    }
    break;

    default:
      printf("Unknown opcode\n");
  }

  // Handle Timers
  if (delay_timer > 0)
    --delay_timer;

  if (sound_timer > 0) {
    printf("beep");
    --sound_timer;
  }
}
