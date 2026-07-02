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
    TEST_BINARY("15 + 15", alu_add, 15, 15);
    TEST_BINARY("127 + 128", alu_add, 127, 128);
    TEST_BINARY("255 + 255", alu_add, 255, 255);
    TEST_BINARY("1000 + 2000", alu_add, 1000, 2000);
    TEST_BINARY("32767 + 1", alu_add, 32767, 1);
    TEST_BINARY("65535 + 65535", alu_add, 65535, 65535);
    TEST_BINARY("1000000 + 2000000", alu_add, 1000000, 2000000);
    TEST_BINARY("1000000000 + 1000000000", alu_add, 1000000000LL, 1000000000LL);
    TEST_BINARY("2147483647 + 1", alu_add, 2147483647LL, 1);

    printf("\n=========== SUBTRAÇÃO ===========\n");
    TEST_BINARY("0 - 0", alu_sub, 0, 0);
    TEST_BINARY("15 - 8", alu_sub, 15, 8);
    TEST_BINARY("255 - 128", alu_sub, 255, 128);
    TEST_BINARY("1000 - 999", alu_sub, 1000, 999);
    TEST_BINARY("32768 - 1", alu_sub, 32768, 1);
    TEST_BINARY("65535 - 32768", alu_sub, 65535, 32768);
    TEST_BINARY("1000000 - 123456", alu_sub, 1000000, 123456);
    TEST_BINARY("2000000000 - 1000000000", alu_sub, 2000000000LL, 1000000000LL);
    TEST_BINARY("2147483647 - 2147483646", alu_sub, 2147483647LL, 2147483646LL);
    TEST_BINARY("100 - 200", alu_sub, 100, 200);

    printf("\n=========== MULTIPLICAÇÃO ===========\n");
    TEST_BINARY("2 * 3", alu_mul, 2, 3);
    TEST_BINARY("15 * 15", alu_mul, 15, 15);
    TEST_BINARY("100 * 200", alu_mul, 100, 200);
    TEST_BINARY("255 * 255", alu_mul, 255, 255);
    TEST_BINARY("1000 * 1000", alu_mul, 1000, 1000);
    TEST_BINARY("32767 * 2", alu_mul, 32767, 2);
    TEST_BINARY("65535 * 10", alu_mul, 65535, 10);
    TEST_BINARY("100000 * 1000", alu_mul, 100000, 1000);
    TEST_BINARY("1000000 * 1000", alu_mul, 1000000, 1000);
    TEST_BINARY("2147483647 * 2", alu_mul, 2147483647LL, 2);

    printf("\n=========== DIVISÃO ===========\n");
    TEST_BINARY("10 / 2", alu_div, 10, 2);
    TEST_BINARY("255 / 5", alu_div, 255, 5);
    TEST_BINARY("1000 / 10", alu_div, 1000, 10);
    TEST_BINARY("32768 / 2", alu_div, 32768, 2);
    TEST_BINARY("65535 / 255", alu_div, 65535, 255);
    TEST_BINARY("1000000 / 100", alu_div, 1000000, 100);
    TEST_BINARY("2000000000 / 2", alu_div, 2000000000LL, 2);
    TEST_BINARY("2147483647 / 3", alu_div, 2147483647LL, 3);
    TEST_BINARY("7 / 8", alu_div, 7, 8);
    TEST_BINARY("100 / 0", alu_div, 100, 0);

    printf("\n=========== FATORIAL ===========\n");
    TEST_UNARY("0!", alu_fat, 0);
    TEST_UNARY("1!", alu_fat, 1);
    TEST_UNARY("2!", alu_fat, 2);
    TEST_UNARY("5!", alu_fat, 5);
    TEST_UNARY("8!", alu_fat, 8);
    TEST_UNARY("10!", alu_fat, 10);
    TEST_UNARY("12!", alu_fat, 12);
    TEST_UNARY("15!", alu_fat, 15);
    TEST_UNARY("20!", alu_fat, 20);
    TEST_UNARY("21!", alu_fat, 21);

    return 0;
}