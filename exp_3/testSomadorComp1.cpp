#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <cmath>

// 1. Extrai o valor decimal com sinal respeitando estritamente o bit 3 como sinal do Complemento de Um
int8_t obterDecimalDoComp1(uint8_t bits) {
    if (bits & 0x08) {
        return -((~bits) & 0x0F);
    }
    return (int8_t)(bits & 0x0F);
}

// 2. Converte um inteiro com sinal de volta para string binária seguindo as regras do Comp. de 1
std::string paraStringBinariaComp1(int8_t valor) {
    uint8_t bits = 0;
    if (valor < 0) {
        bits = (~(std::abs(valor))) & 0x0F;
    } else {
        bits = valor & 0x0F;
    }

    std::string str = "";
    for (int i = 3; i >= 0; i--) {
        str += ((bits >> i) & 0x01) ? '1' : '0';
    }
    return str;
}

uint8_t somarComplementoDeUm(uint8_t aBits, uint8_t bBits, bool& gerouCarryOut) {
    // Soma armazenada em container de 16 bits para capturar o estouro no bit 4 (Carry Out)
    uint16_t somaPura = (aBits & 0x0F) + (bBits & 0x0F);

    if (somaPura & 0x10) {
        gerouCarryOut = true;
        somaPura = (somaPura & 0x0F) + 1;
    } else {
        gerouCarryOut = false;
    }
    
    return (uint8_t)(somaPura & 0x0F);
}

// 4. Função de Injeção de Sinais e Automação dos Casos de Teste
void rodarTeste(std::string a_str, std::string b_str, std::string op) {
    // CORREÇÃO CRUCIAL: Captura os bits originais da string sem passar por conversor prévio
    uint8_t aBits = strtol(a_str.c_str(), NULL, 2) & 0x0F;
    uint8_t bBits = strtol(b_str.c_str(), NULL, 2) & 0x0F;
    
    uint8_t resultadoBits = 0;
    bool carryOutAtivo = false;
    bool overflow = false;

    if (op == "add") {
        resultadoBits = somarComplementoDeUm(aBits, bBits, carryOutAtivo);
    } 
    else if (op == "sub") {
        // Na subtração de hardware, B é invertido bit a bit (NOT) antes de entrar no somador
        uint8_t bInvertidoBits = (~bBits) & 0x0F;
        resultadoBits = somarComplementoDeUm(aBits, bInvertidoBits, carryOutAtivo);
        
        // Atualiza bBits para o valor invertido para que a análise de overflow de soma funcione
        bBits = bInvertidoBits;
    }

    // Converte os estados de bits finais para decimais apenas para exibição e teste de overflow
    int8_t valA = obterDecimalDoComp1(aBits);
    int8_t valB = obterDecimalDoComp1(bBits);
    int8_t resultadoDecimal = obterDecimalDoComp1(resultadoBits);

    // Regra Universal de Overflow baseada em Inversão de Sinal Involuntária
    // (Dois positivos geram negativo OU dois negativos geram positivo)
    if ((valA > 0 && valB > 0 && resultadoDecimal <= 0) || 
        (valA < 0 && valB < 0 && resultadoDecimal >= 0)) {
        overflow = true;
    }

    // Se a operação original era subtração, recalculamos o valB decimal real apenas para o print
    int8_t valBExibicao = obterDecimalDoComp1(strtol(b_str.c_str(), NULL, 2) & 0x0F);

    // Exibição formatada no terminal
    std::cout << "Entrada Binaria:  [A]: " << a_str << " (" << (int)valA << ") " 
              << (op == "add" ? "+" : "-") << " [B]: " << b_str << " (" << (int)valBExibicao << ")" << std::endl;
    std::cout << "End-Around Carry: " << (carryOutAtivo ? "SIM (+1 no LSB)" : "NAO") << std::endl;
    std::cout << "Saida da ULA:     ( " << paraStringBinariaComp1(resultadoDecimal) << " ) Decimal: " << (int)resultadoDecimal << std::endl;
    std::cout << "Status:           OVERFLOW: " << (overflow ? "SIM" : "NAO") << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "--- CASOS DE TESTE DA ULA: DESAFIO COMPLEMENTO DE 1 (4 BITS) ---" << std::endl;
    std::cout << "================================================================" << std::endl;
    
    // Teste 1: 3 + 2 = 5 (Operação Normal, sem carry)
    std::cout << "[Caso 1] Esperado: 3 + 2 = 5 | Carry: NAO | Overflow: NAO" << std::endl;
    rodarTeste("0011", "0010", "add"); 
    
    // Teste 2: 7 + 2 = 9 (Overflow Positivo)
    std::cout << "[Caso 2] Esperado: 7 + 2 = 9 | Carry: NAO | Overflow: SIM" << std::endl;
    rodarTeste("0111", "0010", "add"); 

    // Teste 3: -4 + (-3) = -7 (Operação com Negativos, gera Carry out, sem Overflow)
    std::cout << "[Caso 3] Esperado: (-4) + (-3) = -7 | Carry: SIM | Overflow: NAO" << std::endl;
    rodarTeste("1011", "1100", "add"); 
    
    // Teste 4: -3 - (-4) = +1 (Subtração Normal, gera Carry na soma interna)
    std::cout << "[Caso 4] Esperado: (-3) - (-4) = 1 | Carry: SIM | Overflow: NAO" << std::endl;
    rodarTeste("1100", "1011", "sub"); 

    // Teste 5: -7 - 1 = -8 -> Estoura o limite de -7, gerando Overflow
    std::cout << "[Caso 5] Esperado: (-7) - 1 = -8 | Carry: SIM | Overflow: SIM" << std::endl;
    rodarTeste("1000", "0001", "sub"); 

    // Teste 6: 6 - (-3) = 9 (Overflow Positivo via Subtração)
    std::cout << "[Caso 6] Esperado: 6 - (-3) = 9 | Carry: NAO | Overflow: SIM" << std::endl;
    rodarTeste("0110", "1100", "sub");

    // Teste 7: Operação envolvendo o Zero Negativo (1111) como elemento neutro
    std::cout << "[Caso 7] Esperado: 3 + (-0) = 3 | Carry: SIM | Overflow: NAO" << std::endl;
    rodarTeste("0011", "1111", "add"); 

    return 0;
}