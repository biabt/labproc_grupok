#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdint>

#define N_BITS 4

// Adaptando a função substituindo a String do Arduino por std::string do C++
int converterParaInteiro(std::string binStr) {
    long valor = strtol(binStr.c_str(), NULL, 2); 

    long mascaraValida = (1 << N_BITS) - 1; //(Ex: se nBits=4, vira 1111)
    valor = valor & mascaraValida; // Filtra a mascara com os bits validos (ex: 10101 vira 0101)

    if (valor & (1 << (N_BITS - 1))) {
        long mascaraExtensaoSinal = ~mascaraValida;
        
        return (int)(valor | mascaraExtensaoSinal); 
    }
    
    return (int)valor;
}

int MascaraBits(int value) { return value & ((1 << N_BITS) - 1); }

int fatorial(int n) {
    if (n < 0) return 0; // Fatorial não definido para números negativos
    if (n == 0 || n == 1) return 1;
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

// CORREÇÃO: Alterado de String para std::string
std::string paraStringBinaria(int valor) {
    std::string str = "";
    for (int i = N_BITS - 1; i >= 0; i--) {
        // Concatena caracteres puros ('1' ou '0') na std::string
        str += ((valor >> i) & 0x0001) ? '1' : '0';
    }
    return str;
}
bool OverFlowPositivo(int value) { return value > (1 << N_BITS - 1) - 1; }
bool OverFlowNegativo(int value) { return value < -(1 << N_BITS - 1); }
bool HasOverFlow(int value) { return OverFlowPositivo(value) || OverFlowNegativo(value); }
bool OverFlowSomaPositivo(int valA, int valB, int resultado) { return valA > 0 && valB > 0 && OverFlowPositivo(resultado); }
bool OverFlowSomaNegativo(int valA, int valB, int resultado) { return valA < 0 && valB < 0 && OverFlowNegativo(resultado); }
bool HasOverFlowSoma(int valA, int valB, int resultado) {
    return OverFlowSomaPositivo(valA, valB, resultado) || OverFlowSomaNegativo(valA, valB, resultado);
}

// Função de teste automatizado
void rodarTeste(std::string a_str, std::string b_str, std::string op) {
    int valA = converterParaInteiro(a_str);
    int valB = converterParaInteiro(b_str);
    int resultado = 0;
    bool overflow = false;
    std::string sinalOp = "";

    if (op == "add") {
        resultado = valA + valB;
        overflow = HasOverFlowSoma(valA, valB, resultado);
        sinalOp = "+";
    } else if (op == "sub") {
        resultado = valA - valB;
        overflow = HasOverFlowSoma(valA, -valB, resultado);
        sinalOp = "-";
    } else if (op == "mul") {
        resultado = valA * valB;
        overflow = HasOverFlow(resultado);
        sinalOp = "*";
    } else if (op == "fat") {
        resultado = fatorial(valA);
        overflow = HasOverFlow(resultado);
        sinalOp = "!";
    } else {
        std::cout << "Operação inválida." << std::endl;
        return;
    }

    // Mascara o resultado final para 4 bits para exibição correta da string binária
    int resultadoMascarado = MascaraBits(resultado);

    // Trocado \n por std::endl para forçar a impressão imediata no terminal do Windows
    std::cout << "Operacao: ";
    if (op == "fat") {
        std::cout << (int)valA << " ( " << paraStringBinaria(valA) << " )" << "!" << " = ";
    } else {
        std::cout << (int)valA << " ( " << paraStringBinaria(valA) << " )" << " " << sinalOp << " " << (int)valB << " ( " << paraStringBinaria(valB) << " )" << " = ";
    }
    std::cout << (int)resultado << " ( " << paraStringBinaria(resultado) << " )" << " | Overflow: " << (overflow ? "SIM" : "NAO") << std::endl;
}

int main() {
    // Trocado \n por std::endl para garantir o flush inicial do buffer
    std::cout << "--- CASOS DE TESTE DA ULA DE 4 BITS ---" << std::endl;
    
    // Teste 1: 3 + 2 = 5 (OK) [cite: 232]
    std::cout << "Teste 1 - Resultado esperado: 3 + 2 = 5, Overflow: NAO" << std::endl;
    rodarTeste("0011", "0010", "add"); 
    
    // Teste 2: 7 + 2 = 9 (Deve acusar Overflow -> Invasão de bit de sinal gerando -7)
    std::cout << "Teste 2 - Resultado esperado: 7 + 2 = 9, Overflow: SIM" << std::endl;
    rodarTeste("0111", "0010", "add"); 

    std::cout << "Teste 3 - Resultado esperado: (-5) + (-4) = -9, Overflow: SIM" << std::endl;
    rodarTeste("1011", "1100", "add"); 
    
    std::cout << "Teste 4 - Resultado esperado: (-8) - (-5) = -3, Overflow: NAO" << std::endl;
    rodarTeste("1000", "1011", "sub"); 

    std::cout << "Teste 5 - Resultado esperado: (-8) - (1) = -9, Overflow: SIM" << std::endl;
    rodarTeste("1000", "0001", "sub"); 

    std::cout << "Teste 6 - Resultado esperado: (6) - (-3) = 9, Overflow: SIM" << std::endl;
    rodarTeste("0110", "1011", "sub");

    std::cout << "Teste 7 - Resultado esperado: (-2) - (-8) = 6, Overflow: NAO" << std::endl;
    rodarTeste("1110", "1000", "sub"); 

    std::cout << "\n[NOVOS] MULTIPLICACAO:" << std::endl;
    // Teste 8: 2 * 3 = 6 (OK)
    std::cout << "Teste 8 - Esperado: 2 * 3 = 6, Overflow: NAO" << std::endl;
    rodarTeste("0010", "0011", "mul");

    // Teste 9: 3 * 3 = 9 (Overflow Positivo -> 9 excede 7)
    std::cout << "Teste 9 - Esperado: 3 * 3 = 9, Overflow: SIM" << std::endl;
    rodarTeste("0011", "0011", "mul");

    // Teste 10: -2 * 3 = -6 (OK)
    std::cout << "Teste 10 - Esperado: -2 * 3 = -6, Overflow: NAO" << std::endl;
    rodarTeste("1110", "0011", "mul");

    // Teste 11: -4 * 3 = -12 (Overflow Negativo -> -12 ultrapassa -8)
    std::cout << "Teste 11 - Esperado: -4 * 3 = -12, Overflow: SIM" << std::endl;
    rodarTeste("1100", "0011", "mul");

    std::cout << "\n[NOVOS] FATORIAL (Limites: -8 a +7):" << std::endl;
    // Teste 12: 3! = 6 (OK)
    std::cout << "Teste 12 - Esperado: 3! = 6, Overflow: NAO" << std::endl;
    rodarTeste("0011", "0000", "fat");

    // Teste 13: 4! = 24 (Overflow Positivo -> 24 excede 7)
    std::cout << "Teste 13 - Esperado: 4! = 24, Overflow: SIM" << std::endl;
    rodarTeste("0100", "0000", "fat");

    // Teste 14: 0! = 1 (OK)
    std::cout << "Teste 14 - Esperado: 0! = 1, Overflow: NAO" << std::endl;
    rodarTeste("0000", "0000", "fat");

    return 0;
}