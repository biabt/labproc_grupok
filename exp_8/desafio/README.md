# Desafio: Calculadora com Matrix Keypad 4x4 + LCD1602

## Descrição

Uma versão alternativa da calculadora binária que utiliza:
- **Entrada**: Matrix Keypad 4x4 via I2C (Freenove Keypad library)
- **Saída**: Display LCD1602 via I2C (PCF8574 expander)
- **Processamento**: ULA em Assembly ARM64 (alu.s)

Este projeto substitui a interface SDL2 (janela gráfica) por uma interface de hardware embarcado completamente via I2C, adequada para operação headless em Raspberry Pi 3.

---

## Arquitetura

```
[Matrix Keypad 4x4]     [LCD1602 via I2C]
       ↓                       ↑
   (I2C 0x48)           (I2C via PCF8574 0x27)
       ↓                       ↑
       └─→ [desafio.c] ←──────┘
               ↓
           [ULA - alu.s]
         (alu_add/sub/mul/div/fat)
```

---

## Mapeamento de Teclas

### Matriz 4x4 (linhas × colunas)

```
┌─────┬─────┬─────┬─────┐
│  1  │  2  │  3  │  A  │  (A = +)
├─────┼─────┼─────┼─────┤
│  4  │  5  │  6  │  B  │  (B = -)
├─────┼─────┼─────┼─────┤
│  7  │  8  │  9  │  C  │  (C = *)
├─────┼─────┼─────┼─────┤
│  *  │  0  │  #  │  D  │  (* = !, # = =, D = /)
└─────┴─────┴─────┴─────┘
```

| Tecla | Ação |
|-------|------|
| 0-9   | Digitar número |
| A     | Adição (+) |
| B     | Subtração (-) |
| C     | Multiplicação (*) |
| D     | Divisão (/) |
| #     | Executar operação (=) |
| *     | Fatorial (!) |

---

## Exibição no LCD (2 linhas × 16 caracteres)

```
Linha 1: A=42   +  B=8  
Linha 2: Res:50            
```

- **Linha 1**: Mostra operandos (A, B) e operação pendente
- **Linha 2**: Mostra resultado ou mensagem de erro

### Mensagens de Erro

- `OVERFLOW` — Resultado excede 32 bits
- `DIV/0` — Divisão por zero
- `ERROR` — Erro na operação

---

## Configuração de Hardware

### Endereços I2C do Sistema

| Componente | Endereço I2C | Descrição |
|-----------|---|---|
| LCD1602 | `0x27` ou `0x3F` | PCF8574 (expansor I2C para LCD) |
| Matrix Keypad 4x4 | `0x48` | Keypad I2C (Freenove) |

O programa tenta auto-detectar o endereço do LCD automaticamente (0x27 primeiro, depois 0x3F).
A classe Keypad (Freenove) gerencia o endereço 0x48 automaticamente.

---

## Compilação

### Pré-requisitos

```bash
# Instalar ferramentas I2C e wiringPi
sudo apt install -y i2c-tools wiringpi libwiringpi-dev build-essential

# Ter o Freenove Kit instalado em:
# ~/Freenove_Kit/Code/C_Code/21_MatrixKeypad/
```

### Testar I2C antes de compilar

Conecte o LCD1602 e verifique se o sistema o reconhece:

```bash
# Escanear barramento I2C
i2cdetect -y 1
```

Se conectado, o endereço (geralmente `27` ou `3F`) aparecerá na tabela.

### Compilar do diretório raiz (exp_8)

```bash
make desafio
```

Comando equivalente (executado diretamente):
```bash
g++ -Wall -O2 desafio/desafio.c alu.s \
    ~/Freenove_Kit/Code/C_Code/21_MatrixKeypad/Keypad.cpp \
    ~/Freenove_Kit/Code/C_Code/21_MatrixKeypad/Key.cpp \
    -o desafio/desafio_arm -lwiringPi -lwiringPiDev
```

Resultado: `desafio/desafio_arm`

---

## Execução

### No Raspberry Pi 3

```bash
# Executar o binário diretamente
./desafio/desafio_arm

# Modo headless (recomendado para lab)
sudo ./desafio/desafio_arm
```

**Nota**: Use `sudo` se o programa precisar acessar GPIO com privilégios.

### Via Makefile

```bash
# Do diretório exp_8
make desafio-run

# Equivalente a:
sudo ./desafio/desafio_arm
```

---

## Funcionamento

1. **Inicialização**:
   - Configura GPIO (wiringPi)
   - Detecta endereço I2C do LCD
   - Inicializa LCD com backlight
   - Inicializa polling do keypad

2. **Loop Principal**:
   ```
   Aguarda keypress
     ↓
   Decodifica tecla
     ↓
   Atualiza estado calculadora
     ↓
   Atualiza LCD
     ↓
   Aguarda próxima entrada
   ```

