#include <WiFi.h>
#include <WebServer.h>

// Mapeamento físico dos GPIOs para os LEDs de monitoramento
#define LED_BIT0 6
#define LED_BIT1 7
#define LED_BIT2 8
#define LED_BIT3 9

WebServer server(80);

// Configuração do Ponto de Acesso (Access Point) do ESP32
const char *ssid = "Calculadora_ESP32";
const char *password = "";

// Conversão com extensão de sinal dinâmica baseada em nBits
int converterParaInteiro(String binStr, int nBits) {
    long valor = strtol(binStr.c_str(), NULL, 2); 
    long mascaraValida = (1 << nBits) - 1; 
    valor = valor & mascaraValida; 

    if (valor & (1 << (nBits - 1))) {
        long mascaraExtensaoSinal = ~mascaraValida;
        return (int)(valor | mascaraExtensaoSinal); 
    }
    return (int)valor;
}

int MascaraBits(int value, int nBits) { 
    return value & ((1 << nBits) - 1); 
}

// Validação flexível que aceita strings binárias de qualquer tamanho nBits
bool isStringBinaryValidWifi(String binStr, int nBits) {
    if (binStr.length() != nBits) 
        return false; 
    
    for (int i = 0; i < nBits; i++) {
        char c = binStr.charAt(i);
        if (c != '0' && c != '1') 
            return false;        
    }
    return true;
}

// Renderização dinâmica da string binária de saída com N bits
String paraStringBinaria(int valor, int nBits) {
    String str = "";
    for (int i = nBits - 1; i >= 0; i--) {
        str += ((valor >> i) & 0x01) ? "1" : "0";
    }
    return str;
}

int fatorial(int n) {
    if (n < 0) return 0; 
    if (n == 0 || n == 1) return 1;
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

// Macros dinâmicas de verificação de limites baseadas no nBits atual da requisição
bool OverFlowPositivo(int value, int nBits) { return value > (1 << (nBits - 1)) - 1; }
bool OverFlowNegativo(int value, int nBits) { return value < -(1 << (nBits - 1)); }
bool HasOverFlow(int value, int nBits) { return OverFlowPositivo(value, nBits) || OverFlowNegativo(value, nBits); }

bool OverFlowSomaPositivo(int valA, int valB, int resultado, int nBits) { return valA > 0 && valB > 0 && OverFlowPositivo(resultado, nBits); }
bool OverFlowSomaNegativo(int valA, int valB, int resultado, int nBits) { return valA < 0 && valB < 0 && OverFlowNegativo(resultado, nBits); }
bool HasOverFlowSoma(int valA, int valB, int resultado, int nBits) {
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
    int nBits     = server.arg("bits").toInt();

    // Garante que o nBits fornecido esteja dentro de limites aceitáveis de hardware
    if (nBits < 2 || nBits > 24) {
        server.send(400, "application/json", "{\"error\":\"Quantidade de bits inválida (Use entre 2 e 24).\"}");
        return;
    }

    if (!isStringBinaryValidWifi(paramA, nBits) || (op != "fat" && !isStringBinaryValidWifi(paramB, nBits))) {
        server.send(400, "application/json", "{\"error\":\"Os operandos devem conter a quantidade exata de bits especificada.\"}");
        return;
    }

    int valA = converterParaInteiro(paramA, nBits);
    int valB = (op == "fat") ? 0 : converterParaInteiro(paramB, nBits);
    int resultado = 0;
    bool overflow = false;
    String operacaoExecutada = "";

    // 2. Engine da ULA Expandida para as 5 Operações do Laboratório
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

    // Isola o barramento mascarado de N bits
    int resultadoMascarado = MascaraBits(resultado, nBits);

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