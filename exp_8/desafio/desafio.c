/* =============================================================
 * desafio.c - Calculadora Binaria com Matrix Keypad + LCD1602
 * Alvo: Raspberry Pi 3 (Cortex-A53, ARM64)
 *
 * Utiliza:
 *   - Matrix Keypad 4x4 para entrada (I2C via Freenove Keypad class)
 *   - Display LCD1602 via I2C (PCF8574 expander)
 *   - ULA em Assembly ARM64 (alu.s)
 * 
 * Fluxo:
 *   1. Inicialização: wiringPi + LCD I2C (0x27) + Keypad I2C (0x48)
 *   2. Loop principal: Polling do teclado via Keypad class
 *   3. Decodificação de tecla → operação na calculadora
 *   4. Atualização do LCD com resultado
 * ============================================================= */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <pcf8574.h>
#include <lcd.h>

/* Freenove Keypad library (C++ classes) */
#include "Keypad.hpp"

/* ---------- Protótipos das rotinas escritas em alu.s ---------- */
extern int64_t alu_add(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_sub(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_mul(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_fat(int64_t n, uint32_t *err);
extern int64_t alu_div(int64_t a, int64_t b, uint32_t *err);

/* ---------- Configuração de LCD1602 via I2C ---------- */
#define PCF8574_ADDR_1  0x27        // PCF8574T
#define PCF8574_ADDR_2  0x3F        // PCF8574AT
#define BASE            64          // BASE para pinos do PCF8574

#define RS      BASE+0
#define RW      BASE+1
#define EN      BASE+2
#define LED     BASE+3
#define D4      BASE+4
#define D5      BASE+5
#define D6      BASE+6
#define D7      BASE+7

/* ---------- Configuração da Matrix Keypad 4x4 via I2C ---------- */
#define ROWS    4
#define COLS    4

const char keys[ROWS][COLS] = {
    {'1','2','3','A'},  // A = +
    {'4','5','6','B'},  // B = -
    {'7','8','9','C'},  // C = *
    {'*','0','#','D'}   // D = /, # = =, * = !
};

const byte rowPins[ROWS] = {16, 20, 21, 26};  // Pinos de linha (mapeados pela classe Keypad)
const byte colPins[COLS] = {19, 13, 6, 5};    // Pinos de coluna (mapeados pela classe Keypad)

/* Instância global do Keypad (Freenove) */
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

/* ---------- Estado da máquina de estados da calculadora -------- */
typedef enum { OP_NONE, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_FAT } opcode_t;

typedef struct {
    char     display[64];   /* string mostrada no visor         */
    int32_t  operand_a;     /* registrador A (até 32 bits)      */
    int32_t  operand_b;     /* registrador B (até 32 bits)      */
    opcode_t pending_op;    /* operacao decodificada e pendente */
    int      entering_b;    /* 0 = digitando A, 1 = digitando B */
    int      error_state;   /* flag de erro / exceção           */
} calc_state_t;

static calc_state_t st;
static int lcdhd;           /* handle do LCD */

/* ========== FUNÇÕES DE LCD ========== */

int detectI2C(int addr) {
    int _fd = wiringPiI2CSetup(addr);
    if (_fd < 0) {
        printf("Erro detectando endereco: 0x%x\n", addr);
        return 0;
    } else {
        if (wiringPiI2CWrite(_fd, 0) < 0) {
            printf("Nao encontrado dispositivo no endereco 0x%x\n", addr);
            return 0;
        } else {
            printf("Encontrado dispositivo LCD no endereco 0x%x\n", addr);
            return 1;
        }
    }
}

void lcd_init(void) {
    int i;
    int pcf8574_address = 0;

    printf("Inicializando LCD...\n");

    if (detectI2C(PCF8574_ADDR_1)) {
        pcf8574_address = PCF8574_ADDR_1;
    } else if (detectI2C(PCF8574_ADDR_2)) {
        pcf8574_address = PCF8574_ADDR_2;
    } else {
        printf("Nenhum endereco I2C encontrado para LCD.\n"
               "Use 'i2cdetect -y 1' para verificar o endereco.\n");
        exit(1);
    }

    pcf8574Setup(BASE, pcf8574_address);
    for (i = 0; i < 8; i++) {
        pinMode(BASE + i, OUTPUT);
    }

    digitalWrite(LED, HIGH);    /* ligando backlight */
    digitalWrite(RW, LOW);      /* permitindo escrita no LCD */

    lcdhd = lcdInit(2, 16, 4, RS, EN, D4, D5, D6, D7, 0, 0, 0, 0);
    if (lcdhd == -1) {
        printf("Erro: falha ao inicializar LCD\n");
        exit(1);
    }

    printf("LCD inicializado com sucesso\n");
}

void lcd_display_state(void) {
    char line1[17];
    char line2[17];
    const char *opname = "";

    if (st.error_state) {
        snprintf(line1, 17, "%-16s", st.display);
        snprintf(line2, 17, "%-16s", "");
    } else {
        /* Monta nome da operacao pendente */
        switch (st.pending_op) {
            case OP_ADD: opname = "+"; break;
            case OP_SUB: opname = "-"; break;
            case OP_MUL: opname = "*"; break;
            case OP_DIV: opname = "/"; break;
            case OP_FAT: opname = "!"; break;
            default: opname = ""; break;
        }

        /* Linha 1: A=valor OP B=valor */
        snprintf(line1, 17, "A=%-3d %s B=%-3d",
                 st.operand_a, opname, st.operand_b);

        /* Linha 2: Resultado ou proxima entrada */
        snprintf(line2, 17, "Res:%-11s", st.display);
    }

    lcdPosition(lcdhd, 0, 0);
    lcdPrintf(lcdhd, "%s", line1);

    lcdPosition(lcdhd, 0, 1);
    lcdPrintf(lcdhd, "%s", line2);
}

/* ========== FUNÇÕES DE KEYPAD (I2C via Freenove Keypad class) ========== */

void keypad_init(void) {
    printf("Inicializando Matrix Keypad (I2C)...\n");
    keypad.setDebounceTime(50);  /* 50ms debounce */
    printf("Keypad inicializado com sucesso\n");
}

char keypad_getKey(void) {
    return keypad.getKey();  /* Retorna 0 se nenhuma tecla, senão retorna o caractere */
}

/* ========== FUNÇÕES DA CALCULADORA (adaptadas de main.c) ========== */

static void calc_reset(void) {
    memset(&st, 0, sizeof(st));
    strcpy(st.display, "0");
}

static void push_digit(int digit) {
    int32_t *target = st.entering_b ? &st.operand_b : &st.operand_a;

    /* Qualquer tecla após erro reinicia a calculadora */
    if (st.error_state) {
        calc_reset();
        st.error_state = 0;
    }

    /* Evita ultrapassar o limite de um inteiro de 32 bits (2,147,483,647) */
    if (*target > (INT32_MAX - digit) / 10) {
        st.error_state = 1;
        strcpy(st.display, "OVERFLOW");
        return;
    }

    *target = (*target * 10) + digit;

    snprintf(st.display, sizeof(st.display), "%d", (int)*target);
}

static void select_op(opcode_t op) {
    if (st.error_state) {
        calc_reset();
        st.error_state = 0;
    }

    st.pending_op = op;
    st.entering_b = (op != OP_FAT);  /* fatorial é unário */

    if (op == OP_FAT) {
        /* fatorial é executado imediatamente */
        uint32_t err = 0;
        int64_t r = alu_fat(st.operand_a, &err);
        if (err) {
            st.error_state = 1;
            strcpy(st.display, "OVERFLOW");
        } else {
            st.operand_a = (int32_t)r;
            snprintf(st.display, sizeof(st.display), "%d", (int)st.operand_a);
        }
    }
}

static void calc_equals(void) {
    uint32_t err = 0;
    int64_t result = 0;

    switch (st.pending_op) {
        case OP_ADD:
            result = alu_add(st.operand_a, st.operand_b, &err);
            break;
        case OP_SUB:
            result = alu_sub(st.operand_a, st.operand_b, &err);
            break;
        case OP_MUL:
            result = alu_mul(st.operand_a, st.operand_b, &err);
            break;
        case OP_DIV:
            result = alu_div(st.operand_a, st.operand_b, &err);
            break;
        default:
            return;  /* nada pendente */
    }

    if (err) {
        st.error_state = 1;
        strcpy(st.display, st.pending_op == OP_DIV ? "DIV/0" : "ERROR");
    } else {
        if (result > INT32_MAX || result < INT32_MIN) {
            st.error_state = 1;
            strcpy(st.display, "OVERFLOW");
        } else {
            st.operand_a = (int32_t)result;
            snprintf(st.display, sizeof(st.display), "%d", (int)st.operand_a);
        }
    }

    st.pending_op = OP_NONE;
    st.entering_b = 0;
    st.operand_b = 0;
}

static void handle_keypad(char key) {
    switch (key) {
        case '0': push_digit(0); break;
        case '1': push_digit(1); break;
        case '2': push_digit(2); break;
        case '3': push_digit(3); break;
        case '4': push_digit(4); break;
        case '5': push_digit(5); break;
        case '6': push_digit(6); break;
        case '7': push_digit(7); break;
        case '8': push_digit(8); break;
        case '9': push_digit(9); break;

        case 'A': select_op(OP_ADD); break;  /* + */
        case 'B': select_op(OP_SUB); break;  /* - */
        case 'C': select_op(OP_MUL); break;  /* * */
        case 'D': select_op(OP_DIV); break;  /* / */

        case '#': calc_equals(); break;      /* = */
        case '*': select_op(OP_FAT); break;  /* ! (fatorial) */

        default: break;
    }
}

/* ========== MAIN ========== */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("Calculadora ARM - Matrix Keypad + LCD1602 (I2C)\n");
    printf("Inicializando hardware...\n\n");

    /* Inicializar wiringPi (usar BCM) */
    wiringPiSetupGpio();

    /* Inicializar LCD (I2C 0x27) */
    lcd_init();

    /* Inicializar Keypad (I2C via Freenove class) */
    keypad_init();

    /* Resetar calculadora */
    calc_reset();

    /* Mostrar estado inicial */
    lcd_display_state();

    printf("\nCalculadora pronta. Aguardando entrada...\n");
    printf("Teclas: 0-9 (digitos), A/B/C/D (operacoes), # (igual), * (fatorial)\n");

    /* Loop principal */
    while (1) {
        char key = keypad_getKey();

        if (key) {
            printf("Tecla pressionada: %c\n", key);
            handle_keypad(key);
            lcd_display_state();
        }

        delay(50);  /* polling interval */
    }

    return 0;
}
