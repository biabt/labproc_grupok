#include <stdio.h>
#include <stdint.h>
#include <time.h>

extern int64_t alu_add(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_sub(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_mul(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_fat(int64_t n, uint32_t *err);
extern int64_t alu_div(int64_t a, int64_t b, uint32_t *err);

long long elapsed_ns(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start.tv_sec) * 1000000000LL +
           (end.tv_nsec - start.tv_nsec);
}

#define TEST_BINARY(nome, func, a, b)                                  \
{                                                                       \
    struct timespec start, end;                                         \
    uint32_t err = 0;                                                   \
    clock_gettime(CLOCK_MONOTONIC, &start);                             \
    int64_t r = func(a, b, &err);                                       \
    clock_gettime(CLOCK_MONOTONIC, &end);                               \
    printf("%-20s = %6lld | err=%u | %8lld ns\n",                       \
           nome, (long long)r, err, elapsed_ns(start,end));             \
}

#define TEST_UNARY(nome, func, a)                                       \
{                                                                       \
    struct timespec start, end;                                         \
    uint32_t err = 0;                                                   \
    clock_gettime(CLOCK_MONOTONIC, &start);                             \
    int64_t r = func(a, &err);                                          \
    clock_gettime(CLOCK_MONOTONIC, &end);                               \
    printf("%-20s = %6lld | err=%u | %8lld ns\n",                       \
           nome, (long long)r, err, elapsed_ns(start,end));             \
}

int main(void)
{
    printf("=========== SOMA ===========\n");
    TEST_BINARY("0 + 0", alu_add, 0, 0);
    TEST_BINARY("1 + 1", alu_add, 1, 1);
    TEST_BINARY("2 + 3", alu_add, 2, 3);
    TEST_BINARY("4 + 5", alu_add, 4, 5);
    TEST_BINARY("7 + 8", alu_add, 7, 8);
    TEST_BINARY("10 + 2", alu_add, 10, 2);
    TEST_BINARY("12 + 3", alu_add, 12, 3);
    TEST_BINARY("15 + 0", alu_add, 15, 0);
    TEST_BINARY("15 + 15", alu_add, 15, 15);
    TEST_BINARY("8 + 6", alu_add, 8, 6);

    printf("\n=========== SUBTRAÇÃO ===========\n");
    TEST_BINARY("0 - 0", alu_sub, 0, 0);
    TEST_BINARY("5 - 2", alu_sub, 5, 2);
    TEST_BINARY("10 - 5", alu_sub, 10, 5);
    TEST_BINARY("15 - 1", alu_sub, 15, 1);
    TEST_BINARY("15 - 15", alu_sub, 15, 15);
    TEST_BINARY("8 - 4", alu_sub, 8, 4);
    TEST_BINARY("3 - 7", alu_sub, 3, 7);
    TEST_BINARY("9 - 2", alu_sub, 9, 2);
    TEST_BINARY("14 - 6", alu_sub, 14, 6);
    TEST_BINARY("1 - 15", alu_sub, 1, 15);

    printf("\n=========== MULTIPLICAÇÃO ===========\n");
    TEST_BINARY("0 * 5", alu_mul, 0, 5);
    TEST_BINARY("1 * 7", alu_mul, 1, 7);
    TEST_BINARY("2 * 3", alu_mul, 2, 3);
    TEST_BINARY("4 * 4", alu_mul, 4, 4);
    TEST_BINARY("5 * 5", alu_mul, 5, 5);
    TEST_BINARY("6 * 7", alu_mul, 6, 7);
    TEST_BINARY("8 * 2", alu_mul, 8, 2);
    TEST_BINARY("10 * 3", alu_mul, 10, 3);
    TEST_BINARY("12 * 2", alu_mul, 12, 2);
    TEST_BINARY("15 * 15", alu_mul, 15, 15);

    printf("\n=========== DIVISÃO ===========\n");
    TEST_BINARY("10 / 2", alu_div, 10, 2);
    TEST_BINARY("15 / 3", alu_div, 15, 3);
    TEST_BINARY("14 / 7", alu_div, 14, 7);
    TEST_BINARY("9 / 2", alu_div, 9, 2);
    TEST_BINARY("8 / 4", alu_div, 8, 4);
    TEST_BINARY("5 / 5", alu_div, 5, 5);
    TEST_BINARY("0 / 3", alu_div, 0, 3);
    TEST_BINARY("15 / 1", alu_div, 15, 1);
    TEST_BINARY("7 / 8", alu_div, 7, 8);
    TEST_BINARY("10 / 0", alu_div, 10, 0);

    printf("\n=========== FATORIAL ===========\n");
    TEST_UNARY("0!", alu_fat, 0);
    TEST_UNARY("1!", alu_fat, 1);
    TEST_UNARY("2!", alu_fat, 2);
    TEST_UNARY("3!", alu_fat, 3);
    TEST_UNARY("4!", alu_fat, 4);
    TEST_UNARY("5!", alu_fat, 5);
    TEST_UNARY("6!", alu_fat, 6);
    TEST_UNARY("8!", alu_fat, 8);
    TEST_UNARY("10!", alu_fat, 10);
    TEST_UNARY("21!", alu_fat, 21);

    return 0;
}