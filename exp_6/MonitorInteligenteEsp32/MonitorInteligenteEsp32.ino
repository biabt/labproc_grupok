#include <WiFi.h>
#include <WebServer.h>

// --- Definições de Pinos ---
#define PIN_LDR          34  
#define PIN_BUTTON_SOS   25  
#define PIN_LED_EXTERNO  26  
#define PIN_LED_BUILDIN  LED_BUILTIN   
#define IS_DESAFIO false

// --- Configurações de Rede Wi-Fi ---
const char* ssid     = "ServoLED_ESP32_GrupoK";
const char* password = "";

WebServer server(80);

// --- Variáveis Globais de Controle ---
volatile bool sosPressionado = false;
volatile unsigned long tempoUltimaInterrupcao = 0;
const unsigned long TEMPO_DEBOUNCE = 50; 

unsigned long tempoAnteriorLDR = 0;
const unsigned long INTERVALO_LDR = 1000; 

unsigned long tempoAnteriorPisca = 0;
const unsigned long INTERVALO_PISCA = 2000; 

int valorADC = 0;
const int LIMIAR_ESCURIDAO = 1500; 
bool statusLedBuiltIn = false;

enum EstadoSistema { NORMAL, BAIXA_LUMINOSIDADE, EMERGENCIA_SOS };
EstadoSistema estadoAtual = NORMAL;

unsigned long tempoInicioSOS = 0;
const unsigned long DURACAO_SOS = 3000; 

void IRAM_ATTR tratadorInterrupcaoSOS();
void handleDados();

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_BUTTON_SOS, INPUT_PULLUP); 
  pinMode(PIN_LED_EXTERNO, OUTPUT);
  pinMode(PIN_LED_BUILDIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_SOS), tratadorInterrupcaoSOS, FALLING);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  Serial.println("\nConectado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.localIP());

  // Define a rota que o HTML externo vai consultar
  server.on("/dados", handleDados);
  server.begin();
}

void loop() {
  server.handleClient(); 
  
  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoAnteriorLDR >= INTERVALO_LDR) {
    valorADC = analogRead(PIN_LDR);
    tempoAnteriorLDR = tempoAtual;
  }

  atualizarEstadoSistema();
  gerenciarSinalizacao();
}

void IRAM_ATTR tratadorInterrupcaoSOS() {
  unsigned long tempoInterrupcaoAtual = millis();
  if (tempoInterrupcaoAtual - tempoUltimaInterrupcao > TEMPO_DEBOUNCE) {
    sosPressionado = true;
    tempoUltimaInterrupcao = tempoInterrupcaoAtual;
  }
}

void atualizarEstadoSistema() {
  unsigned long tempoAtual = millis();
  if (sosPressionado) {
    estadoAtual = EMERGENCIA_SOS;
    tempoInicioSOS = tempoAtual;
    sosPressionado = false;
    neopixelWrite(NEOPIXEL_PIN, 255, 0, 0);
  }
  if (estadoAtual == EMERGENCIA_SOS) {
    if (tempoAtual - tempoInicioSOS >= DURACAO_SOS) {
        estadoAtual = NORMAL;
        neopixelWrite(NEOPIXEL_PIN, 0, 0, 0);
    }
  } else {
    estadoAtual = (valorADC < LIMIAR_ESCURIDAO) ? BAIXA_LUMINOSIDADE : NORMAL;
  }
}

void gerenciarSinalizacao() {
  unsigned long tempoAtual = millis();
  switch (estadoAtual) {
    case EMERGENCIA_SOS:
      neopixelWrite(NEOPIXEL_PIN, 255, 0, 0);
      break;
    case BAIXA_LUMINOSIDADE:
      if (tempoAtual - tempoAnteriorPisca >= INTERVALO_PISCA) {
        tempoAnteriorPisca = tempoAtual;
        statusLedBuiltIn = !statusLedBuiltIn;
        neopixelWrite(NEOPIXEL_PIN, 255, 255, 0);
      }
      break;
    case NORMAL:
      if(IS_DESAFIO)
        neopixelWrite(NEOPIXEL_PIN, 0, 255, 0);
      else
        neopixelWrite(NEOPIXEL_PIN, 0, 0, 0);
      statusLedBuiltIn = false;
      break;
  }
}

// --- Envia apenas os dados brutos via HTTP ---
void handleDados() {
  String estadoStr;
  if (estadoAtual == EMERGENCIA_SOS) estadoStr = "EMERGENCIA";
  else if (estadoAtual == BAIXA_LUMINOSIDADE) estadoStr = "BAIXA LUMINOSIDADE";
  else estadoStr = "NORMAL";

  // Monta um JSON simples com as informações
  String json = "{";
  json += "\"ldr\":" + String(valorADC) + ",";
  json += "\"estado\":\"" + estadoStr + "\"";
  json += "}";

  // CORS Enable: Permite que arquivos HTML locais (fora do ESP32) acessem a API sem bloqueio do navegador
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}