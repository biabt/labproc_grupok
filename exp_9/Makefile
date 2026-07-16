# Makefile - Metrônomo Eletrônico com Kit Freenove (Raspberry Pi 3 / Cortex-A53)
#
# Compilar e executar diretamente no Raspberry Pi com gcc nativo
# Integra componentes do Freenove FNK0054:
#   - LED (Blink.c)
#   - Servo Motor (Sweep.c)
#   - Buzzer (Doorbell.c)

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pedantic
LDFLAGS = -lwiringPi -lm

TARGET  = metronomo
DESAFIO_TARGET = desafio
TEST_TARGET = test_periph

# Paths do Freenove Kit
FREENOVE_KIT_SERVO = $(HOME)/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/13_1_Sweep/Sweep.c
FREENOVE_KIT_LED = $(HOME)/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/1_Blink/Blink.c
FREENOVE_KIT_BUZZER = $(HOME)/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/6_1_Doorbell/Doorbell.c

SOURCES = main.c
DESAFIO_SOURCES = desafio.c
TEST_SOURCES = test_peripherals.c
FREENOVE_SOURCES = $(FREENOVE_KIT_SERVO) $(FREENOVE_KIT_LED) $(FREENOVE_KIT_BUZZER)

OBJECTS = $(SOURCES:.c=.o)
DESAFIO_OBJECTS = $(DESAFIO_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)
FREENOVE_OBJECTS = Sweep.o Blink.o Doorbell.o

all: $(TARGET) $(DESAFIO_TARGET)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

desafio.o: desafio.c
	$(CC) $(CFLAGS) -c desafio.c -o desafio.o

Sweep.o: $(FREENOVE_KIT_SERVO)
	@echo "Processando Sweep.c (removendo main)..."
	@sed 's/^int main(/int sweep_main_disabled(/' $(FREENOVE_KIT_SERVO) > temp_sweep.c
	$(CC) $(CFLAGS) -c temp_sweep.c -o Sweep.o
	@rm -f temp_sweep.c
	@echo "Sweep.o gerado com sucesso"

Blink.o: $(FREENOVE_KIT_LED)
	@echo "Processando Blink.c (removendo main)..."
	@sed 's/^void main(/void blink_main_disabled(/' $(FREENOVE_KIT_LED) > temp_blink.c
	$(CC) $(CFLAGS) -c temp_blink.c -o Blink.o
	@rm -f temp_blink.c
	@echo "Blink.o gerado com sucesso"

Doorbell.o: $(FREENOVE_KIT_BUZZER)
	@echo "Processando Doorbell.c (removendo main)..."
	@sed 's/^void main(/void doorbell_main_disabled(/' $(FREENOVE_KIT_BUZZER) > temp_doorbell.c
	$(CC) $(CFLAGS) -c temp_doorbell.c -o Doorbell.o
	@rm -f temp_doorbell.c
	@echo "Doorbell.o gerado com sucesso"

$(TARGET): $(OBJECTS) $(FREENOVE_OBJECTS)
	$(CC) $(OBJECTS) $(FREENOVE_OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "================================"
	@echo "Build successful: ./$(TARGET)"
	@echo "================================"

$(DESAFIO_TARGET): $(DESAFIO_OBJECTS) $(FREENOVE_OBJECTS)
	$(CC) $(DESAFIO_OBJECTS) $(FREENOVE_OBJECTS) -o $(DESAFIO_TARGET) $(LDFLAGS)
	@echo "================================"
	@echo "Desafio build successful: ./$(DESAFIO_TARGET)"
	@echo "================================"

test_periph.o: test_peripherals.c
	$(CC) $(CFLAGS) -c test_peripherals.c -o test_periph.o

$(TEST_TARGET): $(TEST_OBJECTS) $(FREENOVE_OBJECTS)
	$(CC) $(TEST_OBJECTS) $(FREENOVE_OBJECTS) -o $(TEST_TARGET) $(LDFLAGS)
	@echo "================================"
	@echo "Test build successful: ./$(TEST_TARGET)"
	@echo "================================"

run: $(TARGET)
	@echo "Iniciando metrônomo (requer privilégios de root)..."
	sudo ./$(TARGET)

run-desafio: $(DESAFIO_TARGET)
	@echo "Iniciando desafio (requer privilégios de root)..."
	sudo ./$(DESAFIO_TARGET)

test: $(TEST_TARGET)
	@echo "Iniciando testes de periféricos (requer privilégios de root)..."
	sudo ./$(TEST_TARGET)

debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@echo "Debug build complete with symbols"

debug-test: CFLAGS += -g -DDEBUG
debug-test: clean $(TEST_TARGET)
	@echo "Debug test build complete with symbols"

clean:
	rm -f $(OBJECTS) $(DESAFIO_OBJECTS) $(TEST_OBJECTS) $(FREENOVE_OBJECTS) $(TARGET) $(DESAFIO_TARGET) $(TEST_TARGET)

help:
	@echo "Targets disponíveis:"
	@echo "  make          - Compila o projeto metrônomo e desafio"
	@echo "  make run      - Compila e executa metrônomo (requer sudo)"
	@echo "  make run-desafio - Compila e executa desafio com controle de velocidade (requer sudo)"
	@echo "  make test     - Compila e executa testes de periféricos (requer sudo)"
	@echo "  make debug    - Compila metrônomo com símbolos de debug"
	@echo "  make debug-test - Compila testes com símbolos de debug"
	@echo "  make clean    - Remove arquivos compilados"
	@echo "  make help     - Mostra esta mensagem"

.PHONY: all run test debug debug-test clean help
