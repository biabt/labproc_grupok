/* =============================================================
 * desafio.c - Metrônomo Eletrônico com Controle de Velocidade
 * Alvo: Raspberry Pi 3 (Cortex-A53, ARM)
 *
 * Periféricos utilizados (Kit Freenove FNK0054):
 *   - LED Red (GPIO 17): Pisca quando servo atinge 90°
 *   - Botão (GPIO 26): Controle de iniciar/parar
 *   - Botão (GPIO 21): Controle de velocidade (decrementar BPM)
 *   - Buzzer (GPIO 12): Toca quando servo atinge 90°
 *   - Servo Motor (GPIO 18): Movimentação angular suave
 *
 * Funcionalidades:
 *   1. Metrônomo com velocidade ajustável (3000ms até 500ms)
 *   2. Velocidades: 3000, 2500, 2000, 1500, 1000, 500ms (resets para 3000ms)
 *   3. Botão GPIO 21 diminui em 500ms a cada pressão
 *   4. Cada batida: rotação suave contínua
 *   5. LED e buzzer são ativados quando servo atinge ~90°
 *   6. Servo mantém continuidade entre batidas (sem resets)
 *   7. Controle Start/Stop por botão GPIO 26 responsivo
 *
 * Comportamento:
 *   Inicio: Velocidade inicial 3000ms (20 BPM), servo em 0°
 *   Pressionar GPIO 21: Diminui 500ms (máximo de 6 níveis até 500ms)
 *   Ao atingir 500ms e pressionar GPIO 21: Reseta para 3000ms
 *   Batida 1: Servo atual→180° no tempo configurado
 *   Batida 2: Servo atual→0° no tempo configurado
 *   LED+Buzzer: Ativam quando servo passa por ~90° em qualquer direção
 * ============================================================= */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <math.h>

/* ==================== DEFINIÇÕES DE HARDWARE ==================== */

/* Pinos GPIO (usando numeração BCM) */
#define PIN_LED_RED         17      /* GPIO17 - LED Red */
#define PIN_BUTTON_START    26      /* GPIO26 - Start/Stop (com pull-up) */
#define PIN_BUTTON_SPEED    21      /* GPIO21 - Controle de velocidade (com pull-up) */
#define PIN_BUZZER          12      /* GPIO12 - Buzzer */
#define PIN_SERVO           18      /* GPIO18 - Servo Motor */

/* Servo angular constants */
#define SERVO_ANGLE_MIN     0       /* Ângulo mínimo: 0 graus */
#define SERVO_ANGLE_MAX     180     /* Ângulo máximo: 180 graus */

/* Timing constants */
#define BEAT_INTERVAL_MIN   500     /* Intervalo mínimo: 500ms */
#define BEAT_INTERVAL_MAX   3000    /* Intervalo máximo: 3000ms */
#define BEAT_INTERVAL_STEP  500     /* Passo de mudança: 500ms */
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
static button_t button_speed;
static unsigned long beat_interval_ms = BEAT_INTERVAL_MAX;  /* 3000ms inicial */
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

    /* Configurar botão START como entrada com pull-up */
    pinMode(PIN_BUTTON_START, INPUT);
    pullUpDnControl(PIN_BUTTON_START, PUD_UP);

    /* Configurar botão SPEED como entrada com pull-up */
    pinMode(PIN_BUTTON_SPEED, INPUT);
    pullUpDnControl(PIN_BUTTON_SPEED, PUD_UP);

    /* Configurar buzzer como saída */
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    /* Inicializar servo (do Freenove Sweep.c) */
    servoInit(PIN_SERVO);
    servoWrite(PIN_SERVO, 0);  /* Servo começa em 0° */

    printf("GPIO inicializado com sucesso\n");
}

/**
 * @brief Inicializa os botões com debounce
 */
static void button_init(void) {
    button_start.pin = PIN_BUTTON_START;
    button_start.last_state = HIGH;
    button_start.last_press_time = 0;

    button_speed.pin = PIN_BUTTON_SPEED;
    button_speed.last_state = HIGH;
    button_speed.last_press_time = 0;

    printf("Botões inicializados\n");
}

/**
 * @brief Inicializa o metrônomo
 */
