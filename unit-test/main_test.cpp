/*
 * main_test.cpp
 *
 *  Created on: Aug 20, 2020
 *      Author: vagrant
 */
#include "catch.hpp"
#include "Juicer.h"
#include "IDataContainer.h"
#include "SQLiteDB.h"

TEST_CASE("Test Juicer at the highest level with SQLiteDB" ,"[Juicer]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{"ut_obj/test_file.o"};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    REQUIRE(juicer.parse(moduleName, inputFile) == JUICER_OK);

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;

}

//TEST_CASE("Test the correctness of the data structures in the data base" ,"[Juicer]")
//{
//    Juicer          juicer;
//    IDataContainer* idc = 0;
//    Logger          logger;
//
//
//    logger.logWarning("This is just a test.");
//
//    std::string moduleName{"ABC"};
//    std::string inputFile{"ut_obj/test_file.o"};
//
//    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
//    REQUIRE(idc!=nullptr);
//    logger.logInfo("IDataContainer was constructed successfully for unit test.");
//
//    juicer.setIDC(idc);
//
//    REQUIRE(juicer.parse(moduleName, inputFile) == JUICER_OK);
//
//    /**
//     *Clean up our database handle and objects in memory.
//     */
//    ((SQLiteDB*)(idc))->close();
//    REQUIRE(remove("./test_db.sqlite")==0);
//    delete idc;
//
//}

TEST_CASE("Write keys to database that already exist" ,"[Juicer]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{"ut_obj/test_file.o"};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    /**
     * Parse twice to write to the database twice and cause
     * existing-keys errors in the database.
     */
    REQUIRE(juicer.parse(moduleName, inputFile) == JUICER_OK);
    REQUIRE(juicer.parse(moduleName, inputFile) != JUICER_OK);
    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;

}

TEST_CASE("Write Elf File to database with a log file")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{"ut_obj/test_file.o"};

	idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
	REQUIRE(idc!=nullptr);
	logger.logInfo("IDataContainer was constructed successfully for unit test.");

	juicer.setIDC(idc);

	REQUIRE(juicer.parse(moduleName, inputFile)==JUICER_OK);

	logger.setLogFile("logFile");

	/**
	*Clean up our database handle and objects in memory.
	*/
	((SQLiteDB*)(idc))->close();
	delete idc;
	REQUIRE(remove("./logFile")==0);
	REQUIRE(remove("./test_db.sqlite")==0);
}

TEST_CASE("Write Elf File to database with verbosity set to INFO")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{LOGGER_VERBOSITY_INFO};

    std::string moduleName{"ABC"};
    std::string inputFile{"ut_obj/test_file.o"};

	idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
	REQUIRE(idc!=nullptr);
	logger.logInfo("IDataContainer was constructed successfully for unit test.");

	juicer.setIDC(idc);
	juicer.parse(moduleName, inputFile);

//	juicer.parse(moduleName, inputFile);

	/**
	*Clean up our database handle and objects in memory.
	*/
	((SQLiteDB*)(idc))->close();
	delete idc;
	REQUIRE(remove("./test_db.sqlite")==0);
}

TEST_CASE("Write Elf File to database with invalid verbosity")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{-1};

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{"ut_obj/test_file.o"};

	idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
	REQUIRE(idc!=nullptr);
	logger.logInfo("IDataContainer was constructed successfully for unit test.");

	juicer.setIDC(idc);
	juicer.parse(moduleName, inputFile);

	/**
	*Clean up our database handle and objects in memory.
	*/
	((SQLiteDB*)(idc))->close();
	delete idc;
	REQUIRE(remove("./test_db.sqlite")==0);
}

