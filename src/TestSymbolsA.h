#ifndef TESTSYMBOLSA
#define TESTSYMBOLSA

#include <stdint.h>
#include <stdbool.h>
#include "TestSymbolsB.h"

typedef struct
{
    uint8_t testAA[1556713];
    TestStructB_t *testStructB;
    uint8_t testB;
    uint8_t testC;
    uint8_t testD;
    float    fltA;
    bool     boolA;
} TestStructA_t;

#endif
