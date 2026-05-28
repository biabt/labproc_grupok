#include <WiFi.h>
#include <WebServer.h>

// Mapeamento físico dos GPIOs para os LEDs de monitoramento
#define LED_BIT0 6
#define LED_BIT1 7
#define LED_BIT2 8
#define LED_BIT3 9

WebServer server(80);

// Configuração do Ponto de Acesso (Access Point) do ESP32
const char *ssid = "Calculadora_ESP32_GrupoK";
const char *password = "";

// Conversão com extensão de sinal dinâmica baseada em nBits
long converterParaLong(String binStr, long nBits) {
    long valor = strtol(binStr.c_str(), NULL, 2); 
    long mascaraValida = (1 << nBits) - 1; 
    valor = valor & mascaraValida; 

    if (valor & (1 << (nBits - 1))) {
        long mascaraExtensaoSinal = ~mascaraValida;
        return (long)(valor | mascaraExtensaoSinal); 
    }
    return (long)valor;
}

long MascaraBits(long value, long nBits) { 
    return value & ((1 << nBits) - 1); 
}

// Validação flexível que aceita strings binárias de qualquer tamanho nBits
bool isStringBinaryValidWifi(String binStr, long nBits) {
    if (binStr.length() != nBits) 
        return false; 
    
    for (long i = 0; i < nBits; i++) {
        char c = binStr.charAt(i);
        if (c != '0' && c != '1') 
            return false;        
    }
    return true;
}

// Renderização dinâmica da string binária de saída com N bits
String paraStringBinaria(long valor, long nBits) {
    String str = "";
    for (long i = nBits - 1; i >= 0; i--) {
        str += ((valor >> i) & 0x01) ? "1" : "0";
    }
    return str;
}

