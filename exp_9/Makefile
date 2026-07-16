# Makefile - Metrônomo Eletrônico com pigpio (Raspberry Pi 3 / Cortex-A53)
#
# Compilar e executar diretamente no Raspberry Pi com gcc nativo
# Usa pigpio para controle PWM via hardware
#
# Requisitos:
#   - sudo apt-get install pigpio
#   - sudo pigpiod (executar antes de iniciar o programa)

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pedantic
LDFLAGS = -lpigpio -lm

TARGET  = metronomo
TEST_TARGET = test_periph

SOURCES = main.c
TEST_SOURCES = test_peripherals.c

OBJECTS = $(SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

all: $(TARGET)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "================================"
	@echo "Build successful: ./$(TARGET)"
	@echo "Requisitos:"
	@echo "  1. sudo pigpiod"
	@echo "  2. sudo ./$(TARGET)"
	@echo "================================"

test_periph.o: test_peripherals.c
	$(CC) $(CFLAGS) -c test_peripherals.c -o test_periph.o

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) -o $(TEST_TARGET) $(LDFLAGS)
	@echo "================================"
	@echo "Test build successful: ./$(TEST_TARGET)"
	@echo "Requisitos:"
	@echo "  1. sudo pigpiod"
	@echo "  2. sudo ./$(TEST_TARGET)"
	@echo "================================"

run: $(TARGET)
	@echo "Iniciando daemon pigpiod..."
	@sudo pigpiod
	@sleep 1
	@echo "Iniciando metrônomo (requer privilégios de root)..."
	sudo ./$(TARGET)

test: $(TEST_TARGET)
	@echo "Iniciando daemon pigpiod..."
	@sudo pigpiod
	@sleep 1
	@echo "Iniciando testes de periféricos (requer privilégios de root)..."
	sudo ./$(TEST_TARGET)

debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "Debug build complete with symbols"

debug-test: CFLAGS += -g -DDEBUG
debug-test: clean $(TEST_TARGET)
	@echo "Debug test build complete with symbols"

clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(TARGET) $(TEST_TARGET)

help:
	@echo "Targets disponíveis:"
	@echo "  make          - Compila o projeto metrônomo"
	@echo "  make run      - Compila e executa metrônomo (requer sudo e pigpiod)"
	@echo "  make test     - Compila e executa testes de periféricos (requer sudo e pigpiod)"
	@echo "  make debug    - Compila metrônomo com símbolos de debug"
	@echo "  make debug-test - Compila testes com símbolos de debug"
	@echo "  make clean    - Remove arquivos compilados"
	@echo "  make help     - Mostra esta mensagem"
	@echo ""
	@echo "Requisitos:"
	@echo "  sudo apt-get install pigpio"
	@echo "  sudo pigpiod (executar antes de make run/test)"

.PHONY: all run test debug debug-test clean help
