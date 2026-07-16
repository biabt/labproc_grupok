/* =============================================================
 * main.c - Metrônomo Eletrônico para Raspberry Pi 3
 * Alvo: Raspberry Pi 3 (Cortex-A53, ARM)
 *
 * Periféricos utilizados (Kit Freenove FNK0054):
 *   - LED Red (GPIO 17): Pisca quando servo atinge 90° (Blink.c)
 *   - Botão (GPIO 26): Controle de iniciar/parar
 *   - Buzzer (GPIO 12): Toca quando servo atinge 90° (Doorbell.c)
 *   - Servo Motor (GPIO 18): Movimentação angular suave (Sweep.c)
 *
 * Funcionalidades:
 *   1. Metrônomo com 2 batidas por 3 segundos (40 BPM)
 *   2. Cada batida: 1500ms de rotação suave do servo (0→180 ou 180→0)
 *   3. LED e buzzer são ativados quando servo atinge 90°
 *   4. Buzzer toca por 100ms a partir do ponto de 90°
 *   5. Servo com rotação gradual suave (1° a cada ~8.3ms)
 *   6. Controle Start/Stop por botão responsivo
 *
 * Comportamento:
 *   Batida 1: Servo 0→180° em 1500ms, LED+Buzzer ativam em 90°
 *   Batida 2: Servo 180→0° em 1500ms, LED+Buzzer ativam em 90°
 * ============================================================= */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <wiringPi.h>

/* ==================== DEFINIÇÕES DE HARDWARE ==================== */

/* Pinos GPIO (usando numeração BCM do Freenove) */
#define PIN_LED_RED         17      /* GPIO17 - LED Red (Blink.c) */
#define PIN_BUTTON_START    26      /* GPIO26 - Start/Stop (com pull-up) */
#define PIN_BUZZER          12      /* GPIO12 - Buzzer (Doorbell.c) */
#define PIN_SERVO           18      /* GPIO18 - Servo Motor (Sweep.c) */

/* Servo angular constants */
#define SERVO_ANGLE_MIN     0       /* Ângulo mínimo: 0 graus */
#define SERVO_ANGLE_MAX     180     /* Ângulo máximo: 180 graus */

/* Timing constants */
#define BEAT_INTERVAL_MS    1500    /* Intervalo entre batidas: 1500ms */
#define BUZZER_DURATION_MS  100     /* Duração do buzzer: 100ms */

/* Debounce */
#define DEBOUNCE_MS         200      /* Tempo de debounce em ms */

/* ==================== ESTRUTURAS DE DADOS ==================== */

/* Estados do metrônomo */
typedef enum {
    STATE_STOPPED,
    STATE_RUNNING
} metronome_state_t;

/* Estrutura do estado do metrônomo */
typedef struct {
    metronome_state_t state;
    int servo_angle;                /* Ângulo atual do servo */
} metronome_t;

/* Estrutura para debounce de botão */
typedef struct {
    int pin;
    int last_state;
    unsigned long last_press_time;
} button_t;

/* ==================== VARIÁVEIS GLOBAIS ==================== */

static metronome_t metronome;
static button_t button_start;
static volatile int should_exit = 0;

/* ==================== PROTÓTIPOS DAS FUNÇÕES DO FREENOVE ==================== */

/* Do Sweep.c */
extern void servoInit(int pin);
extern void servoWrite(int pin, int angle);
extern void servoWriteMS(int pin, int ms);

/* ==================== FUNÇÕES AUXILIARES DE TEMPO ==================== */

/**
 * @brief Obtém o tempo atual em milissegundos
 * @return Tempo em ms desde um ponto de referência
 */
static unsigned long get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/**
 * @brief Calcula a diferença de tempo em ms
 * @param start Tempo de início
 * @param end Tempo de fim
 * @return Diferença em ms (trata overflow)
 */
static unsigned long time_diff_ms(unsigned long start, unsigned long end) {
    if (end >= start) {
        return end - start;
    } else {
        /* Trata overflow de unsigned long */
        return (0xFFFFFFFF - start) + end;
    }
}

/* ==================== FUNÇÕES DE INICIALIZAÇÃO ==================== */

/**
 * @brief Inicializa os pinos GPIO
 */
