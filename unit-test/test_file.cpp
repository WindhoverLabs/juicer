/*
 * test_file.cpp
 *
 *  Created on: Jul 29, 2020
 *      Author: vagrant
 *This file is used by the testing suite. It is meant to be used as an
 *object file that will be executed by Juicer and checked for correctness
 *on our unit tests.
 */

#include "test_file.h"

#include<libelf.h>
#include <libdwarf.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

Square sq = {};
Circle ci = {};
enum Color rainbow  = RED;
