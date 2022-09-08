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

union Oject
{
   int32_t id;
   char data[16];
};



/**
 *The fields padding1 and padding2(as the name implies) are to prevent
 *gcc from inserting padding at compile-time and altering the expected results in our tests.
 * Tested on Ubuntu 20.04 and Ubuntu 18.04.
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


enum ModeSlot_t
{
    MODE_SLOT_NONE                           = -1,
    MODE_SLOT_1                              =  0,
    MODE_SLOT_2                              =  1,
    MODE_SLOT_3                              =  2,
    MODE_SLOT_4                              =  3,
    MODE_SLOT_5                              =  4,
    MODE_SLOT_6                              =  5,
    MODE_SLOT_MAX                            =  6
} ;

struct Circle
{
    float 	diameter = 7;
    float 	radius;
    int 	points[128];
    ModeSlot_t mode;
    Oject o;
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
