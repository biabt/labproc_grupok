// =============================================================
// alu.s - Unidade Logica Aritmetica (ULA) em Assembly ARM64
// Alvo: Raspberry Pi 3 - Cortex-A53 (AArch64)
//
// Convencao de chamada AAPCS64:
//   x0 = operando A (entrada) / valor de retorno (resultado)
//   x1 = operando B (entrada)
//   x2 = ponteiro para flag de erro (uint32_t* err_flag)
//        err_flag = 0 -> operacao valida
//        err_flag = 1 -> erro (ex: divisao por zero, overflow)
//
// Cada rotina abaixo corresponde a um ramo do "Decodificador de
// OpCode" mostrado no fluxograma da ULA do laboratorio.
// =============================================================

    .text
    .align 4

// -------------------------------------------------------------
// int64_t alu_add(int64_t a, int64_t b, uint32_t *err)
// [+] ADD - Instrucao direta no pipeline da CPU
// -------------------------------------------------------------
    .global alu_add
alu_add:
    str     wzr, [x2]           // err = 0 (sem erro possivel na soma)
    adds    x0, x0, x1          // x0 = a + b, atualiza flags (NZCV)
    ret

// -------------------------------------------------------------
// int64_t alu_sub(int64_t a, int64_t b, uint32_t *err)
// [-] SUB - Checagem obrigatoria da flag de sinal negativo
// -------------------------------------------------------------
    .global alu_sub
alu_sub:
    str     wzr, [x2]           // err = 0
    subs    x0, x0, x1          // x0 = a - b, atualiza flag N (sinal)
    ret                          // o chamador em C consulta o sinal
                                  // de x0 diretamente (valor com sinal)

// -------------------------------------------------------------
// int64_t alu_mul(int64_t a, int64_t b, uint32_t *err)
// [*] MUL - Deslocamento de bits (shift) e adicao via MUL nativo
// -------------------------------------------------------------
    .global alu_mul
alu_mul:
    str     wzr, [x2]           // err = 0
    mul     x0, x0, x1          // x0 = a * b
    ret

// -------------------------------------------------------------
// int64_t alu_fat(int64_t n, uint32_t *err)
// [!] FAT - Loop iterativo com monitoramento de overflow
// x0 = n, x1 = ponteiro err (note: aqui soh 1 operando)
// -------------------------------------------------------------
    .global alu_fat
alu_fat:
    str     wzr, [x1]           // err = 0
    cmp     x0, #0
    b.lt    fat_error           // fatorial de negativo -> erro
    mov     x2, #1               // x2 = resultado (acumulador), fat(0)=1
    mov     x3, x0               // x3 = contador = n
fat_loop:
    cmp     x3, #1
    b.le    fat_done
    // multiplica x2 = x2 * x3, verificando overflow de 64 bits
    umulh   x4, x2, x3          // x4 = bits altos do produto (deteccao overflow)
    mul     x2, x2, x3          // x2 = bits baixos (resultado real)
    cmp     x4, #0              // se parte alta != 0, houve overflow
    b.ne    fat_error
    sub     x3, x3, #1
    b       fat_loop
fat_done:
    mov     x0, x2
    ret
fat_error:
    mov     w5, #1
    str     w5, [x1]            // err = 1
    mov     x0, #0
    ret

// -------------------------------------------------------------
// int64_t alu_div(int64_t a, int64_t b, uint32_t *err)
// [/] DIV - Tratamento de excecao obrigatorio (RNF01)
// -------------------------------------------------------------
    .global alu_div
alu_div:
    cmp     x1, #0
    b.eq    div_by_zero          // protege contra kernel panic / crash
    str     wzr, [x2]            // err = 0
    sdiv    x0, x0, x1           // divisao inteira com sinal
    ret
div_by_zero:
    mov     w5, #1
    str     w5, [x2]             // err = 1 -> C exibe aviso e volta ao input
    mov     x0, #0
    ret