long fatorial(long n) {
    if (n < 0) return 0; 
    if (n == 0 || n == 1) return 1;
    long result = 1;
    for (long i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

// Macros dinâmicas de verificação de limites baseadas no nBits atual da requisição
bool OverFlowPositivo(long value, long nBits) { return value > (1 << (nBits - 1)) - 1; }
bool OverFlowNegativo(long value, long nBits) { return value < -(1 << (nBits - 1)); }
bool HasOverFlow(long value, long nBits) { return OverFlowPositivo(value, nBits) || OverFlowNegativo(value, nBits); }

bool OverFlowSomaPositivo(long valA, long valB, long resultado, long nBits) { return valA > 0 && valB > 0 && OverFlowPositivo(resultado, nBits); }
bool OverFlowSomaNegativo(long valA, long valB, long resultado, long nBits) { return valA < 0 && valB < 0 && OverFlowNegativo(resultado, nBits); }
bool HasOverFlowSoma(long valA, long valB, long resultado, long nBits) {
    return OverFlowSomaPositivo(valA, valB, resultado, nBits) || OverFlowSomaNegativo(valA, valB, resultado, nBits);
}

void tratarCalculo() {
    // Permite que a requisição HTTP vinda do navegador quebre as travas de CORS
    server.sendHeader("Access-Control-Allow-Origin", "*");

    // Validação de segurança: verifica a existência dos parâmetros e do novo delimitador "bits"
    if (!server.hasArg("a") || !server.hasArg("b") || !server.hasArg("op") || !server.hasArg("bits")) {
        server.send(400, "application/json", "{\"error\":\"Parâmetros ausentes na requisição.\"}");
        return;
    }

    String paramA = server.arg("a");
    String paramB = server.arg("b");
    String op     = server.arg("op");
    long nBits     = atol(server.arg("bits").c_str());

    // Garante que o nBits fornecido esteja dentro de limites aceitáveis de hardware
    if (nBits < 2 || nBits > 64) {
        server.send(400, "application/json", "{\"error\":\"Quantidade de bits inválida (Use entre 2 e 24).\"}");
        return;
    }

    if (!isStringBinaryValidWifi(paramA, nBits) || (op != "fat" && !isStringBinaryValidWifi(paramB, nBits))) {
        server.send(400, "application/json", "{\"error\":\"Os operandos devem conter a quantidade exata de bits especificada.\"}");
        return;
    }

    long valA = converterParaLong(paramA, nBits);
    long valB = (op == "fat") ? 0 : converterParaLong(paramB, nBits);
    long resultado = 0;
    bool overflow = false;
    String operacaoExecutada = "";

    // 2. Engine da ULA Expandida para as 5 Operações do Laboratório
    unsigned long t_inicio = micros();
    if (op == "add") {
        resultado = valA + valB;
        operacaoExecutada = "( " + String(valA) + " ) + ( " + String(valB) + " )";
        overflow = HasOverFlowSoma(valA, valB, resultado, nBits);
    } 
    else if (op == "sub") {
        resultado = valA - valB;
        operacaoExecutada = "( " + String(valA) + " ) - ( " + String(valB) + " )";
        overflow = HasOverFlowSoma(valA, -valB, resultado, nBits);
    } 
    else if (op == "mul") {
        resultado = valA * valB;
        operacaoExecutada = "( " + String(valA) + " ) * ( " + String(valB) + " )";
        overflow = HasOverFlow(resultado, nBits);
    } 
    else if (op == "fat") {
        resultado = fatorial(valA);
        operacaoExecutada = String(valA) + "!";
        overflow = HasOverFlow(resultado, nBits);
    } 
    else if (op == "div") {
        operacaoExecutada = "( " + String(valA) + " ) / ( " + String(valB) + " )";
        if (valB == 0) {
            resultado = 0;
            overflow = true; // Impede quebra do processador e sinaliza erro de barramento
        } else {
            resultado = valA / valB;
            overflow = HasOverFlow(resultado, nBits);
        }
    } 
    else {
        server.send(400, "application/json", "{\"error\":\"Operação inválida. Use 'add', 'sub', 'mul', 'div' ou 'fat'.\"}");
        return;
    }

    unsigned long t_fim = micros();
    unsigned long tempo_us = t_fim - t_inicio;

    // Isola o barramento mascarado de N bits
    long resultadoMascarado = MascaraBits(resultado, nBits);

    // 3. Atualização física dos Atuadores (Mapeia os 4 bits mais baixos nos LEDs físicos)
    digitalWrite(LED_BIT0, (resultadoMascarado >> 0) & 0x01);
    digitalWrite(LED_BIT1, (resultadoMascarado >> 1) & 0x01);
    digitalWrite(LED_BIT2, (resultadoMascarado >> 2) & 0x01);
    digitalWrite(LED_BIT3, (resultadoMascarado >> 3) & 0x01);

    // 4. Construção da resposta JSON estruturada idêntica à esperada pelo Dashboard HTML
    String jsonResposta = "{";
    jsonResposta += "\"operacao\":\"" + operacaoExecutada + "\",";
    jsonResposta += "\"resultado_decimal\":" + String(resultado) + ",";
    jsonResposta += "\"resultado_bin\":\"" + paraStringBinaria(resultadoMascarado, nBits) + "\",";
    jsonResposta += "\"overflow\":" + String(overflow ? "true" : "false") + ",";
    jsonResposta += "\"tempo_us\":" + String(tempo_us);
    jsonResposta += "}";

    server.send(200, "application/json", jsonResposta);
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_BIT0, OUTPUT);
    pinMode(LED_BIT1, OUTPUT);
    pinMode(LED_BIT2, OUTPUT);
    pinMode(LED_BIT3, OUTPUT);

    // Teste Cego de Integridade Física do Hardware (Lógica de Inicialização)
    digitalWrite(LED_BIT0, HIGH); delay(150);
    digitalWrite(LED_BIT1, HIGH); delay(150);
    digitalWrite(LED_BIT2, HIGH); delay(150);
    digitalWrite(LED_BIT3, HIGH); delay(150);
    digitalWrite(LED_BIT0, LOW);  digitalWrite(LED_BIT1, LOW);
    digitalWrite(LED_BIT2, LOW);  digitalWrite(LED_BIT3, LOW);

    // Inicialização da Rede SoftAP do ESP32
    WiFi.softAP(ssid, password);
    Serial.println("Wi-Fi Iniciado com Sucesso!");
    Serial.print("IP do Servidor da Calculadora: ");
    Serial.println(WiFi.softAPIP());

    // Vincula a rota HTTP ao método de callback analítico
    server.on("/calc", HTTP_GET, tratarCalculo);

    server.begin();
    Serial.println("Servidor HTTP ativo na porta 80.");
}

void loop() {
    server.handleClient(); // Mantém a escuta do barramento de requisições de rede ativa
}