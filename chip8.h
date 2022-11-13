class chip8 {
  private:
    // Store current opcode
    unsigned short opcode;

    // CHIP-8 has 4096 memory slots, 1 byte each
    unsigned char memory[4096];

    // Store flags. 16th flag is carry flag
    unsigned char V[16];

    // Index register I, program counter pc
    // Values from 0x000 to 0xFFF
    unsigned short I;
    unsigned short pc;

    // Screen size of CHIP-8 is 64x32
    unsigned char gfx[2048];

    // Used for timing. Counts down by 60hz
    unsigned char delay_timer;

    // Used for sound. When nonzero, beep
    unsigned char sound_timer;

    // 16 level stack
    unsigned short stack[16];
    unsigned short sp;

    // 0x0-0xF. Hex based keypad
    unsigned char key[16];

  public:
    void initialize();
    void emulateCycle();
    void loadGame(char fname[]);
};
