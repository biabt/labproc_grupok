/* =============================================================
 * test_peripherals.c - Testes Individuais dos Periféricos
 * 
 * Biblioteca: pigpio (PWM por hardware)
 * Instalação: sudo apt-get install pigpio
 * Daemon: sudo pigpiod (executar antes de iniciar o programa)
 * 
 * Compile com: make test_periph
 * Execute com: sudo ./test_periph
 * 
 * Este programa permite testar cada periférico isoladamente
 * para verificar a correta conexão e funcionamento do hardware.
 * Usa a mesma configuração GPIO que main.c (BCM numbering).
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pigpio.h>

/* ==================== DEFINIÇÕES DE PINOS (BCM) ==================== */

/* Kit Freenove FNK0054 */
#define PIN_LED_RED         17      /* GPIO17 - LED Red */
#define PIN_BUTTON_START    26      /* GPIO26 - Start/Stop */
#define PIN_BUZZER          12      /* GPIO12 - Buzzer */
#define PIN_SERVO           18      /* GPIO18 - Servo Motor */

/* ==================== PROTÓTIPOS DO PIGPIO ==================== */

/* Funções já disponíveis em pigpio.h */

/* ==================== INICIALIZAÇÃO ==================== */

void init_gpio(void) {
    printf("Inicializando pigpio (BCM numbering)...\n");
    if (gpioInitialise() < 0) {
        printf("Erro ao inicializar pigpio\n");
        printf("Certifique-se de que: sudo pigpiod está rodando\n");
        printf("E você tem privilégios de root\n");
        exit(1);
    }

    /* LED como saída */
    gpioSetMode(PIN_LED_RED, PI_OUTPUT);

    /* Botão como entrada com pull-up */
    gpioSetMode(PIN_BUTTON_START, PI_INPUT);
    gpioSetPullUpDown(PIN_BUTTON_START, PI_PUD_UP);

    /* Buzzer como saída */
    gpioSetMode(PIN_BUZZER, PI_OUTPUT);

    /* Inicializar servo com posição central */
    gpioServo(PIN_SERVO, 1500);  /* 1500µs = 90° */

    printf("pigpio inicializado com sucesso\n");
}

/* ==================== TESTES ==================== */

void test_led(void) {
    printf("\n=== TESTE DE LED VERMELHO (GPIO 17) ===\n");

    printf("LED ligado por 2 segundos...\n");
    gpioWrite(PIN_LED_RED, 1);
    usleep(2000000);  /* 2 segundos em microsegundos */
    gpioWrite(PIN_LED_RED, 0);

    printf("Piscando LED 5 vezes...\n");
    for (int i = 0; i < 5; i++) {
        gpioWrite(PIN_LED_RED, 1);
        usleep(300000);  /* 300ms */
        gpioWrite(PIN_LED_RED, 0);
        usleep(300000);  /* 300ms */
    }

    printf("Teste de LED concluído\n");
}

void test_buzzer(void) {
    printf("\n=== TESTE DE BUZZER (GPIO 12) ===\n");

    printf("Gerando beep 1 (500ms)...\n");
    gpioWrite(PIN_BUZZER, 1);
    usleep(500000);  /* 500ms */
    gpioWrite(PIN_BUZZER, 0);
    usleep(300000);  /* 300ms */

    printf("Gerando beep 2 (300ms)...\n");
    gpioWrite(PIN_BUZZER, 1);
    usleep(300000);  /* 300ms */
    gpioWrite(PIN_BUZZER, 0);
    usleep(300000);  /* 300ms */

    printf("Gerando beep 3 (100ms)...\n");
    gpioWrite(PIN_BUZZER, 1);
    usleep(100000);  /* 100ms */
    gpioWrite(PIN_BUZZER, 0);

    printf("Teste de buzzer concluído\n");
}

