#include <WiFi.h>
#include <WebServer.h>

// --- Definições de Pinos ---
#define PIN_LDR          3  
#define PIN_BUTTON_TRAVESSIA   6 
#define PIN_LED_BUILDIN  LED_BUILTIN   

// --- Configurações de Rede Wi-Fi ---
const char* ssid     = "ESP32_GrupoK";
const char* password = "";

WebServer server(80);

// --- Variáveis Globais de Controle ---
volatile bool travesssiaPressionado = false;
volatile unsigned long tempoUltimaInterrupcao = 0;
const unsigned long TEMPO_DEBOUNCE = 50; 

unsigned long tempoAnteriorLDR = 0;
const unsigned long INTERVALO_LDR = 1000; 

unsigned long tempoAnteriorPisca = 0;
const unsigned long INTERVALO_PISCA = 2000; 

int valorADC = 0;
const int LIMIAR_ESCURIDAO = 2000; 
bool statusLedBuiltIn = false;

enum EstadoSistema { NORMAL, BAIXA_LUMINOSIDADE, EMERGENCIA_SOS };
EstadoSistema estadoAtual = NORMAL;

unsigned long tempoInicioSOS = 0;
const unsigned long DURACAO_SOS = 3000; 

void IRAM_ATTR tratadorInterrupcaoTravessia();
void handleDados();

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_BUTTON_TRAVESSIA, INPUT_PULLUP); 
  pinMode(PIN_LED_BUILDIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_TRAVESSIA), tratadorInterrupcaoTravessia, FALLING);

  WiFi.softAP(ssid, password);
  Serial.println("Wi-Fi Iniciado com Sucesso!");
  Serial.print("IP do Servidor: ");
  Serial.println(WiFi.softAPIP());

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
    estadoAtual = EMERGENCIA_SOS;
    tempoInicioSOS = tempoAtual;
    travesssiaPressionado = false;
    neopixelWrite(PIN_LED_BUILDIN, 255, 0, 0);
  }
  if (estadoAtual == EMERGENCIA_SOS) {
    if (tempoAtual - tempoInicioSOS >= DURACAO_SOS) {
        estadoAtual = NORMAL;
        neopixelWrite(PIN_LED_BUILDIN, 0, 0, 0);
    }
  } else {
    estadoAtual = (valorADC < LIMIAR_ESCURIDAO) ? NORMAL : BAIXA_LUMINOSIDADE;
  }
}

void gerenciarSinalizacao() {
  unsigned long tempoAtual = millis();
  switch (estadoAtual) {
    case EMERGENCIA_SOS:
      neopixelWrite(PIN_LED_BUILDIN, 255, 0, 0);
      break;
    case BAIXA_LUMINOSIDADE:
        if (tempoAtual - tempoAnteriorPisca >= INTERVALO_PISCA) {
            tempoAnteriorPisca = tempoAtual;
            statusLedBuiltIn = !statusLedBuiltIn;
            if (statusLedBuiltIn) {
                neopixelWrite(PIN_LED_BUILDIN, 255, 255, 0); // Amarelo
            } else {
                neopixelWrite(PIN_LED_BUILDIN, 0, 0, 0);     // Desligado
            }
        }
      break;
    case NORMAL:
      neopixelWrite(PIN_LED_BUILDIN, 0, 0, 0);
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