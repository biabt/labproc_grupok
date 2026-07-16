# Guia de Compilação, Execução e Deploy

## 🛠️ Compilação Local (no Raspberry Pi)

### Pré-requisitos

```bash
# Atualizar lista de pacotes
sudo apt-get update

# Instalar wiringPi
sudo apt-get install wiringpi

# Instalar ferramentas de desenvolvimento
sudo apt-get install build-essential git

# Ter o Freenove Kit instalado (caminhos esperados no home)
# ~/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/
```

### Estrutura do Projeto

```
exp_9/
├── main.c                           # Código principal do metrônomo
├── test_peripherals.c               # Código de testes de periféricos
├── pins_config.h                    # Referência de pinos (legado)
├── Makefile                         # Script de compilação
├── README.md, ARCHITECTURE.md, etc. # Documentação
└── Freenove Kit (referenciado externamente)
    ├── Sweep.c     → Servo Motor (GPIO 18)
    ├── Blink.c     → LED Red (GPIO 17)
    └── Doorbell.c  → Buzzer (GPIO 12)
```

### Compilar o Projeto

```bash
# Navegar ao diretório
cd ~/workspace/LabProc/labproc_grupok/exp_9

# Compilação simples
make

# Compilação com debug (para usar gdb)
make debug

# Compilação de testes apenas
make test_periph

# Limpeza de builds anteriores
make clean
```

### Processo de Compilação Detalhado

Quando você executa `make`, o Makefile realiza os seguintes passos:

1. **Compilação de main.c**
   ```bash
   gcc -Wall -Wextra -O2 -pedantic -c main.c -o main.o
   ```
   - Cria `main.o` com o código principal do metrônomo

2. **Processamento e compilação de Sweep.c (Servo)**
   ```bash
   sed 's/^int main(/int sweep_main_disabled(/' \
       ~/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/13_1_Sweep/Sweep.c > temp_sweep.c
   gcc -Wall -Wextra -O2 -pedantic -c temp_sweep.c -o Sweep.o
   rm -f temp_sweep.c
   ```
   - Remove a função `main()` do Sweep.c (renomeia para `sweep_main_disabled`)
   - Compila como objeto `Sweep.o` (funções: `servoInit()`, `servoWrite()`, `servoWriteMS()`)

3. **Processamento e compilação de Blink.c (LED)**
   ```bash
   sed 's/^void main(/void blink_main_disabled(/' \
       ~/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/1_Blink/Blink.c > temp_blink.c
   gcc -Wall -Wextra -O2 -pedantic -c temp_blink.c -o Blink.o
   rm -f temp_blink.c
   ```
   - Remove a função `main()` do Blink.c
   - Compila como objeto `Blink.o`

4. **Processamento e compilação de Doorbell.c (Buzzer)**
   ```bash
   sed 's/^void main(/void doorbell_main_disabled(/' \
       ~/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/6_1_Doorbell/Doorbell.c > temp_doorbell.c
   gcc -Wall -Wextra -O2 -pedantic -c temp_doorbell.c -o Doorbell.o
   rm -f temp_doorbell.c
   ```
   - Remove a função `main()` do Doorbell.c
   - Compila como objeto `Doorbell.o`

5. **Linker - Junta todos os objetos**
   ```bash
   gcc main.o Sweep.o Blink.o Doorbell.o -o metronomo -lwiringPi -lm
   ```
   - Liga `main.o` + `Sweep.o` + `Blink.o` + `Doorbell.o`
   - Linked com bibliotecas: `-lwiringPi -lm`
   - Gera executável `metronomo`

### Arquivos Gerados Após Compilação

```
exp_9/
├── main.o              # Objeto compilado (metrônomo)
├── Sweep.o             # Objeto compilado (servo)
├── Blink.o             # Objeto compilado (LED)
├── Doorbell.o          # Objeto compilado (buzzer)
├── metronomo           # ✓ Executável principal
├── test_periph.o       # Objeto de testes (opcional)
└── test_periph         # ✓ Executável de testes (opcional)
```

### Saída Esperada da Compilação

