/* =============================================================
 * fechadura.cpp - Sistema de Tranca Eletrônica RPi3
 * Alvo: Raspberry Pi 3 (Cortex-A53)
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <pcf8574.h>
#include <lcd.h>
#include <sys/time.h>
#include "Keypad.hpp"

// Configurações LCD I2C
#define LCD_ADDR          0x27
#define BASE              64
#define RS (BASE+0), RW (BASE+1), EN (BASE+2), LED (BASE+3), D4 (BASE+4), D5 (BASE+5), D6 (BASE+6), D7 (BASE+7)

// Pinos Periféricos
#define PIN_BUZZER        12
#define PIN_TRIG          23
#define PIN_ECHO          24

// Configurações de Segurança
#define PASSWORD          "1234"
#define MAX_PASS_LEN      10
#define DIST_THRESHOLD    10.0 // cm (Acima disso, porta considerada aberta)

// Máquina de Estados
typedef enum { STATE_LOCKED, STATE_UNLOCKED, STATE_ALARM } lock_state_t;

// Variáveis Globais
lock_state_t current_state = STATE_LOCKED;
char input_buffer[MAX_PASS_LEN];
int input_ptr = 0;
int lcd_h;

// Instância Keypad (conforme seu código anterior)
const char keys[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};
byte rowPins[4] = {16, 20, 21, 26};
byte colPins[4] = {19, 13, 6, 5};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 4);

/* --- Funções do Sensor Ultrassônico --- */
float get_distance() {
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long start, end;
    while(digitalRead(PIN_ECHO) == LOW);
    start = micros();
    while(digitalRead(PIN_ECHO) == HIGH);
    end = micros();

    return (float)(end - start) * 0.01715;
}

/* --- Funções de Feedback --- */
void buzzer_feedback(int duration_ms) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(duration_ms);
    digitalWrite(PIN_BUZZER, LOW);
}

void update_lcd(const char* line1, const char* line2) {
    lcdClear(lcd_h);
    lcdPosition(lcd_h, 0, 0);
    lcdPrintf(lcd_h, line1);
    lcdPosition(lcd_h, 0, 1);
    lcdPrintf(lcd_h, line2);
}

/* --- Lógica Principal --- */
void process_key(char key) {
    if (key == '#') { // Tecla para submeter (Enter)
        input_buffer[input_ptr] = '\0'; // Garante que a string está finalizada
        
        if (strcmp(input_buffer, PASSWORD) == 0) {
            current_state = STATE_UNLOCKED;
            update_lcd("ACESSO LIBERADO", "Seja Bem-vindo");
            buzzer_feedback(200); 
            delay(3000); 
            current_state = STATE_LOCKED;
            // Limpa após sucesso
            input_ptr = 0;
            memset(input_buffer, 0, MAX_PASS_LEN);
        } else {
            update_lcd("ACESSO NEGADO", "Senha Incorreta");
            buzzer_feedback(1000); 
            delay(1000);
            // Limpa após erro para nova tentativa
            input_ptr = 0;
            memset(input_buffer, 0, MAX_PASS_LEN);
            update_lcd("INSIRA A SENHA:", "");
        }
    } 
    else if (key == '*') { // Tecla para Limpar (Clear/Backspace)
        input_ptr = 0;
        memset(input_buffer, 0, MAX_PASS_LEN);
        update_lcd("INSIRA A SENHA:", "");
    }
    else if (input_ptr < MAX_PASS_LEN - 1) {
        // Armazena o caractere
        input_buffer[input_ptr++] = key;
        input_buffer[input_ptr] = '\0'; // Mantém a string válida para o printf
        
        // EXIBIÇÃO: Enviamos o buffer real em vez da máscara de '*'
        update_lcd("INSIRA A SENHA:", input_buffer);
        
        // Feedback sonoro curto para cada tecla
        digitalWrite(PIN_BUZZER, HIGH);
        delay(30);
        digitalWrite(PIN_BUZZER, LOW);
    }
}

int main() {
    wiringPiSetupGpio();
    
    // Inicialização LCD
    pcf8574Setup(BASE, LCD_ADDR);
    for(int i=0; i<8; i++) pinMode(BASE+i, OUTPUT);
    digitalWrite(BASE+3, HIGH); // Backlight
    lcd_h = lcdInit(2, 16, 4, BASE+0, BASE+2, BASE+4, BASE+5, BASE+6, BASE+7, 0, 0, 0, 0);

    // Inicialização Outros
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    keypad.setDebounceTime(50);

    update_lcd("SISTEMA PRONTO", "Aguardando...");

    while(1) {
        // 1. Monitoramento do Sensor (Degrau 1: Core + Sensor)
        float dist = get_distance();
        if (current_state == STATE_LOCKED && dist > DIST_THRESHOLD) {
            // Porta aberta sem autorização!
            update_lcd("ALERTA!", "PORTA ABERTA");
            buzzer_feedback(100); // Alarme intermitente
        }

        // 2. Leitura do Teclado (Degrau 2: Keypad Integration)
        char key = keypad.getKey();
        if (key) {
            process_key(key);
        }

        if (current_state == STATE_LOCKED && input_ptr == 0) {
            lcdPosition(lcd_h, 0, 0);
            lcdPrintf(lcd_h, "PORTA TRANQUEDA ");
            lcdPosition(lcd_h, 0, 1);
            lcdPrintf(lcd_h, "Status: OK      ");
        }

        delay(100); // Polling amigável à CPU
    }

    return 0;
}