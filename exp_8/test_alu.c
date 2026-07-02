#include <stdio.h>
#include <stdint.h>

extern int64_t alu_add(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_sub(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_mul(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_fat(int64_t n, uint32_t *err);
extern int64_t alu_div(int64_t a, int64_t b, uint32_t *err);

int main(void) {
    uint32_t err;
    printf("15 + 15 = %lld (err=%u)\n", (long long)alu_add(15,15,&err), err);
    printf("5 - 9   = %lld (err=%u)\n", (long long)alu_sub(5,9,&err), err);
    printf("6 * 7   = %lld (err=%u)\n", (long long)alu_mul(6,7,&err), err);
    printf("fat(6)  = %lld (err=%u)\n", (long long)alu_fat(6,&err), err);
    printf("fat(21) = %lld (err=%u) [espera overflow, err=1]\n", (long long)alu_fat(21,&err), err);
    printf("10 / 2  = %lld (err=%u)\n", (long long)alu_div(10,2,&err), err);
    printf("10 / 0  = %lld (err=%u) [espera err=1, sem crash]\n", (long long)alu_div(10,0,&err), err);
    return 0;
}
