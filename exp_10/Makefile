# Makefile - Sistema de Tranca Eletronica (Raspberry Pi 3 / Cortex-A53)
#
# Compilar e executar diretamente no Raspberry Pi com g++ nativo
# Dependencias:
#   - wiringPi
#   - bibliotecas LCD/PCF8574 (wiringPi)

CXX      = g++
CXXFLAGS = -Wall -Wextra -O2 -pedantic -std=c++17
LDFLAGS  = -lwiringPi

TARGET   = fechadura
SOURCES  = main.cpp
OBJECTS  = $(SOURCES:.cpp=.o)

all: $(TARGET)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "================================"
	@echo "Build successful: ./$(TARGET)"
	@echo "================================"

run: $(TARGET)
	@echo "Iniciando sistema de tranca (requer privilegios de root)..."
	sudo ./$(TARGET)

debug: CXXFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "Debug build complete with symbols"

clean:
	rm -f $(OBJECTS) $(TARGET)

help:
	@echo "Targets disponiveis:"
	@echo "  make       - Compila o projeto"
	@echo "  make run   - Compila e executa (requer sudo)"
	@echo "  make debug - Compila com simbolos de debug"
	@echo "  make clean - Remove arquivos compilados"
	@echo "  make help  - Mostra esta mensagem"

.PHONY: all run debug clean help
