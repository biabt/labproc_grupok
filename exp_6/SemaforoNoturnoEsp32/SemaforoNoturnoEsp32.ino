#include <WiFi.h>
#include <WebServer.h>

// --- Definições de Pinos ---
#define PIN_LDR          34  
#define PIN_BUTTON_TRAVESSIA   25  
#define NEOPIXEL_PIN  LED_BUILTIN   


// --- Configurações de Rede Wi-Fi ---
const char* ssid     = "SemaforoNoturno_ESP32_GrupoK";
const char* password = "";

WebServer server(80);

// --- Variáveis Globais de Controle ---
volatile bool travesssiaPressionado = false;
volatile unsigned long tempoUltimaInterrupcao = 0;
const unsigned long TEMPO_DEBOUNCE = 50; 

unsigned long tempoAnteriorLDR = 0;
const unsigned long INTERVALO_LDR = 1000; 

unsigned long tempoAnteriorPisca = 0;
unsigned long tempoAnteriorSemaforo = 0;
const unsigned long INTERVALO_PISCA = 1000; // 1 segundo para o pisca do modo noturno

int valorADC = 0;
const int LIMIAR_ESCURIDAO = 1500; 
bool statusLedBuiltIn = false;

// --- Nova Máquina de Estados Unificada ---
enum EstadoSistema { VERDE, AMARELO, VERMELHO, NOTURNO };
EstadoSistema estadoAtual = VERDE;


// Tempos de cada fase do semáforo (em milissegundos)
const unsigned long TEMPO_VERDE    = 5000; 
const unsigned long TEMPO_AMARELO  = 2000; 
const unsigned long TEMPO_VERMELHO = 5000; 

void IRAM_ATTR tratadorInterrupcaoTravessia();
void handleDados();
void atualizarEstadoSistema();
void gerenciarSinalizacao();

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_BUTTON_TRAVESSIA, INPUT_PULLUP); 
  pinMode(NEOPIXEL_PIN, OUTPUT);


  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_TRAVESSIA), tratadorInterrupcaoTravessia, FALLING);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  Serial.println("\nConectado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());

  server.on("/dados", handleDados);
  server.begin();
}

void loop() {
  server.handleClient(); 
  
  unsigned long tempoAtual = millis();

  // Leitura do LDR a cada 1 segundo (Frequência de 1Hz)
  if (tempoAtual - tempoAnteriorLDR >= INTERVALO_LDR) {
    valorADC = analogRead(PIN_LDR);
    tempoAnteriorLDR = tempoAtual;
  }

  atualizarEstadoSistema();
  gerenciarSinalizacao();
}

void IRAM_ATTR tratadorInterrupcaoTravessia() {
  unsigned long tempoInterrupcaoAtual = millis();
  if (tempoInterrupcaoAtual - tempoUltimaInterrupcao > TEMPO_DEBOUNCE) {
    travesssiaPressionado = true;
    tempoUltimaInterrupcao = tempoInterrupcaoAtual;
  }
}

void atualizarEstadoSistema() {
  unsigned long tempoAtual = millis();

  if (travesssiaPressionado) {
    if (estadoAtual != VERMELHO) {
        estadoAtual = VERMELHO; // Força o semáforo para vermelho imediatamente
        tempoAnteriorSemaforo = tempoAtual; // Reinicia o timer do semáforo
        travesssiaPressionado = false;
    }
  }
  else if (valorADC < LIMIAR_ESCURIDAO) {    
      estadoAtual = NOTURNO;
  } else {
    // Se o sistema estava no modo noturno e a luz voltou, ele reinicia com segurança no VERDE
    if (estadoAtual == NOTURNO) {
      estadoAtual = VERDE;
      tempoAnteriorSemaforo = tempoAtual;
    }

    // Transições baseadas em tempo (millis)
    if (estadoAtual == VERDE && (tempoAtual - tempoAnteriorSemaforo >= TEMPO_VERDE)) {
      estadoAtual = AMARELO;
      tempoAnteriorSemaforo = tempoAtual;
    } 
    else if (estadoAtual == AMARELO && (tempoAtual - tempoAnteriorSemaforo >= TEMPO_AMARELO)) {
      estadoAtual = VERMELHO;
      tempoAnteriorSemaforo = tempoAtual;
    } 
    else if (estadoAtual == VERMELHO && (tempoAtual - tempoAnteriorSemaforo >= TEMPO_VERMELHO)) {
      estadoAtual = VERDE;
      tempoAnteriorSemaforo = tempoAtual;
    }
  }
}

void gerenciarSinalizacao() {
  unsigned long tempoAtual = millis();
  
  switch (estadoAtual) {
    case NOTURNO:
      // Pisca a luz amarela de forma não-bloqueante a cada 1 segundo
      if (tempoAtual - tempoAnteriorPisca >= INTERVALO_PISCA) {
        tempoAnteriorPisca = tempoAtual;
        statusLedBuiltIn = !statusLedBuiltIn;
        if (statusLedBuiltIn) {
          neopixelWrite(NEOPIXEL_PIN, 255, 120, 0); // Amarelo
        } else {
          neopixelWrite(NEOPIXEL_PIN, 0, 0, 0);       // Desligado
        }
      }
      break;

    case VERDE:
      neopixelWrite(NEOPIXEL_PIN, 0, 255, 0); // Verde estável
      break;

    case AMARELO:
      neopixelWrite(NEOPIXEL_PIN, 255, 120, 0); // Amarelo estável
      break;

    case VERMELHO:
      neopixelWrite(NEOPIXEL_PIN, 255, 0, 0); // Vermelho estável
      break;
  }
}

void handleDados() {
  String estadoStr;
  switch (estadoAtual) {
    case VERDE:          estadoStr = "VERDE"; break;
    case AMARELO:        estadoStr = "AMARELO"; break;
    case VERMELHO:       estadoStr = "VERMELHO"; break;
    case NOTURNO:        estadoStr = "NOTURNO (PISCA AMARELO)"; break;
  }

  String json = "{";
  json += "\"ldr\":" + String(valorADC) + ",";
  json += "\"estado\":\"" + estadoStr + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}