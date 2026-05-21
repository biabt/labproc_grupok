#include <Arduino.h>

int8_t obterDecimalDoComp1(uint8_t bits) {
    if (bits & 0x08) {
        return -((~bits) & 0x0F);
    }
    return (int8_t)(bits & 0x0F);
}

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

bool HasOverflow(int8_t a, int8_t b, int8_t resultado) {
    return (a > 0 && b > 0 && resultado <= 0) || 
           (a < 0 && b < 0 && resultado >= 0);
}

uint8_t somarComplementoDeUm(uint8_t aBits, uint8_t bBits, bool& gerouCarryOut) {
    uint16_t somaPura = (aBits & 0x0F) + (bBits & 0x0F);
    
    if (somaPura & 0x10) {
        gerouCarryOut = true;
        somaPura = (somaPura & 0x0F) + 1;
    } else {
        gerouCarryOut = false;
    }
    
    return (uint8_t)(somaPura & 0x0F);
}


void setup() {
    Serial.begin(115200);


    Serial.println(F("===================================================="));
    Serial.println(F("CALCULADORA DE 4 BITS - COMPLEMENTO DE 1"));
    Serial.println(F("===================================================="));
    Serial.println(F("Digite o comando no formato exato: A,B,OP"));
}

void loop() {
    if (Serial.available() > 0) {
        String entradaRaw = Serial.readString();
        entradaRaw.trim();

        int primeiraVirgula = entradaRaw.indexOf(',');
        int segundaVirgula  = entradaRaw.indexOf(',', primeiraVirgula + 1);

        if (primeiraVirgula == -1 || segundaVirgula == -1) {
            Serial.println(F("[ERRO] Formato inválido! Certifique-se de usar: A,B,OP (Ex: 0011,0010,add)"));
            return;
        }

        String paramA = entradaRaw.substring(0, primeiraVirgula);
        String paramB = entradaRaw.substring(primeiraVirgula + 1, segundaVirgula);
        String op     = entradaRaw.substring(segundaVirgula + 1);
        
        paramA.trim();
        paramB.trim();
        op.trim();

        if (!isStringBinaryValid(paramA) || !isStringBinaryValid(paramB)) {
            Serial.println(F("[ERRO] Os operandos devem conter exatamente 4 caracteres binários (0 ou 1)."));
            return;
        }

        // 1. Conversão das entradas strings para o tipo inteiro estrito de 8 bits
        uint8_t aBits = strtol(paramA.c_str(), NULL, 2) & 0x0F;
        uint8_t bBits = strtol(paramB.c_str(), NULL, 2) & 0x0F;
        int8_t resultado = 0;
        bool carryOutAtivo = false;
        bool overflow = false;

        // 2. Processamento Analítico da ULA em Complemento de Um 
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
        else {
            Serial.println(F("[ERRO] Operação desconhecida. Use 'add' para SOMA ou 'sub' para SUBTRAÇÃO."));
            return;
        }
        overflow = HasOverflow(obterDecimalDoComp1(aBits), obterDecimalDoComp1(bBits), obterDecimalDoComp1(resultadoBits));


        Serial.println(F("----------------------------------------------------"));
        Serial.print(F("Operação: ")); Serial.print(valA); Serial.print(op == "add" ? " + " : " - "); Serial.println(valB);
        Serial.print(F("Bits de Entrada: [A]: ")); Serial.print(paramA); Serial.print(F(" | [B]: ")); Serial.println(paramB);
        Serial.print(F("End-Around Carry (Carry Out): ")); Serial.println(carryOutAtivo ? F("1") : F("0"));
        Serial.print(F("Resultado Decimal: ")); Serial.println((int)resultado);
        Serial.print(F("Resultado Binário (Comp. 1): ")); Serial.println(paraStringBinariaComp1(resultado));
        if (overflow) {
            Serial.println(F("HOUVE OVERFLOW!"));
        } else {
            Serial.println(F("SEM OVERFLOW"));
        }
        Serial.println(F("----------------------------------------------------\n"));
    }
}