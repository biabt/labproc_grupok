# Makefile - Calculadora Binaria ARM (Raspberry Pi 3 / Cortex-A53)
#
# Rodar ESTE Makefile DIRETAMENTE no Raspberry Pi (ele já é ARM64,
# então não precisamos de cross-compiler lá — o gcc nativo do Pi já
# gera código ARM64 nativamente).

CC      = gcc
AS      = gcc          # usamos o gcc também para montar o .s (mais simples)
CFLAGS  = -Wall -O2 $(shell pkg-config --cflags sdl2 SDL2_ttf)
LDFLAGS = $(shell pkg-config --libs sdl2 SDL2_ttf)

TARGET  = calculadora_arm

all: $(TARGET)

alu.o: alu.s
	$(AS) -c alu.s -o alu.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

$(TARGET): alu.o main.o
	$(CC) alu.o main.o -o $(TARGET) $(LDFLAGS)

test: alu.o test_alu.c
	$(CC) alu.o test_alu.c -o test_alu
	./test_alu

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o $(TARGET) test_alu

.PHONY: all run clean test
