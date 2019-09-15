// Sam Little
// 9.14.2019
// chip8.h

#include <stdint.h>

#define MEMORY 4098

class Chip8
{
  public:
    // true if screen needs to be drawn
    bool drawFlag;
    // graphics
    uint8_t gfx[2048];
    // keypad
    uint8_t key[16]; 
    Chip8();
    ~Chip8();
    // load game rom. "name" is filename
    bool loadGame(const char *name);
    // emulates cycle
    void emulateCycle();
  private:
    // current opcode
    uint16_t opcode;
    // memory of our chip8
    uint8_t memory[MEMORY];
    // cpu registers (15 8-bit registers)
    uint8_t V[16];
    // index register
    uint16_t I;
    // program counter
    uint16_t pc;
    //interrupts / hardware registers
    uint8_t delay_timer;
    uint8_t sound_timer;
    // stack
    uint16_t stack[16]; 
    // stack pointer
    uint16_t sp;
    // sets up the object
    void initialize();
};
