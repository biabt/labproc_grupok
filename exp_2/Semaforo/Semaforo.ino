const int LED_VERDE = 8;
const int LED_AMARELO = 9;
const int LED_VERMELHO = 10;

void apagarTodos() {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, LOW);
}

void setup() {
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
}

void loop() {

  // VERDE por 3 segundos
  apagarTodos();
  digitalWrite(LED_VERDE, HIGH);
  delay(3000);

  // VERMELHO por 4 segundos
  apagarTodos();
  digitalWrite(LED_VERMELHO, HIGH);
  delay(4000);

  // AMARELO por 1 segundo
  apagarTodos();
  digitalWrite(LED_AMARELO, HIGH);
  delay(1000);
}