```bash
$ make clean
rm -f main.o Sweep.o Blink.o Doorbell.o test_periph.o metronomo test_periph

$ make
Processando Sweep.c (removendo main)...
Sweep.o gerado com sucesso
Processando Blink.c (removendo main)...
Blink.o gerado com sucesso
Processando Doorbell.c (removendo main)...
Doorbell.o gerado com sucesso
================================
Build successful: ./metronomo
================================
```

### Mensagens de Erro Comuns na Compilação

**Erro**: `fatal error: wiringPi.h: No such file or directory`
```
Solução: Instalar wiringPi
sudo apt-get install wiringpi
```

**Erro**: `undefined reference to 'servoInit'`
```
Solução: Sweep.o não foi compilado. Verifique:
1. Caminho do Freenove Kit em ~/Freenove_Projects_Kit_for_Raspberry_Pi
2. Execute: make clean && make
```

**Erro**: `undefined reference to '__atomic_load'`
```
Solução: Adicionar -lm no LDFLAGS (já está no Makefile)
```


## 🚀 Execução

### Executar Metrônomo

```bash
# Simples (requer sudo)
sudo ./metronomo

# Com output redirecionado
sudo ./metronomo > metronomo.log 2>&1

# Em background com nohup (continua após logout)
nohup sudo ./metronomo > metronomo.log 2>&1 &
```

### Saída Esperada ao Iniciar Metrônomo

```
========================================
  Metrônomo Eletrônico - Raspberry Pi 3
  Kit Freenove FNK0054
========================================

Inicializando hardware...
Inicializando GPIO com wiringPi...
GPIO inicializado com sucesso
Botão inicializado
Metrônomo inicializado: 120 BPM (fixo)

========================================
  Instruções de Uso
========================================
Botões:
  - Botão Start: Inicia/Para o metrônomo

Configurações:
  - BPM Fixo: 120
  - Som: Sempre ativado
  - LED: Piscando em vermelho (GPIO 17)
  - Servo: Movimento a cada batida (GPIO 18)

Pressione Ctrl+C para sair

Metrônomo iniciado: 120 BPM
BEAT! BPM: 120
BEAT! BPM: 120
BEAT! BPM: 120
...
```

### Executar Testes de Periféricos

```bash
# Menu interativo de testes
make test_periph
sudo ./test_periph

# Ou via make
make test
sudo ./test_periph
```

### Menu de Testes Esperado

```
========================================
  Teste de Periféricos - Metrônomo
  Kit Freenove FNK0054 (BCM GPIO)
========================================
1 - Testar LED Vermelho (GPIO 17)
2 - Testar Buzzer (GPIO 12)
3 - Testar Servo Motor (GPIO 18)
4 - Testar Botão START (GPIO 25)
5 - Teste Completo (Simulação)
0 - Sair
========================================
Escolha uma opção: 
```

## 🧪 Testes de Periféricos Individuais

### 1. Teste de LED Vermelho (GPIO 17)

```bash
sudo ./test_periph
# Selecione: 1
```

**Resultado esperado:**
```
=== TESTE DE LED VERMELHO (GPIO 17) ===
LED ligado por 2 segundos...
Piscando LED 5 vezes...
Teste de LED concluído
```

**Comportamento esperado no hardware:**
- LED acende por 2 segundos
- LED pisca 5 vezes com 300ms ligado/desligado

### 2. Teste de Buzzer (GPIO 12)

```bash
sudo ./test_periph
# Selecione: 2
```

**Resultado esperado:**
```
=== TESTE DE BUZZER (GPIO 12) ===
Gerando beep 1 (500ms)...
Gerando beep 2 (300ms)...
Gerando beep 3 (100ms)...
Teste de buzzer concluído
```

**Comportamento esperado no hardware:**
- 3 beeps em sequência com durações diferentes

### 3. Teste de Servo Motor (GPIO 18)

```bash
sudo ./test_periph
# Selecione: 3
```

