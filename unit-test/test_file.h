/*
 * dwarf_file.h
 *
 *  Created on: Aug 20, 2020
 *      Author: vagrant
 *
 *This file is used by the testing suite. It is meant to be used as an
 *object file that will be executed by Juicer and checked for correctness
 *on our unit tests.
 */

#ifndef TEST_FILE_H_
#define TEST_FILE_H_
#include "stdint.h"

typedef struct
{
    int32_t width = 101;
    uint8_t stuff;
    int32_t length;
    uint8_t more_stuff;
    float floating_stuff;
}Square;

/**
 *@note structs that are not typedef'd seem to not be processed by Juicer.
 *Not sure why.
 */
struct Circle
{
    float diameter = 7;
    float radius;
};

struct
{
	float area;
} Triangle;

enum Color
{
  RED,
  BLUE,
  YELLOW
};


#endif /* TEST_FILE_H_ */