void test_servo(void) {
    printf("\n=== TESTE DE SERVO MOTOR (GPIO 18) ===\n");
    printf("Usando pigpio com PWM por hardware\n");
    printf("Mapeamento: 0°=1000µs, 90°=1500µs, 180°=2000µs\n");

    printf("Movendo servo para posição mínima (0°, 1000µs)...\n");
    gpioServo(PIN_SERVO, 1000);
    usleep(1500000);  /* 1.5 segundos */

    printf("Movendo servo para posição central (90°, 1500µs)...\n");
    gpioServo(PIN_SERVO, 1500);
    usleep(1500000);  /* 1.5 segundos */

    printf("Movendo servo para posição máxima (180°, 2000µs)...\n");
    gpioServo(PIN_SERVO, 2000);
    usleep(1500000);  /* 1.5 segundos */

    printf("Retornando servo para centro (90°, 1500µs)...\n");
    gpioServo(PIN_SERVO, 1500);
    usleep(1000000);  /* 1 segundo */

    printf("Teste de servo concluído\n");
}

void test_buttons(void) {
    printf("\n=== TESTE DE BOTÃO (GPIO 25) ===\n");
    printf("Pressione o botão START múltiplas vezes (30 segundos de timeout)...\n");

    unsigned long start_time = gpioTick();
    int button_press_count = 0;

    while ((gpioTick() - start_time) < 30000000) {  /* 30 segundos em microsegundos */
        if (gpioRead(PIN_BUTTON_START) == 0) {  /* LOW = pressionado */
            button_press_count++;
            printf("Botão START pressionado! (contagem: %d)\n", button_press_count);
            usleep(500000);  /* 500ms */
            /* Aguardar liberação do botão */
            while (gpioRead(PIN_BUTTON_START) == 0) usleep(10000);  /* 10ms */
            usleep(100000);  /* 100ms */
        }
        usleep(10000);  /* 10ms */
    }

    printf("Teste de botão concluído (%d vezes pressionado)\n", button_press_count);
}

void test_all_peripherals(void) {
    printf("\n=== TESTE COMPLETO ===\n");
    printf("Simulando 3 batidas de metrônomo...\n");

    for (int i = 0; i < 3; i++) {
        printf("\nBatida %d\n", i + 1);

        /* LED pisca */
        gpioWrite(PIN_LED_RED, 1);

        /* Buzzer toca */
        gpioWrite(PIN_BUZZER, 1);

        /* Servo move para máximo */
        gpioServo(PIN_SERVO, 2000);
        usleep(50000);  /* 50ms */
        
        /* Servo volta para centro */
        gpioServo(PIN_SERVO, 1500);
        usleep(50000);  /* 50ms */

        /* Desligar */
        gpioWrite(PIN_LED_RED, 0);
        gpioWrite(PIN_BUZZER, 0);

        usleep(500000);  /* 500ms */
    }

    printf("Teste completo finalizado\n");
}

void cleanup(void) {
    printf("\nLimpando recursos...\n");
    gpioWrite(PIN_LED_RED, 0);
    gpioWrite(PIN_BUZZER, 0);
    gpioServo(PIN_SERVO, 1500);  /* Centro */
    usleep(100000);  /* 100ms */
    gpioTerminate();
    printf("Limpeza concluída\n");
}

/* ==================== MENU ==================== */

void print_menu(void) {
    printf("\n========================================\n");
    printf("  Teste de Periféricos - Metrônomo\n");
    printf("  Kit Freenove FNK0054 (BCM GPIO)\n");
    printf("========================================\n");
    printf("1 - Testar LED Vermelho (GPIO 17)\n");
    printf("2 - Testar Buzzer (GPIO 12)\n");
    printf("3 - Testar Servo Motor (GPIO 18)\n");
    printf("4 - Testar Botão START (GPIO 25)\n");
    printf("5 - Teste Completo (Simulação)\n");
    printf("0 - Sair\n");
    printf("========================================\n");
    printf("Escolha uma opção: ");
}

/* ==================== MAIN ==================== */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("\n========================================\n");
    printf("  Teste de Periféricos - Metrônomo\n");
    printf("  Raspberry Pi 3 + pigpio\n");
    printf("  (Requisitos: sudo pigpiod)\n");
    printf("========================================\n\n");

    init_gpio();

    int choice;
    while (1) {
        print_menu();
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                test_led();
                break;
            case 2:
                test_buzzer();
                break;
            case 3:
                test_servo();
                break;
            case 4:
                test_buttons();
                break;
            case 5:
                test_all_peripherals();
                break;
            case 0:
                printf("Encerrando...\n");
                cleanup();
                return 0;
            default:
                printf("Opção inválida\n");
        }
    }

    return 0;
}