**Resultado esperado:**
```
=== TESTE DE SERVO MOTOR (GPIO 18) ===
Usando Sweep.c do Freenove Kit
Movendo servo para posição mínima (0°)...
Movendo servo para posição central (90°)...
Movendo servo para posição máxima (180°)...
Retornando servo para centro...
Teste de servo concluído
```

**Comportamento esperado no hardware:**
- Servo move de 0° → 90° → 180° → 90°
- Cada posição mantém por ~1.5 segundos

### 4. Teste de Botão START (GPIO 25)

```bash
sudo ./test_periph
# Selecione: 4
# Pressione o botão START múltiplas vezes dentro de 30 segundos
```

**Resultado esperado:**
```
=== TESTE DE BOTÃO (GPIO 25) ===
Pressione o botão START múltiplas vezes (30 segundos de timeout)...
Botão START pressionado! (contagem: 1)
Botão START pressionado! (contagem: 2)
Botão START pressionado! (contagem: 3)
Teste de botão concluído (3 vezes pressionado)
```

**Comportamento esperado:**
- Cada pressão é registrada com contagem
- Debounce de 50ms evita múltiplas detecções

### 5. Teste Completo (Simulação de Metrônomo)

```bash
sudo ./test_periph
# Selecione: 5
```

**Resultado esperado:**
```
=== TESTE COMPLETO ===
Simulando 3 batidas de metrônomo...

Batida 1
Batida 2
Batida 3
Teste completo finalizado
```

**Comportamento esperado no hardware:**
- 3 batidas simuladas (LED + Buzzer + Servo)
- LED acende
- Buzzer toca
- Servo move para máximo e volta ao centro
- Tudo simultâneo para cada batida

## 🔌 Verificação de Hardware

### Pinos Utilizados (Numeração BCM)

| Periférico | GPIO | Header Pin | Função | Fonte |
|-----------|------|------------|--------|--------|
| LED Red | 17 | Pin 11 | Output | Blink.c |
| Botão START | 25 | Pin 22 | Input+PullUp | Manual |
| Buzzer | 12 | Pin 32 | Output | Doorbell.c |
| Servo | 18 | Pin 12 | Output (PWM) | Sweep.c |

### Verificar Mapeamento de Pinos

```bash
# Listar todos os pinos
gpio -g readall

# Verificar especificamente os pinos do projeto
gpio -g readall | grep -E "GPIO (17|25|12|18)"
```

Saída esperada:
```
 | 17  | 0   |  GPIO.0 |   OUT |   | 11 || 12 |   |      | GPIO.1  | 1   | 18  |
 | 18  | 1   |  GPIO.1 |   OUT |   | 12 || 13 |   |      | GPIO.2  | 2   | 27  |
 | 25  | 6   |  GPIO.6 |   IN  |   | 22 || ...
 | 12  | 8   |  GPIO.8 |   OUT |   | 32 || ...
```

### Testar GPIO Individual

```bash
# LED (GPIO 17) - Output
gpio -g mode 17 out
gpio -g write 17 1      # Acender
gpio -g write 17 0      # Apagar

# Botão (GPIO 25) - Input
gpio -g mode 25 in
gpio -g read 25         # Lê valor (1=solto, 0=pressionado)

# Buzzer (GPIO 12) - Output
gpio -g mode 12 out
gpio -g write 12 1      # Acionar
gpio -g write 12 0      # Desacionar

# Servo (GPIO 18) - PWM (via Freenove)
# Nota: Deve usar o programa compilado, não gpio direto
sudo ./test_periph      # Selecione opção 3
```

### Verificar Alimentação

```bash
# Verificar voltagem nos pinos 3V3 e 5V
# Pino 1 ou 17 = 3.3V
# Pino 2 ou 4 = 5V

# Medir com multímetro:
# - GND (Pin 6, 9, 14, 20, 25, 30, 34, 39)
# - 3.3V (Pin 1, 17)
# - 5V (Pin 2, 4)

# Verificar status da alimentação
vcgencmd get_throttled   # 0=OK, outro valor=problema
vcgencmd measure_volts   # Ver voltagens
```

## 🐛 Debug com GDB

### Compilar com Símbolos de Debug

