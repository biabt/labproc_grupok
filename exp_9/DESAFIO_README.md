# Desafio.c - Metrônomo com Controle de Velocidade Dinâmico

## Descrição

O arquivo **desafio.c** é uma versão estendida do **main.c** que implementa controle dinâmico de velocidade (BPM) através de um botão adicional (GPIO 21).

## Funcionalidades Principais

### 1. **Metrônomo Base (idêntico a main.c)**
- Comportamento 100% compatível com main.c
- Servo com movimento suave e contínuo (0°→180°→0°)
- LED + Buzzer ativados ao passar por ~90°
- Botão GPIO 26 para iniciar/parar

### 2. **Novo: Controle de Velocidade (GPIO 21)**
- **Função**: Diminui BEAT_INTERVAL_MS em 500ms a cada pressão
- **Ciclo de velocidades** (em BPM equivalente para 2 batidas):
  - 3000ms → 40 BPM (padrão inicial)
  - 2500ms → 48 BPM
  - 2000ms → 60 BPM
  - 1500ms → 80 BPM
  - 1000ms → 120 BPM
  - 500ms → 240 BPM
  - **Reset**: Ao atingir 500ms e pressionar GPIO 21, retorna para 3000ms

## Especificações Técnicas

### Pinos GPIO Utilizados
| Pino | Função | Modo | Descrição |
|------|--------|------|-----------|
| GPIO 17 | LED Red | OUTPUT | Pisca ao atingir 90° |
| GPIO 18 | Servo Motor | PWM | Movimento suave 0°-180° |
| GPIO 12 | Buzzer | OUTPUT | Toca ao atingir 90° (100ms) |
| GPIO 26 | Botão Start/Stop | INPUT (Pull-up) | Inicia/Para metrônomo |
| GPIO 21 | Botão Speed | INPUT (Pull-up) | Diminui 500ms (novo) |

### Características de Operação

#### Comportamento Dinâmico de Velocidade
```
Pressão no GPIO 21 → BEAT_INTERVAL_MS -= 500ms
├─ Se BEAT_INTERVAL_MS < 500ms: retorna a 3000ms
├─ Servo continua se movimento durante a mudança
└─ Mensagem de debug mostrada no console
```

#### Movimento do Servo
- **Beat 0**: Servo atual → 180° (tempo = BEAT_INTERVAL_MS)
- **Beat 1**: Servo atual → 0° (tempo = BEAT_INTERVAL_MS)
- **Continuidade**: Servo NÃO reseta entre batidas
- **Suavidade**: Interpolação linear baseada em tempo

#### Detecção de 90°
- Método: Distância (tolerância ≤2°)
- Funciona em ambas as direções (subindo e descendo)
- Ativa LED + Buzzer por 100ms

### Debounce
- **Período**: 200ms entre pressões de botão
- **Método**: Estado anterior + diferença de tempo

## Compilação

### Build Básico
```bash
# Compilar apenas o desafio
make desafio.o

# Compilar tudo (metrônomo + desafio)
make clean
make
```

### Build e Execução
```bash
# Iniciar daemon pigpio (se necessário)
sudo pigpiod

# Compilar e executar desafio.c
make run-desafio

# Compilar e executar main.c
make run

# Compilar testes
make test
```

### Makefile Targets
```bash
make              # Compila main.c + desafio.c
make run          # Compila e executa main.c (metrônomo original)
make run-desafio  # Compila e executa desafio.c (COM controle de velocidade)
make test         # Compila e executa testes de periféricos
make debug        # Compila com símbolos de debug (-g)
make clean        # Remove arquivos .o e binários
make help         # Mostra todos os targets disponíveis
```

## Diferenças com main.c

| Aspecto | main.c | desafio.c |
|---------|--------|-----------|
| BEAT_INTERVAL_MS | `#define` (3000ms fixo) | `static variable` (dinâmico) |
| Botão GPIO 21 | Não existe | Controla velocidade |
| Ciclo de velocidades | N/A | 3000→2500→...→500→3000ms |
| Comportamento servo | Idêntico | Idêntico |
| Comportamento LED/Buzzer | Idêntico | Idêntico |
| Mensagens de debug | Menos detalhes | Inclui BPM atual |

## Instruções de Uso

### Hardware
1. Conectar Raspberry Pi 3 aos componentes do kit Freenove FNK0054
2. Verificar voltagem e conexões dos botões (Pull-up ativo)
3. Garantir servo conectado em GPIO 18

### Software
1. **Iniciar daemon**:
   ```bash
   sudo pigpiod
   ```

2. **Compilar**:
   ```bash
   cd exp_9
   make clean
   make
   ```

3. **Executar**:
   ```bash
   sudo ./desafio
   ```

### Operação
1. **Pressionar GPIO 26**: Inicia o metrônomo
2. **Pressionar GPIO 26**: Para o metrônomo
3. **Pressionar GPIO 21** (enquanto em execução ou parado):
   - Diminui velocidade em 500ms
   - Exibe BPM atual no console
   - Ao atingir 500ms e pressionar novamente: volta a 3000ms

