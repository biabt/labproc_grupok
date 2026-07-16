# Guia de Compilação e Execução - Metrônomo Eletrônico

## 📋 Quick Start (3 Passos)

### 1️⃣ Instalar Dependências (primeira vez)

```bash
sudo apt-get update && sudo apt-get install wiringpi build-essential git

# Clonar Freenove Kit
cd ~
git clone https://github.com/Freenove/Freenove_Projects_Kit_for_Raspberry_Pi.git
```

### 2️⃣ Compilar

```bash
cd ~/workspace/LabProc/labproc_grupok/exp_9
make clean
make
```

**Saída esperada:**
```
Processando Sweep.c (removendo main)...
Processando Blink.c (removendo main)...
Processando Doorbell.c (removendo main)...
Build successful: ./metronomo
```

### 3️⃣ Executar

```bash
# Metrônomo (120 BPM fixo, controle por botão GPIO 25)
sudo ./metronomo

# Testes de periféricos (interativo)
sudo ./test_periph
```

---

## 📊 Pinos Utilizados (BCM GPIO)

| Periférico | GPIO | Header Pin | Função |
|-----------|------|------------|--------|
| LED Red | 17 | 11 | Output |
| Botão | 25 | 22 | Input |
| Buzzer | 12 | 32 | Output |
| Servo | 18 | 12 | PWM |

---

## 🧪 Testes de Periféricos

```bash
sudo ./test_periph
# Menu: 1=LED, 2=Buzzer, 3=Servo, 4=Botão, 5=Simulação, 0=Sair

# Testes diretos com gpio:
gpio -g write 17 1      # LED acender
gpio -g write 12 1      # Buzzer soar
gpio -g read 25         # Ler botão (1=solto, 0=pressionado)
```

---

## 🐛 Troubleshooting

### Freenove Kit não instalado
```bash
cd ~
git clone https://github.com/Freenove/Freenove_Projects_Kit_for_Raspberry_Pi.git
```

### undefined reference to 'servoInit'
```bash
make clean && make
```

### LED/Buzzer/Servo não funcionam
```bash
# Testar GPIO diretamente
gpio -g mode 17 out && gpio -g write 17 1

# Se problema persiste, usar test_periph (opção correspondente)
sudo ./test_periph
```

### Botão não registra
```bash
gpio -g mode 25 in && gpio -g read 25  # Deve ser 1 (solto) ou 0 (pressionado)
```

### Permission denied ao executar
```bash
chmod +x metronomo test_periph
sudo ./metronomo
```

### Servo não se move
```bash
# Servo requer fonte externa 5-6V (~1A)
# GND compartilhado com Pi
# Testar com: sudo ./test_periph (opção 3)
```

---

## 🛠️ Comandos Úteis

```bash
make clean        # Limpar builds anteriores
make debug        # Compilar com símbolos de debug
make test_periph  # Compilar programa de testes
make run          # Executar metrônomo
make test         # Executar testes
make all          # Compilar tudo

# Monitoramento
top -p $(pgrep metronomo)
ps aux | grep metronomo
gpio -g readall   # Ver status de todos os GPIO
```

---

## 📈 Execução Esperada

**Metrônomo:**
```
========================================
  Metrônomo Eletrônico - Raspberry Pi 3
  Kit Freenove FNK0054
========================================

Inicializando hardware...
Metrônomo inicializado: 120 BPM (fixo)

Metrônomo iniciado: 120 BPM
BEAT! BPM: 120
BEAT! BPM: 120
BEAT! BPM: 120
...
```

**Testes:**
```
========================================
  Teste de Periféricos
  Kit Freenove FNK0054 (BCM GPIO)
========================================
1 - Testar LED Vermelho (GPIO 17)
2 - Testar Buzzer (GPIO 12)
3 - Testar Servo Motor (GPIO 18)
4 - Testar Botão START (GPIO 25)
5 - Teste Completo (Simulação)
0 - Sair
Escolha uma opção:
```

---

## 🚀 Deployment (Opcional)

**Deploy remoto:**
```bash
#!/bin/bash
PI_HOST="192.168.1.100"
make clean && make
scp metronomo pi@$PI_HOST:~/exp_9/
ssh pi@$PI_HOST "cd ~/exp_9 && sudo ./metronomo"
```

---
