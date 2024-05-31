//
// Created by Dominic Ng on 29/05/2024.
//

#ifndef EMULATOR_BITMANIPULATION_H
#define EMULATOR_BITMANIPULATION_H
#include <stdint.h>
#include "memory.h"
void printBits(int64_t n);
int64_t sign_extend(int64_t num, int n);
int64_t extractBits(int64_t bits, int start, int end);
int64_t binaryToDecimal(const char *bits);

#endif //EMULATOR_BITMANIPULATION_H


