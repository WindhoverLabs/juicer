/*
 * TestIDataContainer.cpp
 *
 *  Created on: Aug 21, 2020
 *      Author: vagrant
 */
#include <catch.hpp>
#include "IDataContainer.h"

TEST_CASE("Create IDC_TYPE_CCDD IDataContainer", "[IDataContainer]")
{
    IDataContainer* idc = 0;

    std::string moduleName{"ABC"};
    std::string inputFile{"ut_obj/test_file.o"};

	idc = IDataContainer::Create(IDC_TYPE_CCDD, "./test_db.sqlite");
}

TEST_CASE("Create invalid IDataContainer", "[IDataContainer]")
{
	IDataContainer_Type_t invalidType = (IDataContainer_Type_t)-1;

    IDataContainer* idc = 0;

    std::string moduleName{"ABC"};
    std::string inputFile{"ut_obj/test_file.o"};

	idc = IDataContainer::Create(invalidType, "./test_db.sqlite");
}
