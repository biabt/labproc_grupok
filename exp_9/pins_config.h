/* =============================================================
 * pins_config.h - Referência de Pinos do Projeto
 * 
 * NOTA: Este arquivo é LEGADO, mantido apenas como referência.
 * 
 * A configuração atual está em main.c com integração do Kit Freenove.
 * ============================================================= */

#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

/* ==================== MAPEAMENTO DE PINOS (BCM) ==================== */

/* Kit Freenove FNK0054 - Integrado em main.c */
#define PIN_LED_RED         17      /* GPIO17 - LED Red (Blink.c) */
#define PIN_BUTTON_START    25      /* GPIO25 - Button Start/Stop */
#define PIN_BUZZER          12      /* GPIO12 - Buzzer (Doorbell.c) */
#define PIN_SERVO           18      /* GPIO18 - Servo Motor (Sweep.c) */

/* ==================== CONSTANTES ESSENCIAIS ==================== */

#define DEBOUNCE_MS         200      /* Debounce para botões */
#define BPM_DEFAULT         120     /* BPM fixo em 120 */

/* Servo Motor - Constantes do Freenove Sweep.c */
#define OFFSET_MS           3       /* Offset usado pelo Freenove */
#define SERVO_MIN_MS        (5 + OFFSET_MS)      /* ~5.3ms = 0° */
#define SERVO_MID_MS        15      /* ~15ms = 90° */
#define SERVO_MAX_MS        (25 + OFFSET_MS)     /* ~25.3ms = 180° */

#endif /* PINS_CONFIG_H */
