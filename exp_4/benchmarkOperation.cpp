#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdint>

// Converte string binária para inteiro com extensão de sinal dinâmica baseada em nBits
long converterParaLong(std::string binStr, long nBits) {
    long valor = strtol(binStr.c_str(), NULL, 2); 
    long mascaraValida = (1 << nBits) - 1; 
    valor = valor & mascaraValida; 

    if (valor & (1 << (nBits - 1))) {
        long mascaraExtensaoSinal = ~mascaraValida;
        return (long)(valor | mascaraExtensaoSinal); 
    }
    return (long)valor;
}

// Retorna a máscara baseada no número dinâmico de bits
long obterMascaraBits(long value, long nBits) { 
    return value & ((1 << nBits) - 1); 
}

// Retorna o valor interpretado com sinal no escopo de N_BITS
long converterParaSinalNBits(long value, long nBits) {
    long mascarado = obterMascaraBits(value, nBits);
    if (mascarado & (1 << (nBits - 1))) {
        return mascarado - (1 << nBits);
    }
    return mascarado;
}

// Função de processamento iterativo do Fatorial
long fatorial(long n) {
    if (n < 0) return 0; // Fatorial não definido para números negativos
    if (n == 0 || n == 1) return 1;
    long result = 1;
    for (long i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

// Limites dinâmicos baseados no teto de complemento de dois para N bits
bool OverFlowPositivo(long value, long nBits) { return value > (1 << (nBits - 1)) - 1; }
bool OverFlowNegativo(long value, long nBits) { return value < -(1 << (nBits - 1)); }
bool HasOverFlow(long value, long nBits) { return OverFlowPositivo(value, nBits) || OverFlowNegativo(value, nBits); }

// Detecção de overflow na adição e subtração baseada em inversão de sinal
bool verificarHasOverFlowSoma(long valA, long valB, long resultado, long nBits) {
    long resNBits = converterParaSinalNBits(resultado, nBits);
    return ((valA > 0 && valB > 0 && resNBits <= 0) || (valA < 0 && valB < 0 && resNBits >= 0));
}

int main(int argc, char* argv[]) {
    // Formato esperado pelo script Python: ./ula_core [N_BITS] [BIN_A] [BIN_B] [OPERACAO]
    if (argc < 5) {
        std::cerr << "Erro: Parâmetros insuficientes. Use: ./ula_core [N_BITS] [BIN_A] [BIN_B] [OPERACAO]" << std::endl;
        return 1;
    }

    // 1. Captura de argumentos passados pelo orquestrador Python
    long nBits = std::atoi(argv[1]);
    std::string binA = argv[2];
    std::string binB = argv[3];
    std::string op = argv[4];

    // 2. Processamento de conversão com extensão de sinal genérica
    long valA = converterParaLong(binA, nBits);
    long valB = converterParaLong(binB, nBits);
    long resultado = 0;
    bool overflow = false;

    // 3. Estrutura de Controle Alinhada com as 5 Operações do Laboratório
    if (op == "add") {
        resultado = valA + valB;
        overflow = verificarHasOverFlowSoma(valA, valB, resultado, nBits);
    } 
    else if (op == "sub") {
        resultado = valA - valB;
        // Evita estouro de inversão unária (-valB) avaliando os sinais de hardware
        long resNBits = converterParaSinalNBits(resultado, nBits);
        if ((valA >= 0 && valB < 0 && resNBits < 0) || (valA < 0 && valB > 0 && resNBits > 0)) {
            overflow = true;
        }
    } 
    else if (op == "mul") {
        resultado = valA * valB;
        overflow = HasOverFlow(resultado, nBits);
    } 
    else if (op == "fat") {
        resultado = fatorial(valA);
        overflow = HasOverFlow(resultado, nBits);
        valB = 0; // Normaliza o valor secundário para o dump do log no Python
    } 
    else if (op == "div") {
        if (valB == 0) {
            resultado = 0;
            overflow = true; // Força flag de erro de barramento para divisão por zero
        } else {
            resultado = valA / valB;
            overflow = HasOverFlow(resultado, nBits); // Trata o caso crítico (-2^(N-1) / -1)
        }
    } 
    else {
        std::cerr << "Erro: Operação desconhecida." << std::endl;
        return 1;
    }

    // 4. Aplicação do Mascaramento do Barramento Final
    long resultadoMascarado = obterMascaraBits(resultado, nBits);

    // 5. Impressão Padronizada (CSV/String) capturada via pipeline de processo pelo Python
    // Formato: valA,valB,resultadoBruto,resultadoMascarado,overflow
    std::cout << valA << "," << valB << "," << resultado << "," << resultadoMascarado << "," << (overflow ? "1" : "0") << std::endl;

    return 0;
}