/* =============================================================
 * main.c - Metrônomo Eletrônico para Raspberry Pi 3
 * Alvo: Raspberry Pi 3 (Cortex-A53, ARM)
 *
 * Biblioteca: pigpio (PWM por hardware, melhor precisão)
 * Instalação: sudo apt-get install pigpio
 * Daemon: sudo pigpiod (executar antes de iniciar o programa)
 *
 * Periféricos utilizados (Kit Freenove FNK0054):
 *   - LED Red (GPIO 17): Pisca quando servo atinge 90°
 *   - Botão (GPIO 26): Controle de iniciar/parar
 *   - Buzzer (GPIO 12): Toca quando servo atinge 90°
 *   - Servo Motor (GPIO 18): Movimentação angular via PWM
 *
 * Funcionalidades:
 *   1. Metrônomo com 2 batidas por 3 segundos (40 BPM)
 *   2. Cada batida: 1500ms de rotação suave contínua
 *   3. LED e buzzer são ativados quando servo atinge ~90°
 *   4. Buzzer toca por 100ms a partir do ponto de 90°
 *   5. Servo mantém continuidade entre batidas (sem resets)
 *   6. Controle Start/Stop por botão responsivo
 *
 * Comportamento:
 *   Inicio: Servo posicionado em 90° (1500µs)
 *   Batida 1: Servo move suavemente de ângulo atual→180° em 1500ms
 *   Batida 2: Servo move suavemente de ângulo atual→0° em 1500ms
 *   LED+Buzzer: Ativam quando servo passa por ~90° em qualquer direção
 *
 * Compilação: gcc -Wall -Wextra -O2 -c main.c -o main.o
 *             gcc main.o -o metronomo -lpigpio -lm
 * ============================================================= */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pigpio.h>
#include <math.h>

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

/* ==================== PROTÓTIPOS DAS FUNÇÕES DO PIGPIO ==================== */

/* Não é necessário com pigpio - funções já estão em pigpio.h */

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
 * @brief Inicializa os pinos GPIO com pigpio
 */
static void gpio_init(void) {
    printf("Inicializando pigpio...\n");

    if (gpioInitialise() < 0) {
        printf("Erro: Falha ao inicializar pigpio\n");
        printf("Certifique-se de estar executando com privilégios de root\n");
        printf("e que o daemon pigpiod está rodando: sudo pigpiod\n");
        exit(1);
    }

    /* Configurar LED como saída digital (simples ON/OFF) */
    gpioSetMode(PIN_LED_RED, PI_OUTPUT);
    gpioWrite(PIN_LED_RED, 0);  /* LED começa desligado */

    /* Configurar botão como entrada com pull-up */
    gpioSetMode(PIN_BUTTON_START, PI_INPUT);
    gpioSetPullUpDown(PIN_BUTTON_START, PI_PUD_UP);

    /* Configurar buzzer como saída */
    gpioSetMode(PIN_BUZZER, PI_OUTPUT);
    gpioWrite(PIN_BUZZER, 0);

    /* Inicializar servo com PWM (50Hz para servo padrão) */
    /* pigpio controla servo via gpioServo() que já configura automaticamente */
    gpioServo(PIN_SERVO, 1500);  /* Servo começa em 90° (1500µs) */
    usleep(100000);  /* Aguardar 100ms */

    printf("pigpio inicializado com sucesso\n");
}

/**
 * @brief Inicializa o botão com debounce
 */
static void button_init(void) {
    button_start.pin = PIN_BUTTON_START;
    button_start.last_state = 1;  /* HIGH = 1 com pigpio */
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
    gpioWrite(PIN_LED_RED, 1);
}

/**
 * @brief Apaga o LED
 */
static void led_off(void) {
    gpioWrite(PIN_LED_RED, 0);
}

/**
 * @brief Aciona o buzzer
 */
static void buzzer_on(void) {
    gpioWrite(PIN_BUZZER, 1);
}

/**
 * @brief Desliga o buzzer
 */
static void buzzer_off(void) {
    gpioWrite(PIN_BUZZER, 0);
}

