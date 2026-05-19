#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdint>

// Adaptando a função substituindo a String do Arduino por std::string do C++
int8_t converterParaInteiro(std::string binStr) {
    long valor = strtol(binStr.c_str(), NULL, 2); 
    if (valor & 0x08) {
        return (int8_t)(0xF0 | valor); 
    }
    return (int8_t)(valor & 0x0F);
}

// CORREÇÃO: Alterado de String para std::string
std::string paraStringBinaria(int8_t valor) {
    std::string str = "";
    for (int i = 3; i >= 0; i--) {
        // Concatena caracteres puros ('1' ou '0') na std::string
        str += ((valor >> i) & 0x01) ? '1' : '0';
    }
    return str;
}

bool OverFlowPositivo(int8_t valA, int8_t valB, int8_t resultado) { return valA > 0 && valB > 0 && resultado > 7; }
bool OverFlowNegativo(int8_t valA, int8_t valB, int8_t resultado) { return valA < 0 && valB < 0 && resultado < -8;}
bool HasOverFlow(int8_t valA, int8_t valB, int8_t resultado) {
    return OverFlowPositivo(valA, valB, resultado) || OverFlowNegativo(valA, valB, resultado);
}

// Função de teste automatizado
void rodarTeste(std::string a_str, std::string b_str, std::string op) {
    int8_t valA = converterParaInteiro(a_str);
    int8_t valB = converterParaInteiro(b_str);
    int8_t resultado = 0;
    bool overflow = false;

    if (op == "add") {
        resultado = valA + valB;

        overflow = HasOverFlow(valA, valB, resultado);
    } else if (op == "sub") {
        resultado = valA - valB;
        
        overflow = HasOverFlow(valA, -valB, resultado);
    }

    // Mascara o resultado final para 4 bits para exibição correta da string binária
    int8_t resultadoMascarado = resultado & 0x0F;

    // Trocado \n por std::endl para forçar a impressão imediata no terminal do Windows
    std::cout << "Operacao: " << a_str << " (" << (int)valA << ") " 
              << (op == "add" ? "+" : "-") << " " 
              << b_str << " (" << (int)valB << ") = " 
              << "( " << paraStringBinaria(resultadoMascarado) << " ) " 
              << (int)resultado << " | Overflow: " << (overflow ? "SIM" : "NAO") << std::endl;
}

int main() {
    // Trocado \n por std::endl para garantir o flush inicial do buffer
    std::cout << "--- CASOS DE TESTE DA ULA DE 4 BITS ---" << std::endl;
    
    // Teste 1: 3 + 2 = 5 (OK) [cite: 232]
    rodarTeste("0011", "0010", "add"); 
    
    // Teste 2: 7 + 2 = 9 (Deve acusar Overflow -> Invasão de bit de sinal gerando -7) [cite: 219, 232]
    rodarTeste("0111", "0010", "add"); 
    
    // Teste 3: -8 - 1 = -9 (Deve acusar Overflow -> Invasão de bit de sinal gerando +7) [cite: 232]
    rodarTeste("1000", "0001", "sub"); 

    return 0;
}