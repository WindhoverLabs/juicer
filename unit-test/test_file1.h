/*
 * test_file1.h
 *
 *  Created on: Aug 20, 2020
 *      Author: lgomez
 *
 *This file is used by the testing suite. It is meant to be used as an
 *object file that will be executed by Juicer and checked for correctness
 *on our unit tests.
 */

#ifndef TEST_FILE_H_
#define TEST_FILE_H_
#include "stdint.h"

/**
 *The fields padding1 and padding2(as the name implies) are to prevent
 *gcc from inserting padding at compile-time and altering the expected results in our tests.
 */
typedef struct
{
    int32_t width = 101;
    uint16_t stuff;
    uint16_t padding1;
    int32_t length;
    uint16_t more_stuff;
    uint16_t padding2;
    float 	floating_stuff;
    float 	matrix3D[2][4][4];
    float 	matrix1D[2];
}Square;

struct Circle
{
    float 	diameter = 7;
    float 	radius;
    int 	points[128];
};

enum Color
{
  RED,
  BLUE,
  YELLOW
};

struct S {
uint8_t before;
int 	j:5;
int 	k:6;
int 	m:5;
uint8_t p;
int 	n:8;
uint8_t after;
};

#endif /* TEST_FILE_H_ */
