/*
 * test_file.cpp
 *
 *  Created on: Jul 29, 2020
 *      Author: vagrant
 *This file is used by the testing suite. It is meant to be used as an
 *object file that will be executed by Juicer and checked for correctness
 *on our unit tests.
 */

#include "test_file1.h"

#include<libelf.h>
#include <libdwarf.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

Square sq = {};
Circle ci = {};

CFE_ES_HousekeepingTlm_t hk{};

int vector_x = 100;

unsigned int some_unsiged_int = 12;

int8_t precise_int8 = 110;

int16_t precise_int16 = 110;

int32_t precise_int32 = 110;

int64_t precise_int64 = 110;

uint8_t precise_unsigned_int8 = 112;

uint16_t precise_unsigned_int16 = 112;

uint32_t precise_unsigned_int32 = 112;

uint64_t precise_unsigned_int64 = 112;

char character = '2';

int flat_array[] = {1,2,3,4,5,6};

float some_float = 1.5;

short some_short = 20;

unsigned short some_signed_short = 14;

long a_long_value = 0;

long long a_very_long_value = 0;

double some_double = 4.5;

int vector_y = 30;

char alphabet[] = {'a', 'b', 'c'};

enum Color rainbow  = RED;

int another_array[] = {20,21,22,34};