```bash
make debug
# Gera: metronomo (com símbolos -g)
```

### Executar com GDB

```bash
sudo gdb ./metronomo

# Dentro do GDB:
(gdb) break main
(gdb) run
(gdb) continue
(gdb) break metronome_beat
(gdb) continue
(gdb) print metronome.bpm
(gdb) print metronome.state
(gdb) step
(gdb) quit
```

### Debugging Específico do Metrônomo

```bash
# Breakpoint na batida
(gdb) break metronome_beat

# Breakpoint no processamento de botão
(gdb) break button_process_all

# Ver estado do metrônomo
(gdb) print metronome

# Monitorar mudanças no BPM
(gdb) watch metronome.bpm

# Trace de chamadas
(gdb) set print pretty on
(gdb) print metronome
```

### Comandos GDB Úteis

```bash
# Definir breakpoint em função
break metronome_beat

# Definir breakpoint em linha
break main.c:120

# Executar até breakpoint
continue (ou c)

# Próxima linha
next (ou n)

# Entrar em função
step (ou s)

# Sair de função
finish

# Imprimir variável
print metronome.bpm
print metronome

# Listar código ao redor de breakpoint
list

# Backtrace (stack trace)
backtrace

# Mostrar valor em hexadecimal
print /x metronome.state
```

## 📊 Monitoramento em Tempo Real

### Monitor de CPU e Memória

```bash
# Top - Monitor interativo
top

# Procurar processo metrônomo
top -p $(pgrep metronomo)

# Listar processos em tempo real
ps aux | grep metronomo
```

### Monitorar GPIO em Tempo Real

```bash
# Watch GPIO a cada 2 segundos
watch -n 0.1 "gpio readall | grep GPIO"

# Ou com comando customizado
while true; do gpio read 25; sleep 0.1; done
```

### Capturar Output do Programa

```bash
# Redirecionar output
sudo ./metronomo > /tmp/metronomo.log 2>&1

# Monitorar em tempo real
tail -f /tmp/metronomo.log
```

## 🚨 Troubleshooting

### Problema: Compilação falha com "Sweep.c: No such file or directory"

**Erro**:
```
fatal error: ...Sweep.c: No such file or directory
```

**Causa**: Freenove Kit não está instalado no caminho esperado

**Solução**:
```bash
# Clonar o repositório do Freenove Kit
cd ~
git clone https://github.com/Freenove/Freenove_Projects_Kit_for_Raspberry_Pi.git

# Verificar se o arquivo existe
ls ~/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/13_1_Sweep/Sweep.c
```

### Problema: Erro "Permission Denied" ao executar

**Erro**:
```
bash: ./metronomo: Permission denied
```

**Solução**:
```bash
chmod +x metronomo
sudo ./metronomo
```

### Problema: "Falha ao inicializar wiringPi"

**Erro**:
```
Erro: Falha ao inicializar wiringPi
Certifique-se de estar executando com privilégios de root
```

**Solução**:
```bash
# Deve estar com sudo
sudo ./metronomo

# Ou adicionar usuário ao grupo gpio
sudo usermod -aG gpio $USER
# Fazer logout e login novamente
```

### Problema: LED não acende

**Checklist**:
1. Verificar conexão física do LED em GPIO 17 (Pin 11)
   ```bash
   gpio -g mode 17 out
   gpio -g write 17 1      # Deve acender
   gpio -g write 17 0      # Deve apagar
   ```
2. Verificar se LED não está invertido (cátodo deve estar em GND)
3. Testar com test_periph:
   ```bash
   sudo ./test_periph
   # Selecione 1 (Teste de LED)
   ```

### Problema: Buzzer não toca

**Checklist**:
1. Verificar conexão física do buzzer em GPIO 12 (Pin 32)
   ```bash
   gpio -g mode 12 out
   gpio -g write 12 1      # Deve soar
   gpio -g write 12 0      # Deve parar
   ```