/**
 * @brief Move o servo para um ângulo específico (0-180°)
 * Converte ângulo para largura de pulso em microsegundos
 * 0° = 1000µs, 90° = 1500µs, 180° = 2000µs
 */
static void servo_set_angle(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    /* Converter ângulo para pulsewidth em microsegundos */
    unsigned int pulsewidth = 1000 + (angle * 1000 / 180);
    
    gpioServo(PIN_SERVO, pulsewidth);
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
        usleep(100000);  /* Aguardar 100ms para servo estabilizar */
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
    int current_state = gpioRead(btn->pin);
    unsigned long now = get_time_ms();

    /* Detectar transição de HIGH(1) para LOW(0) (botão pressionado) */
    if (current_state == 0 && btn->last_state == 1) {
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
    /* Usar o ângulo ATUAL do servo como ponto de partida */
    int start_angle = metronome.servo_angle;
    int end_angle = (beat_num == 0) ? 180 : 0;
    int direction = (end_angle > start_angle) ? 1 : -1;
    
    /* Calcular a distância total a percorrer */
    int total_distance = abs(end_angle - start_angle);
    
    int current_angle = start_angle;
    int previous_angle = start_angle;
    int buzzer_triggered = 0;
    unsigned long buzzer_end_time = 0;

    printf("Beat %d: Servo %d→%d (distância: %d°)\n", beat_num + 1, start_angle, end_angle, total_distance);
    
    /* Inicia o timing da batida */
    unsigned long beat_start = get_time_ms();
    unsigned long beat_end = beat_start + BEAT_INTERVAL_MS;

    while (get_time_ms() < beat_end) {
        button_process_all();
        if (metronome.state == STATE_STOPPED) {
            buzzer_off();
            led_off();
            return 0;
        }

        /* Calcular ângulo baseado no tempo decorrido */
        unsigned long elapsed = get_time_ms() - beat_start;
        int angle_distance = (int)((elapsed * total_distance) / BEAT_INTERVAL_MS);
        
        /* Limitar a distância ao total */
        if (angle_distance > total_distance) angle_distance = total_distance;
        
        current_angle = start_angle + (direction * angle_distance);
        
        /* Limitar ângulo aos limites [0, 180] */
        if (current_angle < 0) current_angle = 0;
        if (current_angle > 180) current_angle = 180;

        servo_set_angle(current_angle);

        /* Detectar quando passa por 90° (intervalo de tolerância) */
        if (!buzzer_triggered) {
            int prev_dist_to_90 = abs(previous_angle - 90);
            int curr_dist_to_90 = abs(current_angle - 90);
            
            /* Se passou por 90° (distância diminuiu e chegou perto), trigger */
            if (prev_dist_to_90 > curr_dist_to_90 && curr_dist_to_90 <= 2) {
                printf("  → Ângulo ~90° atingido! (atual=%d°) LED ON, Buzzer ON\n", current_angle);
                led_on();
                buzzer_on();
                buzzer_triggered = 1;
                buzzer_end_time = get_time_ms() + BUZZER_DURATION_MS;
            }
        }

        /* Desligar buzzer após 100ms */
        if (buzzer_triggered && get_time_ms() >= buzzer_end_time) {
            buzzer_off();
            led_off();
            buzzer_triggered = 0;
        }

        previous_angle = current_angle;
        usleep(10000);  /* 10ms em microsegundos */
    }

    /* Garantir que chegou no ângulo final */
    servo_set_angle(end_angle);
    
    /* AJUSTE: Aguardar servo estabilizar antes da próxima batida */
    /* Isso evita movimentos erráticos ao inverter a direção */
    usleep(100000);  /* 100ms em microsegundos */
    
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
        usleep(10000);  /* 10ms em microsegundos */
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
    usleep(100000);  /* Aguardar servo estabilizar */

    /* Finalizar pigpio */
    gpioTerminate();

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
    printf("  - Cada batida: 1500ms de movimento suave\n");
    printf("  - Servo: Movimento contínuo alternando entre extremos\n");
    printf("           Batida 1: atual→180°, Batida 2: atual→0°\n");
    printf("  - LED e Buzzer: Ativam quando servo passa por ~90°\n");
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
