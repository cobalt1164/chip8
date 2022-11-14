#include "chip8.h"
#include <stdio.h>
#include <cstdlib>

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

void chip8::loadGame(char fname[]) {
  int bufferSize = 4096;
  char buffer[bufferSize];
  FILE *f = fopen(fname, "rb");

  if (!f) {
    printf("Could not read file");
    return;
  }

  fread(buffer, sizeof(char), bufferSize, f);

  for (int i = 0; i < bufferSize; i++) {
    memory[i + 512] = buffer[i]; // Start at 0x200=512
  }

  fclose(f);
}

void chip8::emulateCycle() {
  // Fetch (opcode at pc + opcode at (pc+1) )
  opcode = memory[pc] << 8 | memory[pc + 1];

  // Decode (check first 4 bits)
  switch (opcode & 0xF000) { 
    case 0x0000: // For opcodes that aren't dependent on first 4
      switch (opcode & 0x000F) {
        case 0x000E: // 00EE: Return from subroutine
          // Set pc to sp-1
          pc = stack[--sp];
        break;

        case 0x0000: // 00E0: Clear the screen
          for (int i = 0; i < 2048; i++)
            gfx[i] = 0;
          pc += 2;
        break;
      }

    case 0xA000: // ANNN: Sets I to the address NNN.
      I = opcode & 0x0FFF;
      pc += 2; // Add 2 to pc because we finished executing
    break;

    case 0x1000: // 1NNN: Jumps to address NNN.
      pc = opcode & 0x0FFF;
    break;

    case 0x2000: // 2NNN: Calls subroutine at NNN.
      stack[sp++] = pc;
      pc = opcode & 0x0FFF;
    break;

    case 0x3000: // 3XNN: Skips the next instruction if VX equals NN 
      if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
        pc += 4;
    break;

    case 0x4000: // 4XNN: Skips the next instruction if VX does not equal NN
      if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
        pc += 4;
    break;

    case 0x5000: // 5XY0: Skips the next instruction if VX equals VY
      if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
        pc += 4;
    break;

    case 0x6000: // 6XNN: Sets VX to NN.
      V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
    break;

    case 0x7000: // 7XNN: Adds NN to VX (dont change carry flag)
      V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);

    case 0x8000: 
      switch (opcode & 0x000F) {
        case 0x0000: // 8XY0: Set VX to VY
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
          pc += 2;
        break;

        case 0x0001: // 8XY1: Set VX to VX or VY
          V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
          pc += 2;
        break;

        case 0x0002: // 8XY2: Set VX to VX and VY
          V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
          pc += 2;
        break;

        case 0x0003: // 8XY3: Set VX to VX xor VY
          V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
          pc += 2;
        break;

        case 0x0004: // 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
         // if VX > 255 - VY
          if (V[(opcode & 0x0F00) >> 8] > 0xFF - V[(opcode & 0x00F0) >> 4])
            V[0xF] = 1;
          else
            V[0xF] = 0;
          V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
          pc += 2;
        break;

        case 0x0005: // 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not. 
         // if VY > VX
          if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
            V[0xF] = 0;
          else
            V[0xF] = 1;
          V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
          pc += 2;
        break;

        case 0x0006: // 8XY6: Stores the least significant bit of VX in VF and then shifts VX to the right by 1. 
          V[0xF] = V[(opcode & 0x0F00) >> 8] & 0xF0; // Big endian, store first bit in VF
          V[(opcode & 0x0F00) >> 8] >>= 1;
          pc += 2;
        break;

        case 0x0007: // 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
         // if VX > VY
          if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
            V[0xF] = 0;
          else
            V[0xF] = 1;
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
          pc += 2;
        break;

        default: // 8XYE: Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
          V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x0F; // Big endian, store last bit in VF
          V[(opcode & 0x0F00) >> 8] <<= 1;
          pc += 2;

      }
    break;
    case 0x9000: // 9XY0: Skips the next instruction if VX does not equal VY.
      if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
        pc += 4;
    break;

    case 0xB000: // BNNN: Jumps to the address NNN plus V0.
      pc = V[0] + (opcode & 0x0FFF);
    break;

    case 0xC000: // CXNN: Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
      V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
      pc += 2;
    break;

    case 0xD000: { // DXYN: 
     /*
      Draws a sprite at coordinate (VX, VY) that has a width of 8 
      pixels and a height of N pixels. Each row of 8 pixels is read
      as bit-coded starting from memory location I; I value does not
      change after the execution of this instruction. As described above,
      VF is set to 1 if any screen pixels are flipped from set to unset
      when the sprite is drawn, and to 0 if that does not happen.
      */
      unsigned short VX = V[(opcode & 0x0F00) >> 8];
      unsigned short VY = V[(opcode & 0x00F0) >> 4];
      unsigned short N = opcode & 0x000F;

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
      pc += 2;
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