static void gpio_init(void) {
    printf("Inicializando GPIO com wiringPi...\n");

    if (wiringPiSetupGpio() == -1) {
        printf("Erro: Falha ao inicializar wiringPi\n");
        printf("Certifique-se de estar executando com privilégios de root\n");
        exit(1);
    }

    /* Configurar LED como saída digital (simples ON/OFF) */
    pinMode(PIN_LED_RED, OUTPUT);
    digitalWrite(PIN_LED_RED, LOW);  /* LED começa desligado */

    /* Configurar botão como entrada com pull-up */
    pinMode(PIN_BUTTON_START, INPUT);
    pullUpDnControl(PIN_BUTTON_START, PUD_UP);

    /* Configurar buzzer como saída */
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    /* Inicializar servo (do Freenove Sweep.c) */
    servoInit(PIN_SERVO);
    servoWrite(PIN_SERVO, 0);  /* Servo começa em 0° */

    printf("GPIO inicializado com sucesso\n");
}

/**
 * @brief Inicializa o botão com debounce
 */
static void button_init(void) {
    button_start.pin = PIN_BUTTON_START;
    button_start.last_state = HIGH;
    button_start.last_press_time = 0;

    printf("Botão inicializado\n");
}

/**
 * @brief Inicializa o metrônomo
 */
static void metronome_init(void) {
    metronome.state = STATE_STOPPED;
    metronome.servo_angle = 0;      /* Servo em 0° */

    printf("Metrônomo inicializado: 40 BPM (2 batidas a cada 3 segundos)\n");
}

/* ==================== FUNÇÕES DE CONTROLE DE PERIFÉRICOS ==================== */

/**
 * @brief Acende o LED
 */
static void led_on(void) {
    digitalWrite(PIN_LED_RED, HIGH);
}

/**
 * @brief Apaga o LED
 */
static void led_off(void) {
    digitalWrite(PIN_LED_RED, LOW);
}

/**
 * @brief Aciona o buzzer
 */
static void buzzer_on(void) {
    digitalWrite(PIN_BUZZER, HIGH);
}

/**
 * @brief Desliga o buzzer
 */
static void buzzer_off(void) {
    digitalWrite(PIN_BUZZER, LOW);
}

/**
 * @brief Move o servo para um ângulo específico (0-180°)
 */
static void servo_set_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    servoWrite(PIN_SERVO, angle);
    metronome.servo_angle = angle;
}

/* ==================== FUNÇÕES DE CONTROLE DO METRÔNOMO ==================== */

/**
 * @brief Inicia o metrônomo
 */
static void metronome_start(void) {
    if (metronome.state == STATE_STOPPED) {
        metronome.state = STATE_RUNNING;
        servo_set_angle(0);
        led_off();
        printf("Metrônomo iniciado\n");
    }
}

/**
 * @brief Para o metrônomo
 */
static void metronome_stop(void) {
    if (metronome.state == STATE_RUNNING) {
        metronome.state = STATE_STOPPED;
        buzzer_off();
        led_off();
        servo_set_angle(0);
        printf("Metrônomo parado\n");
    }
}

/**
 * @brief Alterna entre start/stop
 */
static void metronome_toggle(void) {
    if (metronome.state == STATE_STOPPED) {
        metronome_start();
    } else {
        metronome_stop();
    }
}

/* ==================== FUNÇÕES DE BOTÕES ==================== */

/**
 * @brief Processa botão com debounce
 * @param btn Ponteiro para estrutura do botão
 * @param pressed Função a executar se botão pressionado
 */
static void button_process(button_t *btn, void (*pressed)(void)) {
    int current_state = digitalRead(btn->pin);
    unsigned long now = get_time_ms();

    /* Detectar transição de HIGH para LOW (botão pressionado) */
    if (current_state == LOW && btn->last_state == HIGH) {
        /* Verificar debounce */
        if (time_diff_ms(btn->last_press_time, now) > DEBOUNCE_MS) {
            printf("Botão no pino %d pressionado\n", btn->pin);
            if (pressed) {
                pressed();
            }
            btn->last_press_time = now;
        }
    }

    btn->last_state = current_state;
}

/**
 * @brief Processa o botão
 */
static void button_process_all(void) {
    button_process(&button_start, metronome_toggle);
}

/* ==================== LOOP PRINCIPAL ==================== */

/**
 * @brief Executa uma batida completa do metrônomo
 * @param beat_num Número da batida (0 ou 1)
 * @return 1 se completado, 0 se interrompido
 */
