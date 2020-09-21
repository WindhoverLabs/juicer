/*
 * TestModule.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: vagrant
 */

#include <ElfObj.h>
#include "catch.hpp"


/**
 *@todo This testing module is not done yet.
 */

TEST_CASE( "Correctness of a Module object instance", "[Module] " ) {
    std::string newModuleName{"ABC"};
    uint32_t    moduleId = 102;
    ElfObj      myModule{newModuleName};

    myModule.setId(moduleId);

    REQUIRE( myModule.getName() == newModuleName );
    REQUIRE( myModule.getId() == moduleId );

}




