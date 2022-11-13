#include "chip8.h"
#include <stdio.h>

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

  // TODO: Load fontset
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

}
