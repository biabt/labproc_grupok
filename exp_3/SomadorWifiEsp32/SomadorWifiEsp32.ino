#include <WiFi.h>
#include <WebServer.h>

#define LED_BIT0 10
#define LED_BIT1 11
#define LED_BIT2 12
#define LED_BIT3 13

WebServer server(80);

// SSID e senha do wifi
const char *ssid = "Calculadora_ESP32";
const char *password = "";

int8_t converterParaInteiro(String binStr) {
    long valor = strtol(binStr.c_str(), NULL, 2); // Parsing de base 2
    
    if (valor & 0x08) {
        return (int8_t)(0xF0 | valor); // Completar o binario com 1 caso seja negativo
    }
    return (int8_t)(valor & 0x0F);
}

bool isStringBinaryValid(String binStr) {
    if (binStr.length() != 4) 
        return false; 
    
    for (int i = 0; i < 4; i++) {
        char c = binStr.charAt(i);
        if (c != '0' && c != '1') 
            return false;        
    }
    return true;
}

// Função auxiliar para transformar o resultado numérico em string binária de 4 bits
String paraStringBinaria(int8_t valor) {
    String str = "";
    for (int i = 3; i >= 0; i--) {
        str += ((valor >> i) & 0x01) ? "1" : "0";
    }
    return str;
}

bool OverFlowPositivo(int8_t resultado) { return resultado > 7; }
bool OverFlowNegativo(int8_t resultado) { return resultado < -8;}

void tratarCalculo() {
    server.sendHeader("Access-Control-Allow-Origin", "*");

    if (!server.hasArg("a") || !server.hasArg("b") || !server.hasArg("op")) {
        server.send(400, "application/json", "{\"error\":\"Parâmetros ausentes na requisição.\"}");
        return;
    }

    String paramA = server.arg("a");
    String paramB = server.arg("b");
    String op     = server.arg("op");

    if (!isStringBinaryValid(paramA) || !isStringBinaryValid(paramB)) {
        server.send(400, "application/json", "{\"error\":\"Os operandos devem conter exatamente 4 bits binários (0 ou 1).\"}");
        return;
    }
    int8_t valA = converterParaInteiro(paramA);
    int8_t valB = converterParaInteiro(paramB);
    int8_t resultado = 0;
    bool overflow = false;

    // 2. Operação Aritmética Dinâmica [cite: 53]
    if (op == "add") {
        resultado = valA + valB;
        // Overflow na Soma: Dois positivos geram negativo OU dois negativos geram positivo
        if ((valA > 0 && valB > 0 && OverFlowPositivo(resultado)) || (valA < 0 && valB < 0 && OverFlowNegativo(resultado))) {
            overflow = true;
        }
    } else if (op == "sub") {
        resultado = valA - valB;
        // Overflow na Subtração: Positivo menos negativo gera negativo OU vice-versa
        if ((valA >= 0 && valB < 0 && OverFlowPositivo(resultado)) || (valA < 0 && valB > 0 && OverFlowNegativo(resultado))) {
            overflow = true;
        }
    } else {
        server.send(400, "application/json", "{\"error\":\"Operação inválida. Use 'add' ou 'sub'.\"}");
        return;
    }
    int8_t resultadoMascarado = resultado & 0x0F;

    digitalWrite(LED_BIT0, (resultadoMascarado >> 0) & 0x01);
    digitalWrite(LED_BIT1, (resultadoMascarado >> 1) & 0x01);
    digitalWrite(LED_BIT2, (resultadoMascarado >> 2) & 0x01);
    digitalWrite(LED_BIT3, (resultadoMascarado >> 3) & 0x01);

    String jsonResposta = "{";
    jsonResposta += "\"resultado_decimal\":" + String(resultado) + ",";
    jsonResposta += "\"resultado_bin\":\"" + paraStringBinaria(resultadoMascarado) + "\",";
    jsonResposta += "\"overflow\":" + String(overflow ? "true" : "false");
    jsonResposta += "}";

    server.send(200, "application/json", jsonResposta);
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_BIT0, OUTPUT);
    pinMode(LED_BIT1, OUTPUT);
    pinMode(LED_BIT2, OUTPUT);
    pinMode(LED_BIT3, OUTPUT);

    // Teste cego inicial que liga rapidamente os leds para confirmar seu funcionamento [cite: 153]
    digitalWrite(LED_BIT0, HIGH); delay(150);
    digitalWrite(LED_BIT1, HIGH); delay(150);
    digitalWrite(LED_BIT2, HIGH); delay(150);
    digitalWrite(LED_BIT3, HIGH); delay(150);

    digitalWrite(LED_BIT0, LOW);  
    digitalWrite(LED_BIT1, LOW);
    digitalWrite(LED_BIT2, LOW);  
    digitalWrite(LED_BIT3, LOW);

    WiFi.softAP(ssid, password);
    Serial.println("Wifi Iniciado!");
    Serial.print("IP da Calculadora: ");
    Serial.println(WiFi.softAPIP());

    server.on("/calc", HTTP_GET, tratarCalculo);

    server.begin();
    Serial.println("Servidor HTTP ativo na porta 80.");
}

void loop() {
    server.handleClient();
}