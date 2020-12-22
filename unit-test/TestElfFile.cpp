/*
 * TestModule.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: vagrant
 */

#include "../src/ElfFile.h"
#include "catch.hpp"
#include <limits.h>


/**
 *@todo This testing module is not done yet.
 */

TEST_CASE( "Correctness of a Module object instance", "[Module] " ) {
    std::string newElfName{"ABC"};
    uint32_t    elfId = 102;
    ElfFile     myElf{newElfName};
    char        resolvedPath[PATH_MAX];

    realpath(newElfName.c_str(), resolvedPath);
    newElfName.clear();
    newElfName.insert(0, resolvedPath);

    myElf.setId(elfId);
    REQUIRE( myElf.getName() == newElfName );
    REQUIRE( myElf.getId() == elfId );

}