2. Buzzer ativo deve estar alimentado (alguns requerem 5V separado)
3. Verificar polaridade: + para pino, - para GND
4. Testar com test_periph:
   ```bash
   sudo ./test_periph
   # Selecione 2 (Teste de Buzzer)
   ```

### Problema: Servo não se move

**Checklist**:
1. Verificar conexão física em GPIO 18 (Pin 12)
2. Servo requer MUITA energia (~1A), não pode vir direto de GPIO
   - Usar fonte externa de 5-6V
   - GND compartilhado com Raspberry Pi
3. Testar com test_periph:
   ```bash
   sudo ./test_periph
   # Selecione 3 (Teste de Servo)
   ```
4. Se problema persistir, compilar apenas Sweep.c:
   ```bash
   make Sweep.o
   ```

### Problema: Botão não registra pressionamentos

**Checklist**:
1. Verificar conexão física em GPIO 25 (Pin 22)
2. Botão deve estar conectado entre GPIO e GND
3. Pull-up interno está habilitado no código
4. Testar leitura direta:
   ```bash
   gpio -g mode 25 in
   gpio -g read 25         # 1=solto, 0=pressionado
   ```
5. Testar com test_periph:
   ```bash
   sudo ./test_periph
   # Selecione 4 (Teste de Botão)
   ```

### Problema: Mensagem "undefined reference to 'servoInit'"

**Erro**:
```
undefined reference to `servoInit'
```

**Causa**: Sweep.o não foi compilado corretamente

**Solução**:
```bash
# Recompilar
make clean
make

# Ou apenas recompilar Sweep.o
make Sweep.o

# Se problema persistir, verificar se sed funcionou
sed 's/^int main(/int sweep_main_disabled(/' ~/Freenove_Projects_Kit_for_Raspberry_Pi/Code/C_Code/13_1_Sweep/Sweep.c | head -20
```

### Problema: Programa trava ao sair (Ctrl+C não funciona)

**Solução**:
```bash
# Em outro terminal, forçar término
killall -9 metronomo

# Ou pelo PID
kill -9 $(pgrep metronomo)

# Limpar recursos GPIO
gpio unexportall
```

### Problema: Compilação muito lenta

**Causa**: Sed processando arquivos grandes

**Solução**: É normal, Sweep.c é relativamente grande (~50KB)

**Para otimizar**:
```bash
# Usar -O3 em vez de -O2
# Já está otimizado no Makefile
```

### Problema: Temporização imprecisa do metrônomo

**Diagnóstico**: BPM parece não estar em 120

**Solução**:
1. Verificar com cronômetro: 120 BPM = 2 batidas por segundo = 500ms entre batidas
2. Verificar com strace (análise de chamadas de sistema):
   ```bash
   sudo strace -e gettimeofday ./metronomo 2>&1 | head -50
   ```
3. Aumentar prioridade do processo:
   ```bash
   sudo nice -n -10 ./metronomo
   ```

## 📦 Cross-Compilation (do PC para Raspberry Pi)

Se quiser compilar num PC com Linux ARM (em vez de no Pi):

```bash
# Instalar toolchain ARM para Linux
sudo apt-get install gcc-arm-linux-gnueabihf

# Compilar main.c para Raspberry Pi 32-bit
arm-linux-gnueabihf-gcc -Wall -O2 main.c -o metronomo \
    -I/path/to/wiringpi/include -L/path/to/wiringpi/lib \
    -lwiringPi -lm

# Transferir para Pi
scp metronomo test_periph pi@192.168.1.100:~/exp_9/
```

**Nota**: Cross-compilation não compila Sweep.o, Blink.o, Doorbell.o. Faça isso no Raspberry Pi.

## 🔄 Deployment Automático

### Script Deploy Simples

Crie `deploy.sh` no diretório local:

```bash
#!/bin/bash
# Deploy automático para Raspberry Pi

PI_USER="pi"
PI_HOST="raspberry.local"  # ou IP: 192.168.1.100
PI_PATH="~/exp_9"

echo "Compilando projeto..."
make clean
make

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Enviando arquivos para Pi..."
scp metronomo test_periph $PI_USER@$PI_HOST:$PI_PATH/

