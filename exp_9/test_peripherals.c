/* =============================================================
 * test_peripherals.c - Testes Individuais dos Periféricos
 * 
 * Integração com Kit Freenove FNK0054
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
#include <wiringPi.h>

/* ==================== DEFINIÇÕES DE PINOS (BCM) ==================== */

/* Kit Freenove FNK0054 */
#define PIN_LED_RED         17      /* GPIO17 - LED Red (Blink.c) */
#define PIN_BUTTON_START    26      /* GPIO26 - Start/Stop */
#define PIN_BUZZER          12      /* GPIO12 - Buzzer (Doorbell.c) */
#define PIN_SERVO           18      /* GPIO18 - Servo Motor (Sweep.c) */

/* Servo constants (do Sweep.c do Freenove) */
#define OFFSET_MS           3       /* Define the unit of servo pulse offset: 0.1ms */
#define SERVO_MIN_MS        (5 + OFFSET_MS)     /* minimum angle of servo */
#define SERVO_MAX_MS        (25 + OFFSET_MS)    /* maximum angle of servo */
#define SERVO_MID_MS        ((SERVO_MIN_MS + SERVO_MAX_MS) / 2)  /* middle position */

/* ==================== PROTÓTIPOS DO FREENOVE ==================== */

/* Do Sweep.c */
extern void servoInit(int pin);
extern void servoWrite(int pin, int angle);
extern void servoWriteMS(int pin, int ms);

/* ==================== INICIALIZAÇÃO ==================== */

void init_gpio(void) {
    printf("Inicializando GPIO com wiringPi (BCM numbering)...\n");
    if (wiringPiSetupGpio() == -1) {
        printf("Erro ao inicializar wiringPi\n");
        printf("Certifique-se de estar executando com privilégios de root\n");
        exit(1);
    }

    /* LED como saída (Blink.c style) */
    pinMode(PIN_LED_RED, OUTPUT);

    /* Botão como entrada com pull-up (Doorbell.c style) */
    pinMode(PIN_BUTTON_START, INPUT);
    pullUpDnControl(PIN_BUTTON_START, PUD_UP);

    /* Buzzer como saída (Doorbell.c style) */
    pinMode(PIN_BUZZER, OUTPUT);

    /* Inicializar servo (do Freenove Sweep.c) */
    servoInit(PIN_SERVO);

    printf("GPIO inicializado com sucesso\n");
}

/* ==================== TESTES ==================== */

void test_led(void) {
    printf("\n=== TESTE DE LED VERMELHO (GPIO 17) ===\n");

    printf("LED ligado por 2 segundos...\n");
    digitalWrite(PIN_LED_RED, HIGH);
    delay(2000);
    digitalWrite(PIN_LED_RED, LOW);

    printf("Piscando LED 5 vezes...\n");
    for (int i = 0; i < 5; i++) {
        digitalWrite(PIN_LED_RED, HIGH);
        delay(300);
        digitalWrite(PIN_LED_RED, LOW);
        delay(300);
    }

    printf("Teste de LED concluído\n");
}

void test_buzzer(void) {
    printf("\n=== TESTE DE BUZZER (GPIO 12) ===\n");

    printf("Gerando beep 1 (500ms)...\n");
    digitalWrite(PIN_BUZZER, HIGH);
    delay(500);
    digitalWrite(PIN_BUZZER, LOW);
    delay(300);

    printf("Gerando beep 2 (300ms)...\n");
    digitalWrite(PIN_BUZZER, HIGH);
    delay(300);
    digitalWrite(PIN_BUZZER, LOW);
    delay(300);

    printf("Gerando beep 3 (100ms)...\n");
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);

    printf("Teste de buzzer concluído\n");
}

void test_servo(void) {
    printf("\n=== TESTE DE SERVO MOTOR (GPIO 18) ===\n");
    printf("Usando Sweep.c do Freenove Kit\n");

    printf("Movendo servo para posição mínima (0°)...\n");
    servoWriteMS(PIN_SERVO, SERVO_MIN_MS);
    delay(1500);

    printf("Movendo servo para posição central (90°)...\n");
    servoWriteMS(PIN_SERVO, SERVO_MID_MS);
    delay(1500);

    printf("Movendo servo para posição máxima (180°)...\n");
    servoWriteMS(PIN_SERVO, SERVO_MAX_MS);
    delay(1500);

    printf("Retornando servo para centro...\n");
    servoWriteMS(PIN_SERVO, SERVO_MID_MS);
    delay(1000);

    printf("Teste de servo concluído\n");
}

void test_buttons(void) {
    printf("\n=== TESTE DE BOTÃO (GPIO 25) ===\n");
    printf("Pressione o botão START múltiplas vezes (30 segundos de timeout)...\n");

    unsigned long start_time = millis();
    int button_press_count = 0;

    while ((millis() - start_time) < 30000) {
        if (digitalRead(PIN_BUTTON_START) == LOW) {
            button_press_count++;
            printf("Botão START pressionado! (contagem: %d)\n", button_press_count);
            delay(500);
            /* Aguardar liberação do botão */
            while (digitalRead(PIN_BUTTON_START) == LOW) delay(10);
            delay(100);
        }
        delay(10);
    }

    printf("Teste de botão concluído (%d vezes pressionado)\n", button_press_count);
}

void test_all_peripherals(void) {
    printf("\n=== TESTE COMPLETO ===\n");
    printf("Simulando 3 batidas de metrônomo...\n");

    for (int i = 0; i < 3; i++) {
        printf("\nBatida %d\n", i + 1);

        /* LED pisca */
        digitalWrite(PIN_LED_RED, HIGH);

        /* Buzzer toca */
        digitalWrite(PIN_BUZZER, HIGH);

        /* Servo move para máximo */
        servoWriteMS(PIN_SERVO, SERVO_MAX_MS);
        delay(50);
        
        /* Servo volta para centro */
        servoWriteMS(PIN_SERVO, SERVO_MID_MS);
        delay(50);

        /* Desligar */
        digitalWrite(PIN_LED_RED, LOW);
        digitalWrite(PIN_BUZZER, LOW);

        delay(500);
    }

    printf("Teste completo finalizado\n");
}

void cleanup(void) {
    printf("\nLimpando recursos...\n");
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    servoWriteMS(PIN_SERVO, SERVO_MID_MS);
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
    printf("  Raspberry Pi 3 + Kit Freenove\n");
    printf("  (Integração com Sweep.c)\n");
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
