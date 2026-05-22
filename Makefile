all:
	rm -f chip8_emulator
	gcc src/main.c src/chip8.c -Iinc -o chip8_emulator -lSDL3
	./chip8_emulator

clean:
	rm -f chip8_emulator