static int execute_beat(int beat_num) {
    unsigned long beat_start = get_time_ms();
    unsigned long beat_end = beat_start + BEAT_INTERVAL_MS;
    
    int start_angle = (beat_num == 0) ? 0 : 180;
    int end_angle = (beat_num == 0) ? 180 : 0;
    int direction = (end_angle > start_angle) ? 1 : -1;
    
    int current_angle = start_angle;
    int previous_angle = start_angle - 1;
    int buzzer_triggered = 0;
    unsigned long buzzer_end_time = 0;

    printf("Beat %d: Servo %d→%d\n", beat_num + 1, start_angle, end_angle);

    while (get_time_ms() < beat_end) {
        button_process_all();
        if (metronome.state == STATE_STOPPED) {
            buzzer_off();
            led_off();
            return 0;
        }

        /* Calcular ângulo baseado no tempo decorrido */
        unsigned long elapsed = get_time_ms() - beat_start;
        current_angle = start_angle + (direction * (int)((elapsed * 180) / BEAT_INTERVAL_MS));
        
        /* Limitar ângulo aos limites */
        if (direction > 0) {
            if (current_angle > end_angle) current_angle = end_angle;
        } else {
            if (current_angle < end_angle) current_angle = end_angle;
        }

        servo_set_angle(current_angle);

        /* Detectar quando atinge 90° (transição suave) */
        if (!buzzer_triggered && current_angle == 90 && previous_angle != 90) {
            printf("  → Ângulo 90° atingido! LED ON, Buzzer ON\n");
            led_on();
            buzzer_on();
            buzzer_triggered = 1;
            buzzer_end_time = get_time_ms() + BUZZER_DURATION_MS;
        }

        /* Desligar buzzer após 100ms */
        if (buzzer_triggered && get_time_ms() >= buzzer_end_time) {
            buzzer_off();
            led_off();
            buzzer_triggered = 0;
        }

        previous_angle = current_angle;
        delay(1);
    }

    /* Garantir que LED e buzzer estão desligados ao final */
    buzzer_off();
    led_off();

    return 1;
}

/**
 * @brief Loop principal do metrônomo
 */
static void metronome_loop(void) {
    if (metronome.state == STATE_RUNNING) {
        /* Batida 0: Servo 0→180, ativa quando atinge 90° */
        if (execute_beat(0) == 0) {
            return;  /* Interrompida pelo botão */
        }

        /* Batida 1: Servo 180→0, ativa quando atinge 90° */
        if (execute_beat(1) == 0) {
            return;  /* Interrompida pelo botão */
        }
    } else {
        /* Verificar botão mesmo quando parado */
        button_process_all();
        delay(10);
    }
}

/* ==================== LIMPEZA E SIGNAL HANDLERS ==================== */

/**
 * @brief Handler para sinais de interrupção (Ctrl+C)
 */
static void signal_handler(int sig) {
    (void)sig;
    printf("\nEncerrando metrônomo...\n");
    should_exit = 1;
}

/**
 * @brief Limpa recursos ao encerrar
 */
static void cleanup(void) {
    printf("Limpando recursos...\n");

    /* Parar metrônomo */
    metronome_stop();

    /* Desligar todos os periféricos */
    led_off();
    buzzer_off();
    servo_set_angle(0);

    printf("Metrônomo encerrado com sucesso\n");
}

/* ==================== FUNÇÃO MAIN ==================== */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("\n========================================\n");
    printf("  Metrônomo Eletrônico - Raspberry Pi 3\n");
    printf("  Kit Freenove FNK0054\n");
    printf("========================================\n\n");

    /* Registrar handler para Ctrl+C */
    signal(SIGINT, signal_handler);

    /* Inicializar hardware */
    printf("Inicializando hardware...\n");
    gpio_init();
    button_init();
    metronome_init();

    printf("\n========================================\n");
    printf("  Instruções de Uso\n");
    printf("========================================\n");
    printf("Botões:\n");
    printf("  - Botão (GPIO 26): Inicia/Para o metrônomo\n");
    printf("\nComportamento:\n");
    printf("  - BPM: 40 (2 batidas por 3 segundos)\n");
    printf("  - Cada batida: 1500ms\n");
    printf("  - Servo: Rotação suave 0→180 ou 180→0 durante 1500ms\n");
    printf("  - LED e Buzzer: Ativam quando servo atinge 90°\n");
    printf("  - Buzzer: Soa por 100ms a partir dos 90°\n");
    printf("\nPressione Ctrl+C para sair\n");
    printf("Pressione o botão para iniciar/parar\n\n");

    /* Loop principal */
    while (!should_exit) {
        metronome_loop();
    }

    /* Limpeza */
    cleanup();

    return 0;
}
