#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdint>

// Converte string binária para inteiro com extensão de sinal dinâmica baseada em nBits
int converterParaInteiro(std::string binStr, int nBits) {
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

int fatorial(int n) {
    if (n < 0) return 0; // Fatorial não definido para números negativos
    if (n == 0 || n == 1) return 1;
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

std::string paraStringBinaria(int valor, int nBits) {
    std::string str = "";
    for (int i = nBits - 1; i >= 0; i--) {
        str += ((valor >> i) & 0x0001) ? '1' : '0';
    }
    return str;
}

// Funções de overflow ajustadas para receber nBits como parâmetro
bool OverFlowPositivo(int value, int nBits) { return value > (1 << (nBits - 1)) - 1; }
bool OverFlowNegativo(int value, int nBits) { return value < -(1 << (nBits - 1)); }
bool HasOverFlow(int value, int nBits) { return OverFlowPositivo(value, nBits) || OverFlowNegativo(value, nBits); }

bool OverFlowSomaPositivo(int valA, int valB, int resultado, int nBits) { return valA > 0 && valB > 0 && OverFlowPositivo(resultado, nBits); }
bool OverFlowSomaNegativo(int valA, int valB, int resultado, int nBits) { return valA < 0 && valB < 0 && OverFlowNegativo(resultado, nBits); }
bool HasOverFlowSoma(int valA, int valB, int resultado, int nBits) {
    return OverFlowSomaPositivo(valA, valB, resultado, nBits) || OverFlowSomaNegativo(valA, valB, resultado, nBits);
}

// Função de teste automatizado recebendo nBits parametricamente
void rodarTeste(int nBits, std::string a_str, std::string b_str, std::string op) {
    int valA = converterParaInteiro(a_str, nBits);
    int valB = converterParaInteiro(b_str, nBits);
    int resultado = 0;
    bool overflow = false;
    std::string sinalOp = "";

    if (op == "add") {
        resultado = valA + valB;
        overflow = HasOverFlowSoma(valA, valB, resultado, nBits);
        sinalOp = "+";
    } else if (op == "sub") {
        resultado = valA - valB;
        overflow = HasOverFlowSoma(valA, -valB, resultado, nBits);
        sinalOp = "-";
    } else if (op == "mul") {
        resultado = valA * valB;
        overflow = HasOverFlow(resultado, nBits);
        sinalOp = "*";
    } else if (op == "fat") {
        resultado = fatorial(valA);
        overflow = HasOverFlow(resultado, nBits);
        sinalOp = "!";
    } else if (op == "div") {
        sinalOp = "/";
        if (valB == 0) {
            resultado = 0;
            overflow = true; 
        } else {
            resultado = valA / valB;
            overflow = HasOverFlow(resultado, nBits);
        }
    } else {
        std::cout << "Operação inválida." << std::endl;
        return;
    }

    int resultadoMascarado = MascaraBits(resultado, nBits);

    std::cout << "[" << nBits << " BITS] Operacao: ";
    if (op == "fat") {
        std::cout << (int)valA << " ( " << paraStringBinaria(MascaraBits(valA, nBits), nBits) << " )" << "!" << " = ";
    } else {
        std::cout << (int)valA << " ( " << paraStringBinaria(MascaraBits(valA, nBits), nBits) << " )" << " " << sinalOp << " " << (int)valB << " ( " << paraStringBinaria(MascaraBits(valB, nBits), nBits) << " )" << " = ";
    }
    std::cout << (int)resultado << " ( " << paraStringBinaria(resultadoMascarado, nBits) << " )" << " | Overflow: " << (overflow ? "SIM" : "NAO") << std::endl;
}

int main() {
    std::cout << "========================================================" << std::endl;
    std::cout << "--- BATERIA DE TESTES COM N_BITS PARAMÉTRICO ---" << std::endl;
    std::cout << "========================================================" << std::endl;
    
    // --- GRUPO 1: CENÁRIOS EM 4 BITS (Limites: -8 a +7) ---
    std::cout << "\n>>> EXECUTANDO TESTES EM 4 BITS <<<" << std::endl;
    rodarTeste(4, "0011", "0010", "add"); // 3 + 2 = 5 (OK)
    rodarTeste(4, "0111", "0010", "add"); // 7 + 2 = 9 (Overflow)
    rodarTeste(4, "0011", "0011", "mul"); // 3 * 3 = 9 (Overflow)
    rodarTeste(4, "0110", "0010", "div"); // 6 / 2 = 3 (OK)
    rodarTeste(4, "0100", "0000", "fat"); // 4! = 24 (Overflow)

    // --- GRUPO 2: CENÁRIOS EM 8 BITS (Limites: -128 a +127) ---
    std::cout << "\n>>> EXECUTANDO TESTES EM 8 BITS <<<" << std::endl;
    // Em 4 bits 7+2 dava overflow. Aqui em 8 bits ("00000111" + "00000010") deve dar OK!
    std::cout << "Teste de escala (7 + 2 em 8 bits):" << std::endl;
    rodarTeste(8, "00000111", "00000010", "add"); 
    
    // Em 4 bits 4! dava overflow. Aqui em 8 bits (4! = 24) cabe com folga!
    std::cout << "Teste de escala (4! em 8 bits):" << std::endl;
    rodarTeste(8, "00000100", "00000000", "fat"); 

    // Multiplicação gerando estouro para 8 bits: 20 * 10 = 200 (Estoura +127)
    std::cout << "Teste de estouro em 8 bits (20 * 10 = 200):" << std::endl;
    rodarTeste(8, "00010100", "00001010", "mul"); 

    // --- GRUPO 3: CENÁRIOS EM 16 BITS (Limites: -32768 a +32767) ---
    std::cout << "\n>>> EXECUTANDO TESTES EM 16 BITS <<<" << std::endl;
    // O teste anterior que quebrou em 8 bits (20 * 10 = 200), agora em 16 bits passa liso
    std::cout << "Teste de escala (20 * 10 em 16 bits):" << std::endl;
    rodarTeste(16, "0000000000010100", "0000000000001010", "mul");

    // Fatorial de 8 (8! = 40320) excede o teto de +32767, gerando overflow em 16 bits
    std::cout << "Teste de estouro em 16 bits (8! = 40320):" << std::endl;
    rodarTeste(16, "0000000000001000", "0000000000000000", "fat");

    return 0;
}