echo "Deploy concluído!"
echo "Para iniciar: ssh $PI_USER@$PI_HOST 'cd $PI_PATH && sudo ./metronomo'"
```

Usar:
```bash
chmod +x deploy.sh
./deploy.sh
```

### Iniciar Remotamente

```bash
ssh pi@raspberry.local "cd ~/exp_9 && sudo ./metronomo"
```

## 📈 Monitoramento em Tempo Real

### Monitorar Processo

```bash
# Ver processo em execução
ps aux | grep metronomo

# Monitorar CPU/Memória
top -p $(pgrep metronomo)

# Ver log em tempo real
tail -f metronomo.log
```

### Capturar Output do Programa

```bash
# Executar e redirecionar output
sudo ./metronomo > metronomo.log 2>&1

# Monitorar output em tempo real
tail -f metronomo.log

# Contar batidas no log
grep "BEAT" metronomo.log | wc -l
```

## 📋 Resumo - Fluxo Completo de Build e Run

### 1️⃣ Preparação (primeira vez)

```bash
# Instalar dependências
sudo apt-get update && sudo apt-get install wiringpi build-essential git

# Clonar Freenove Kit (se não estiver)
cd ~
git clone https://github.com/Freenove/Freenove_Projects_Kit_for_Raspberry_Pi.git

# Ir para diretório do projeto
cd ~/workspace/LabProc/labproc_grupok/exp_9
```

### 2️⃣ Compilação

```bash
# Limpar builds anteriores
make clean

# Compilar projeto
make
```

**Saída esperada:**
```
Processando Sweep.c (removendo main)...
Sweep.o gerado com sucesso
Processando Blink.c (removendo main)...
Blink.o gerado com sucesso
Processando Doorbell.c (removendo main)...
Doorbell.o gerado com sucesso
================================
Build successful: ./metronomo
================================
```

### 3️⃣ Testes de Periféricos (Opcional)

```bash
# Compilar programa de testes
make test_periph

# Executar com sudo
sudo ./test_periph

# Menu interativo para testar cada periférico
# Opção 1: LED
# Opção 2: Buzzer
# Opção 3: Servo
# Opção 4: Botão
# Opção 5: Simulação Completa
```

### 4️⃣ Execução do Metrônomo

```bash
# Iniciar metrônomo
sudo ./metronomo

# Ou com redirecionamento de output
sudo ./metronomo > metronomo.log 2>&1
```

**Saída esperada:**
```
========================================
  Metrônomo Eletrônico - Raspberry Pi 3
  Kit Freenove FNK0054
========================================

Inicializando hardware...
Inicializando GPIO com wiringPi...
GPIO inicializado com sucesso
Botão inicializado
Metrônomo inicializado: 120 BPM (fixo)

========================================
  Instruções de Uso
========================================
Botões:
  - Botão Start: Inicia/Para o metrônomo

Configurações:
  - BPM Fixo: 120
  - Som: Sempre ativado
  - LED: Piscando em vermelho (GPIO 17)
  - Servo: Movimento a cada batida (GPIO 18)

Pressione Ctrl+C para sair

Metrônomo iniciado: 120 BPM
BEAT! BPM: 120
BEAT! BPM: 120
BEAT! BPM: 120
```

### 5️⃣ Controle

- **Pressionar botão START (GPIO 25)**: Inicia/Para metrônomo
- **Ctrl+C no terminal**: Encerra programa com limpeza segura

## ✅ Checklist de Compilação Bem-Sucedida

- ✓ `make clean` remove arquivos anteriores
- ✓ `make` gera 4 arquivos .o (main.o, Sweep.o, Blink.o, Doorbell.o)
- ✓ Mensagem final: "Build successful: ./metronomo"
- ✓ Executável `metronomo` criado
- ✓ Executável `test_periph` criado (se compilado)
- ✓ `sudo ./metronomo` inicia sem erros
- ✓ LED pisca quando botão é pressionado
- ✓ Buzzer toca a cada batida
- ✓ Servo se move a cada batida

---

**Última atualização**: 2026-07-16