3. **Operações Suportadas**:
   - Adição (A)
   - Subtração (B)
   - Multiplicação (C)
   - Divisão (D)
   - Fatorial (*) — unário
   - Encadeamento de operações

---

## Limites de Operação

| Parâmetro | Valor |
|-----------|-------|
| Tamanho de operando | 32 bits (int32_t) |
| Intervalo de valores | -2.147.483.648 a +2.147.483.647 |
| Debounce de tecla | 50 ms |
| Polling interval | 50 ms |
| Taxa de refresh LCD | ~20 Hz |

---

## Tratamento de Erros

O programa **nunca trava** (RNF01 — robustness specification):

- **Overflow em operação**: Exibe `OVERFLOW` no LCD, continua aceitando entrada
- **Divisão por zero**: Exibe `DIV/0`, continua aceitando entrada
- **Fatorial de número negativo**: Exibe `OVERFLOW`, continua aceitando entrada
- **Qualquer tecla após erro**: Reseta a calculadora automaticamente

---

## Diferenças em relação a main.c (versão SDL2)

| Aspecto | main.c (SDL2) | desafio.c (I2C Keypad+LCD) |
|--------|---------------|--------------------------|
| Entrada | Teclado via SDL | Matrix Keypad I2C (0x48) |
| Saída | Janela gráfica SDL | LCD1602 via I2C (0x27) |
| Tipo de operando | int64_t | int32_t |
| Modo de operação | Modo desktop | Headless embarcado |
| Dependências gráficas | SDL2, SDL2_ttf | wiringPi, Freenove Keypad |
| Interface | Gráfica (SDL) | Hardware (I2C puro) |
| Tamanho de resultado | 64 bits | 32 bits (adaptado ao LCD) |

---

## Estrutura do Código

- **desafio.c**: Arquivo principal com:
  - Funções de inicialização LCD (I2C auto-detect PCF8574)
  - Instância da classe Keypad (Freenove) para I2C 0x48
  - Máquina de estados da calculadora
  - Loop principal de eventos
  
- **Keypad.cpp / Key.cpp**: Biblioteca Freenove para Matrix Keypad I2C
- **Makefile** (exp_8/): Compilação centralizada com g++ (C++)
- **alu.s**: ULA compartilhada com main.c (sem modificações)

---

## Debugging

### Verificar endereços I2C conectados

```bash
# Escanear barramento I2C
sudo i2cdetect -y 1
```

Você deve ver:
- `0x27` → LCD1602 (PCF8574 expander)
- `0x48` → Matrix Keypad I2C

Exemplo de saída esperada:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:  --  --  --  --  --  --  --  --  --  --  --  --
10:  --  --  --  --  --  --  --  --  --  --  --  --
20:  --  --  --  --  --  27  --  --  --  --  --  --
30:  --  --  --  --  --  --  --  --  --  --  --  --
40:  --  --  --  --  --  --  48  --  --  --  --  --
50:  --  --  --  --  --  --  --  --  --  --  --  --
60:  --  --  --  --  --  --  --  --  --  --  --  --
70:  --  --  --  --  --  --  --  --  --  --  --  --
```

Dispositivo encontrado em `0x27` (PCF8574T com LCD)
Dispositivo encontrado em `0x48` (Matrix Keypad I2C)

---

## Troubleshooting

| Problema | Solução |
|----------|---------|
| `Keypad.hpp: No such file` | Instalar Freenove kit em `~/Freenove_Kit/` |
| `lcd.h: No such file` | Instalar: `sudo apt install wiringpi-dev` |
| `./desafio_arm: not found` | Compilar no próprio Pi (não cross-compile) |
| LCD não exibe nada | Verificar I2C com `i2cdetect -y 1` (deve ver 0x27) |
| Keypad não responde | Verificar I2C com `i2cdetect -y 1` (deve ver 0x48) |
| Erro ao compilar .cpp | Garantir que usa `g++` e não `gcc` (Makefile correto) |
| Ambos dispositivos I2C não aparecem | Verificar conexão física dos cabos I2C (SDA/SCL) |

---

## Próximos Passos (Extensões Possíveis)

1. **Salvar histórico** em EEPROM
2. **Modo hexadecimal** (H)
3. **Operações científicas** (sqrt, pow, etc.)
4. **Exibição em binário** no LCD
5. **LEDs de status** para operações
6. **Buzzer** para confirmação de operação

---

## Referências

- Freenove fnk0054 Kit: [freenove.com](https://www.freenove.com)
- WiringPi: [wiringpi.com](http://wiringpi.com)
- Raspberry Pi GPIO: [pinout.xyz](https://pinout.xyz)

---

## Autor

Desenvolvido como desafio adicional do laboratório PCS3732 (2026)
