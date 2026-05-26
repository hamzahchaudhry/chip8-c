#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>

extern unsigned char key[16];
extern unsigned char gfx[64 * 32];
extern bool drawFlag;

void initialize(void);
int loadGame(const char* path);
void emulateCycle(void);
void tickTimers(void);

#endif
