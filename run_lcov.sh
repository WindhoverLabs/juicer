#!/bin/bash

#Generates html report from `gcda ` and `gcno ` files

lcov -c --directory $1 --output-file $1/main_coverage.info
genhtml $1/main_coverage.info --output-directory $1/report
