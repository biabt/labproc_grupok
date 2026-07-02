# Calculadora Binária ARM — Raspberry Pi 3 (Cortex-A53)

Implementação da ULA do laboratório PCS3732, alvo **Raspberry Pi 3
(ARM64)**, com núcleo aritmético em Assembly e interface gráfica
local via HDMI, conforme o esquemático de requisitos (RF01, RF02,
RNF01) do documento do laboratório.

## Arquitetura da solução

```
alu.s        -> Núcleo ULA em Assembly ARM64 (AArch64)
                 [+] alu_add   [-] alu_sub   [*] alu_mul
                 [!] alu_fat   [/] alu_div (com tratamento de erro)

main.c       -> Interface gráfica (SDL2), roda no framebuffer HDMI
                 do Pi. Decodifica teclado, chama a ULA em Assembly,
                 converte o resultado para ASCII e desenha na tela.

test_alu.c   -> Harness de teste independente da UI, só valida a
                 lógica da ULA (útil para depuração isolada).

Makefile     -> Compilação nativa no Pi.
```

Isso segue à risca o fluxograma "Arquitetura Lógica: Fluxo da ULA"
do material: **Interrupção do Teclado → Decodificador de OpCode →
[+/-/*/!//]→ Conversão Binário→ASCII → Buffer de Vídeo (HDMI)**.

A parte "pesada" de aritmética roda em Assembly nativo ARM64 (usando
registradores `x0-x5`, seguindo a convenção de chamada AAPCS64), e o
C só orquestra: input, chamada da função e desenho. Isso é o que
justifica caracterizar a ULA como "implementação ARM" de fato — os
opcodes `adds`, `subs`, `mul`, `umulh`, `sdiv` são instruções reais
do Cortex-A53, não simulação.

## Por que SDL2 e não X11/Qt puro?

O Raspberry Pi 3 roda Raspberry Pi OS (Linux) com abstração pesada
de processos, como o próprio slide do laboratório aponta. Em vez de
lutar contra essa abstração, usamos SDL2 com backend **KMSDRM**, que
escreve *diretamente* no framebuffer HDMI sem precisar de um
ambiente desktop completo (X11) rodando — ideal para uma "calculadora
standalone" que liga direto na tela, parecido com o espírito do
"Desafio Standalone" do slide 8 (lá é I2C+LCD; aqui é HDMI direto,
que é o requisito RF02 real do documento).

## Instalação no Raspberry Pi 3

```bash
sudo apt update
sudo apt install -y gcc libsdl2-dev libsdl2-ttf-dev
# fonte usada por padrão (ajuste FONT_PATH em main.c se não existir):
sudo apt install -y fonts-dejavu-core
```

## Compilação (feita diretamente no Pi, que já é ARM64 nativo)

```bash
git clone <seu-repositorio>
cd calc_pi3
make            # compila alu.s + main.c e gera ./calculadora_arm
make test       # roda apenas o teste da ULA (sem interface gráfica)
```

## Execução

**Modo desktop (dentro do Raspberry Pi OS com área de trabalho):**
```bash
./calculadora_arm
```

**Modo standalone via HDMI direto (fora do X11), recomendado para
bancada de laboratório:**
```bash
sudo systemctl stop lightdm      # para o ambiente gráfico, se rodando
export SDL_VIDEODRIVER=kmsdrm
sudo ./calculadora_arm
```
(precisa de `sudo` porque acesso direto ao DRM/framebuffer requer
privilégio de kernel — equivalente ao acesso GPIO citado no slide 4).

## Como usar (RF01 — entrada 4-bit)

| Tecla        | Ação                                    |
|--------------|------------------------------------------|
| `0`–`9`      | Digita o operando (limitado a 0–15, 4 bits) |
| `+ - * /`    | Seleciona a operação e começa a digitar B |
| `!`          | Fatorial imediato do operando A          |
| `Enter`/`=`  | Executa (chama a ULA em Assembly)         |
| `C`/`Esc`    | Reset (limpa registradores)               |

Também é possível clicar nos botões desenhados na tela (o layout
segue o teclado do slide 3: dígitos, operações e botão de fatorial
separado, já que fatorial é operação unária).

## Tratamento de exceção (RNF01)

- **Divisão por zero**: a rotina `alu_div` checa `x1 == 0` *antes*
  de executar `sdiv`, evitando `SIGFPE`/crash. Seta a flag de erro e
  o C exibe `"ERRO: DIV/0"` na tela, sem derrubar o processo — a
  aplicação continua aguardando novo input, exatamente como pede o
  requisito de teste do slide 5 (RNF01).
- **Overflow no fatorial**: `alu_fat` usa `umulh` para capturar os
  bits altos de cada multiplicação; se a parte alta for diferente de
  zero, há overflow de 64 bits e a operação é abortada com erro
  controlado, em vez de devolver um resultado incorreto silenciosamente.

## Testes (conforme a Tabela de Planejamento Pré-Código do slide 6)

| Requisito | Teste                        | Resultado esperado                     | Como validar aqui |
|-----------|-------------------------------|------------------------------------------|--------------------|
| RF01      | Digitar "1111"                | Registrador = 15 (decimal)               | `push_digit()` em `main.c`, veja o campo `A=` na tela |
| RF02      | Renderizar string arbitrária  | Aparece no monitor HDMI                  | `SDL_RenderPresent` |
| RNF01     | Operação inválida (A/0)       | Exibe aviso, não trava, aceita novo input | `alu_div` + `error_state` |

Isso já foi validado por emulação (QEMU aarch64) durante o
desenvolvimento: soma, subtração, multiplicação, fatorial (incluindo
o caso de overflow) e divisão (incluindo divisão por zero) retornaram
exatamente os valores esperados, sem crash.
