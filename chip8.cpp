#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include "time.h"

#include "chip8.h"

unsigned char chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

Chip8::Chip8() {}
Chip8::~Chip8() {}

void Chip8::initialize()
{
  //init registers, memory for first time
  drawFlag = false;
  for(int i=0; i < (2048); ++i){
    gfx[i] = 0;
  }
  for(int i=0; i<16; ++i){
    key[i] = 0;
    V[i] = 0;
    stack[i] = 0;
  }
  opcode = 0;
  for(int i=0; i<MEMORY; ++i){
    memory[i] = 0;
  }
  I = 0;
  pc = 0x200;
  delay_timer = 0;
  sound_timer = 0;
  sp = 0;
  srand(time(NULL));

  for (int i = 0; i < 80; ++i) {
        memory[i] = chip8_fontset[i];
  }
}

bool Chip8::loadGame(const char *name)
{
  initialize();
  printf("Now loading: %s\n", name);

  FILE* rom = fopen(name, "rb");
  if(rom == NULL){
    std::cerr << "ERROR: Cannot open ROM." << std::endl;
    return false;
  }
  fseek(rom, 0, SEEK_END);
  long rom_size = ftell(rom);
  rewind(rom);

  char* rom_buffer = (char*) malloc(sizeof(char) * rom_size);
  if(rom_buffer == NULL){
    std::cerr << "ERROR: Could not allocate memory to load ROM." << std::endl;
    return false;
  }
  
  size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, rom);
  if(result != rom_size){
    std::cerr << "Failed to read ROM." << std::endl;
    return false;
  }
  
  if((4096-512) > rom_size){
    for(int i=0; i<rom_size; ++i){
      memory[i+512] = (uint8_t)rom_buffer[i];
    }
  } else {
    std::cerr << "ROM was too large to fit in 4096 bytes." << std::endl;
    return false;
  }

  fclose(rom);
  free(rom_buffer);
  return true;
}