### Console Output Exemplo
```
========================================
  Metrônomo Eletrônico - DESAFIO
  Controle de Velocidade (GPIO 21)
  Raspberry Pi 3 + Kit Freenove
========================================

Inicializando hardware...
GPIO inicializado com sucesso
Botões inicializados
Metrônomo inicializado: Velocidade inicial 3000ms

========================================
  Instruções de Uso - DESAFIO
========================================
Botões:
  - GPIO 26 (START): Inicia/Para o metrônomo
  - GPIO 21 (SPEED): Diminui 500ms a cada pressão
                     Cicla: 3000→2500→2000→1500→1000→500→3000ms

Velocidades (BPM):
  - 3000ms = 40 BPM   (2 batidas por 3 segundos)
  - 2500ms = 48 BPM   (2 batidas por 2.5 segundos)
  - 2000ms = 60 BPM   (2 batidas por 2 segundos)
  - 1500ms = 80 BPM   (2 batidas por 1.5 segundos)
  - 1000ms = 120 BPM  (2 batidas por 1 segundo)
  - 500ms  = 240 BPM  (2 batidas por 0.5 segundo)

Comportamento:
  - Servo: Movimento contínuo alternando entre extremos
  - LED e Buzzer: Ativam quando servo passa por ~90°
  - Buzzer: Soa por 100ms a partir dos 90°

Pressione Ctrl+C para sair
Pressione GPIO 26 para iniciar/parar
Pressione GPIO 21 para ajustar velocidade

Botão no pino 26 pressionado
Metrônomo iniciado
Velocidade atual: 3000ms (40 BPM)
Beat 1: Servo 0→180 (distância: 180°, intervalo: 3000ms)
  → Ângulo ~90° atingido! (atual=90°) LED ON, Buzzer ON
Beat 2: Servo 180→0 (distância: 180°, intervalo: 3000ms)
  → Ângulo ~90° atingido! (atual=89°) LED ON, Buzzer ON
Botão no pino 21 pressionado
Velocidade alterada para 2500ms (48 BPM)
```

## Possíveis Problemas e Soluções

### Botão GPIO 21 não funciona
- **Verificação**: Confirmar fiação do botão
- **Solução**: Testar com `sudo ./test_periph` (testa GPIO 21)

### Velocidade não muda
- **Causa**: Debounce ativo (200ms mínimo entre pressões)
- **Solução**: Aguardar 200ms entre pressões

### Servo pula ou não move suavemente
- **Causa**: BEAT_INTERVAL_MS muito pequeno (<500ms) com servo lento
- **Solução**: Aumentar BEAT_INTERVAL_MIN se necessário

### LED/Buzzer não ativam ao mudar velocidade
- **Comportamento**: Normal, ativam apenas ao passar por ~90°
- **Verificação**: Confirmar que servo atinge 90° (geralmente passa rápido)

## Comparação: main.c vs desafio.c

```c
// main.c (velocidade fixa)
#define BEAT_INTERVAL_MS 1500

// desafio.c (velocidade dinâmica)
static unsigned long beat_interval_ms = 1500;

void decrease_speed(void) {
    beat_interval_ms -= BEAT_INTERVAL_STEP;
    if (beat_interval_ms < BEAT_INTERVAL_MIN) {
        beat_interval_ms = BEAT_INTERVAL_MAX;
    }
}
```

## Estrutura de Funções (desafio.c)

### Funções Principais
- `get_time_ms()` - Tempo em ms
- `gpio_init()` - Inicializa GPIO (inclui GPIO 21)
- `button_init()` - Inicializa estruturas de botão
- `button_process()` - Processa individual com debounce
- `button_process_all()` - Processa todos os botões
- `execute_beat()` - Executa uma batida (servo suave + trigger)
- `metronome_loop()` - Loop principal
- `decrease_speed()` - Diminui velocidade em 500ms
- `display_current_speed()` - Exibe BPM atual

### Configurações Ajustáveis
```c
#define BEAT_INTERVAL_MIN   500     /* Mínimo: 500ms */
#define BEAT_INTERVAL_MAX   3000    /* Máximo: 3000ms */
#define BEAT_INTERVAL_STEP  500     /* Passo: 500ms */
#define BUZZER_DURATION_MS  100     /* Buzzer: 100ms */
#define DEBOUNCE_MS         200     /* Debounce: 200ms */
```

## Referências

- [main.c](main.c) - Versão com velocidade fixa
- [test_peripherals.c](test_peripherals.c) - Testes de periféricos
- [Makefile](Makefile) - Sistema de build
- [COMPILATION_QUICK.md](COMPILATION_QUICK.md) - Guia rápido
- [COMPILATION.md](COMPILATION.md) - Documentação detalhada
