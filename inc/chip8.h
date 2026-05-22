#ifndef CHIP8_H
#define CHIP8_H

extern unsigned char gfx[64 * 32];
extern bool drawFlag;

void initialize(void);
int loadGame(char *path);
void emulateCycle(void);


#endif