static void metronome_init(void) {
    metronome.state = STATE_STOPPED;
    metronome.servo_angle = 0;      /* Servo em 0° */

    printf("Metrônomo inicializado: Velocidade inicial 3000ms\n");
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

/* ==================== FUNÇÕES DE CONTROLE DE VELOCIDADE ==================== */

/**
 * @brief Diminui a velocidade do metrônomo (aumenta beat_interval_ms)
 * Valores: 3000 → 2500 → 2000 → 1500 → 1000 → 500 → 3000 (reset)
 */
static void decrease_speed(void) {
    beat_interval_ms -= BEAT_INTERVAL_STEP;
    
    if (beat_interval_ms < BEAT_INTERVAL_MIN) {
        beat_interval_ms = BEAT_INTERVAL_MAX;
        printf("Velocidade resetada para 3000ms (20 BPM)\n");
    } else {
        unsigned long bpm = (60000 / beat_interval_ms) * 2;  /* 2 batidas por ciclo */
        printf("Velocidade alterada para %lums (%lu BPM)\n", beat_interval_ms, bpm);
    }
}

/**
 * @brief Exibe a velocidade atual
 */
static void display_current_speed(void) {
    unsigned long bpm = (60000 / beat_interval_ms) * 2;
    printf("Velocidade atual: %lums (%lu BPM)\n", beat_interval_ms, bpm);
}

/* ==================== FUNÇÕES DE CONTROLE DO METRÔNOMO ==================== */

/**
 * @brief Inicia o metrônomo
 */
static void metronome_start(void) {
    if (metronome.state == STATE_STOPPED) {
        metronome.state = STATE_RUNNING;
        servo_set_angle(0);
        delay(100);  /* Aguardar servo estabilizar na posição inicial */
        led_off();
        display_current_speed();
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
 * @brief Processa todos os botões
 */
static void button_process_all(void) {
    button_process(&button_start, metronome_toggle);
    button_process(&button_speed, decrease_speed);
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

    printf("Beat %d: Servo %d→%d (distância: %d°, intervalo: %lums)\n", 
           beat_num + 1, start_angle, end_angle, total_distance, beat_interval_ms);
    
    /* Inicia o timing da batida */
    unsigned long beat_start = get_time_ms();
    unsigned long beat_end = beat_start + beat_interval_ms;

    while (get_time_ms() < beat_end) {
        button_process_all();
        if (metronome.state == STATE_STOPPED) {
            buzzer_off();
            led_off();
            return 0;
        }

        /* Calcular ângulo baseado no tempo decorrido */
        unsigned long elapsed = get_time_ms() - beat_start;
        int angle_distance = (int)((elapsed * total_distance) / beat_interval_ms);
        
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
        delay(10);
    }

    /* Garantir que chegou no ângulo final */
    servo_set_angle(end_angle);
    
    /* Aguardar servo estabilizar antes da próxima batida */
    delay(100);
    
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
        /* Batida 0: Servo atual→180° */
        if (execute_beat(0) == 0) {
            return;  /* Interrompida pelo botão */
        }

        /* Batida 1: Servo atual→0° */
        if (execute_beat(1) == 0) {
            return;  /* Interrompida pelo botão */
        }
    } else {
        /* Verificar botões mesmo quando parado */
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
    printf("  Metrônomo Eletrônico - DESAFIO\n");
    printf("  Controle de Velocidade (GPIO 21)\n");
    printf("  Raspberry Pi 3 + Kit Freenove\n");
    printf("========================================\n\n");

    /* Registrar handler para Ctrl+C */
    signal(SIGINT, signal_handler);

    /* Inicializar hardware */
    printf("Inicializando hardware...\n");
    gpio_init();
    button_init();
    metronome_init();

    printf("\n========================================\n");
    printf("  Instruções de Uso - DESAFIO\n");
    printf("========================================\n");
    printf("Botões:\n");
    printf("  - GPIO 26 (START): Inicia/Para o metrônomo\n");
    printf("  - GPIO 21 (SPEED): Diminui 500ms a cada pressão\n");
    printf("                     Cicla: 3000→2500→2000→1500→1000→500→3000ms\n");
    printf("\nVelocidades (BPM):\n");
    printf("  - 3000ms = 40 BPM   (2 batidas por 3 segundos)\n");
    printf("  - 2500ms = 48 BPM   (2 batidas por 2.5 segundos)\n");
    printf("  - 2000ms = 60 BPM   (2 batidas por 2 segundos)\n");
    printf("  - 1500ms = 80 BPM   (2 batidas por 1.5 segundos)\n");
    printf("  - 1000ms = 120 BPM  (2 batidas por 1 segundo)\n");
    printf("  - 500ms  = 240 BPM  (2 batidas por 0.5 segundo)\n");
    printf("\nComportamento:\n");
    printf("  - Servo: Movimento contínuo alternando entre extremos\n");
    printf("  - LED e Buzzer: Ativam quando servo passa por ~90°\n");
    printf("  - Buzzer: Soa por 100ms a partir dos 90°\n");
    printf("\nPressione Ctrl+C para sair\n");
    printf("Pressione GPIO 26 para iniciar/parar\n");
    printf("Pressione GPIO 21 para ajustar velocidade\n\n");

    /* Loop principal */
    while (!should_exit) {
        metronome_loop();
    }

    /* Limpeza */
    cleanup();

    return 0;
}
