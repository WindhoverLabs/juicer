/*
 * main_test.cpp
 *
 *  Created on: Aug 20, 2020
 *      Author: vagrant
 */
#include "catch.hpp"
#include "Juicer.h"
#include "IDataContainer.h"

TEST_CASE("Test Juicer at the highest level with SQLiteDB" ,"[Juicer]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");

    juicer.setIDC(idc);

    std::string moduleName{"ABC"};
    std::string inputFile{"../obj/test_file.o"};

    juicer.parse(moduleName, inputFile);

    REQUIRE(true);
}

