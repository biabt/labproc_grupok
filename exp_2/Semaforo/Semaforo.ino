#define NEOPIXEL_PIN LED_BUILTIN

void setup() {
  pinMode(NEOPIXEL_PIN, OUTPUT);
}

void loop() {
  // VERDE por 3 segundos
  neopixelWrite(NEOPIXEL_PIN, 0, 255, 0);
  delay(3000);

  // AMARELO por 1 segundo
  neopixelWrite(NEOPIXEL_PIN, 255, 255, 0);
  delay(1000);

  // VERMELHO por 4 segundos
  neopixelWrite(NEOPIXEL_PIN, 255, 0, 0);
  delay(4000);
}