void Chip8::emulateCycle()
{
  //fetch
  opcode = memory[pc] << 8 | memory[pc + 1];
    
  //decode
  switch(opcode & 0xF000){
    
    //0x0000
    case 0x0000:
    {
      switch(opcode & 0x000F){
        case 0x0000:
        {
          //clear screen
          for(int i=0; i<2048; ++i){
            gfx[i] = 0;  
          }
          drawFlag = true;
          pc += 2;
        }
          break;
        case 0x000E:
          //return from subroutine
          --sp;
          pc = stack[sp];
          pc += 2;
          break;
        default:
          printf("\nUnknown op code: %.4X\n", opcode);
          exit(3);
      }
    }
      break;
    case 0x1000:
      //jump to address NNN.
      pc = opcode & 0x0FFF;
      break;
    
    case 0x2000:
      //calls subroutine at NNN.
      stack[sp] = pc;
      ++sp;
      pc = opcode & 0x0FFF;
      break;
    case 0x3000:
      //skips next instruction if VX == NN
      if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)){
        pc += 4;
      } else {
        pc += 2;
      }
      break;
    case 0x4000:
      //skips next instruction in VX != NN
      if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)){
        pc += 4;
      } else {
        pc += 2;
      }
      break;
    case 0x5000:
      //skips next instruction if VX = VY
      if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]){
        pc += 4;
      } else {
        pc += 2;  
      }
      break;
    case 0x6000:
      //sets VX to NN.
      V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
      pc += 2;
      break;
    case 0x7000:
      //adds NN to VX (carry flag unchanged)
      V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
      pc += 2;
      break;
    case 0x8000:
      switch(opcode & 0x000F){
        case 0x0000:
          //sets VX to VY
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;
        case 0x0001:
          //sets VX to VX or VY (bitwise or)
          V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;
        case 0x0002:
          //sets VX to VX and VY (bitwise and)
          V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;
        case 0x0003:
          //sets VX to VX xor VY (bitwise xor)
          V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;
        case 0x0004:
          //adds vy to vx. vf is 1 when there's a carry, 0 when not
          V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
          if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
            V[0xF] = 1;
          else
            V[0xF] = 0;
          pc += 2;
          break;
        case 0x0005:
          //vy subtracted from vx. vf is 0 when there's a borrow, and 1 when there isnt
          if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
            V[0xF] = 0;
          else
            V[0xF] = 1;
          V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;
        case 0x0006:
          //Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
          V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
          V[(opcode & 0x0F00) >> 8] >>= 1;
          pc += 2;
          break;
        case 0x0007:
          //Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
          if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
            V[0xF] = 0;
          else
            V[0xF] = 1;
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
          pc += 2;
          break;
        case 0x000E:
          //Stores the most significant bit of VX in VF and then shifts VX to the left by 1.[3]
          V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
          V[(opcode & 0x0F00) >> 8] <<= 1;
          pc += 2;
          break;
        default:
          printf("\nUnknown op code: %.4X\n", opcode);
          exit(3);    
      }
      break; 
    case 0x9000:
      //Skips the next instruction if VX doesn't equal VY. (Usually the next instruction is a jump to skip a code block)
      if(V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
        pc += 4;
      else
        pc += 2;
      break;
    case 0xA000:
      //Sets I to the address NNN.
      I = opcode & 0x0FFF;
      pc += 2;
      break;
    case 0xB000:
      //Jumps to the address NNN plus V0.
      pc = (opcode & 0x0FFF) + V[0];
      break;
    case 0xC000:
      //Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
      V[(opcode & 0x0F00) >> 8] = (rand() % (0xFF + 1)) & (opcode & 0x00FF);
      pc += 2;
      break;
    case 0xD000:
    {
      //Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. Each row of 8 pixels is read as bit-coded starting from memory location I; I value doesn’t change after the execution of this instruction. As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that doesn’t happen
      unsigned short x = V[(opcode & 0x0F00) >> 8];
      unsigned short y = V[(opcode & 0x00F0) >> 4];
      unsigned short height = opcode & 0x000F;
      unsigned short pixel;      
      V[0xF] = 0;
      
      for(int col=0; col<height; col++){
        pixel = memory[I + col];
        for(int row=0; row < 8; row++){
          if((pixel & (0x80 >> row)) != 0){
            if(gfx[(x + row + ((y + col) * 64))] == 1){
              V[0xF] = 1;
            }
            gfx[x + row + ((y + col) * 64)] ^= 1;
          }
        }
      }
      drawFlag = true;
      pc += 2;
    }
      break;
    case 0xE000:
      switch(opcode & 0x00FF){
        case 0x009E:
          //Skips the next instruction if the key stored in VX is pressed. (Usually the next instruction is a jump to skip a code block)
          if(key[V[(opcode & 0x0F00) >> 8]])
            pc += 4;
          else
            pc += 2;
          break;
        case 0x00A1:
          //Skips the next instruction if the key stored in VX isn't pressed. (Usually the next instruction is a jump to skip a code block)
          if(key[V[(opcode & 0x0F00) >> 8]] == 0)
            pc += 4;
          else
            pc += 2;
          break;
        default:
          printf("\nUnknown op code: %.4X\n", opcode);
          exit(3);
      }
      break;
    case 0xF000:
      printf("got 0xF000\n");
      switch(opcode & 0x00FF){
        case 0x0007:
          printf("\tgot 0xF007\n");
          //Sets VX to the value of the delay timer.
          V[(opcode & 0x0F00) >> 8] = delay_timer;
          pc += 2;
          break;
        case 0x000A:
        {
          printf("\tgot 0xF00A\n");
          //A key press is awaited, and then stored in VX. (Blocking Operation. All instruction halted until next key event)
          bool key_pressed = false;
          for(int i=0; i<16; ++i){
            if(key[i] != 0){
              V[(opcode & 0x0F00) >> 8] = i;
              key_pressed = true;
            }
          }
          if(!key_pressed)
            return;
          pc += 2;
        }
          break;
        case 0x0015:
          printf("\tgot 0xF015\n");
          //Sets the delay timer to VX.
          delay_timer = V[(opcode & 0x0F00) >> 8];
          pc += 2;
          break;
        case 0x0018:
          printf("\tgot 0xF0018\n");
          //Sets the sound timer to VX.
          sound_timer = V[(opcode & 0x0F00) >> 8];
          pc += 2;
          break;
        case 0x001E:
          printf("\tgot 0xF01E\n");
          //Adds VX to I.[4]
          if(I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
            V[0xF] = 1;
          else
            V[0xF] = 0;
          I += V[(opcode & 0x0F00) >> 8];
          pc += 2;
          break;
        case 0x0029:
          printf("\tgot 0xF029\n");
          //Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
          I = V[(opcode & 0x0F00) >> 8] * 0x5;
          pc += 2;
          break;
        case 0x0033:
          printf("\tgot 0xF033\n");
          //Stores the binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2. (In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.)
          memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
          memory[I + 2] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
          memory[I + 3] = V[(opcode & 0x0F00) >> 8] % 10;
          pc += 2;
          break;
        case 0x0055:
        {
          printf("\tgot 0xF055\n");
          //Stores V0 to VX (including VX) in memory starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified.
          for(int i=0; i <= ((opcode & 0x0F00)>>8); ++i){
            memory[I+i] = V[i];
          }
          I += ((opcode & 0x0F00) >> 8) + 1;
          pc += 2;
        }
          break;
        case 0x0065:
        {
          printf("\tgot 0xF065\n");
          //Fills V0 to VX (including VX) with values from memory starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified.
          for(int i=0; i <= V[(opcode & 0x0F00) >> 8]; ++i){
            V[i] = memory[I + i];
          }
          I += ((opcode & 0x0F00) >> 8) + 1;
          pc += 2;
        }
          break;
        default:
          printf("\nUnknown op code: %.4X\n", opcode);
          exit(3);
      }
      break;
    default:
      printf("\nUnknown op code: %.4X\n", opcode);
      exit(3);
  } 
  // update timers
  if(delay_timer > 0)
    --delay_timer;
  if(sound_timer > 0)
    --sound_timer;
}
