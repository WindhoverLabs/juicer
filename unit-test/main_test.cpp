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
#include "test_file.h"
#include <map>
#include <string>
#include <stddef.h>


#define TEST_FILE_DIR "ut_obj"

/**
 *Checks if the platform is little endian.
 *This is used as a source of truth for our unit tests.
 */
bool is_little_endian()
{
	int n = 1;
	return (*(char *)&n == 1);
}


/**
 *@brief This call back function assumes that the first column is that row's id,
 *for now.
 */
static int callback(void *veryUsed, int argc, char **argv, char **azColName)
{
  int   i;

  auto* row = (std::map<std::string, std::vector<std::string> >*)veryUsed ;

  std::vector<std::string> tableData{};

  for(i=1; i<argc; i++)
  {
	  std::string tempData{argv[i]};

	  tableData.push_back(tempData);
  }

  std::string id{argv[0]};

  (*row)[id] = tableData;

  return 0;
}

TEST_CASE("Test Juicer at the highest level with SQLiteDB" ,"[Juicer]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{TEST_FILE_DIR};

    inputFile += "/test_file.o";

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


TEST_CASE("Test the correctness of the Circle struct after Juicer has processed it." ,"[Juicer]")
{
	/**
	 * This assumes that the test_file was compiled on
	 * gcc (Ubuntu 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609
	 *  little-endian machine.
	 */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int 			rc;
    char* 			errorMessage = nullptr;
    std::string 	little_endian = is_little_endian()? "1": "0";

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{TEST_FILE_DIR};

    inputFile += "/test_file.o";

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(moduleName, inputFile);

    REQUIRE(rc == JUICER_OK);

    std::string getCircleStructQuery{"SELECT * FROM symbols WHERE name = \"Circle\"; "};

    ((SQLiteDB*)(idc))->close();

    sqlite3 *database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::map<std::string, std::vector<std::string>> circleMap{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), callback, &circleMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     * Check the correctness of Circle struct.
     */

    REQUIRE(circleMap["19"].at(0) == "1");
    REQUIRE(circleMap["19"].at(1) == "Circle");
    REQUIRE(circleMap["19"].at(2) == std::to_string(sizeof(Circle)));

    /**
     *Check the fields of the Circle struct.
     */

    std::string getCircleFields{"SELECT * FROM fields WHERE symbol = 19;"};

    std::map<std::string, std::vector<std::string>> fieldsMap{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), callback, &fieldsMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     *Check the correctness of the fields
     */

    REQUIRE(fieldsMap["9"].at(0) == "19");
    REQUIRE(fieldsMap["9"].at(1) == "diameter");
    REQUIRE(fieldsMap["9"].at(2) == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(fieldsMap["9"].at(3) == "17");
    REQUIRE(fieldsMap["9"].at(4) == "0");
    REQUIRE(fieldsMap["9"].at(5) == little_endian);

    REQUIRE(fieldsMap["10"].at(0) == "19");
    REQUIRE(fieldsMap["10"].at(1) == "radius");
    REQUIRE(fieldsMap["10"].at(2) == std::to_string(offsetof(Circle, radius)));
    REQUIRE(fieldsMap["10"].at(3) == "17");
    REQUIRE(fieldsMap["10"].at(4) == "0");
    REQUIRE(fieldsMap["10"].at(5) == little_endian);

    /**
     *Check the correctness of the types
     */
    std::string getFieldsSymbols{"SELECT * FROM symbols WHERE id = 17;"};

    std::map<std::string, std::vector<std::string>> typesMap{};

    rc = sqlite3_exec(database, getFieldsSymbols.c_str(), callback, &typesMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(typesMap["17"].at(0) == "1");
    REQUIRE(typesMap["17"].at(1) == "float");
    REQUIRE(typesMap["17"].at(2) == std::to_string(sizeof(float)));

    /**
     * *Clean up our database handle and objects in memory.
     */

    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;
}

TEST_CASE("Test the correctness of the Square struct after Juicer has processed it." ,"[Juicer]")
{
	/**
	 * This assumes that the test_file was compiled on
	 * gcc (Ubuntu 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609
	 *  little-endian machine.
	 */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int 			rc;
    char* 			errorMessage = nullptr;
    std::string 	little_endian = is_little_endian()? "1": "0";

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{TEST_FILE_DIR};

    inputFile += "/test_file.o";

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(moduleName, inputFile);

    REQUIRE(rc == JUICER_OK);

    std::string getCircleStructQuery{"SELECT * FROM symbols WHERE name = \"Square\"; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3 *database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::map<std::string, std::vector<std::string>> circleMap{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), callback, &circleMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     * Check the correctness of Square struct.
     */
    REQUIRE(circleMap["18"].at(0) == "1");
    REQUIRE(circleMap["18"].at(1) == "Square");
    REQUIRE(circleMap["18"].at(2) == std::to_string(sizeof(Square)));

    /**
     *Check the fields of the Square struct.
     */
    std::string getCircleFields{"SELECT * FROM fields WHERE symbol = 18;"};

    std::map<std::string, std::vector<std::string>> fieldsMap{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), callback, &fieldsMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     *Check the correctness of the fields
     */
    REQUIRE(fieldsMap.at("4").at(0) == "18");
    REQUIRE(fieldsMap.at("4").at(1) == "width");
    REQUIRE(fieldsMap.at("4").at(2) == std::to_string(offsetof(Square, width)));
    REQUIRE(fieldsMap.at("4").at(3) == "6");
    REQUIRE(fieldsMap.at("4").at(4) == "0");
    REQUIRE(fieldsMap.at("4").at(5) == little_endian);

    REQUIRE(fieldsMap["5"].at(0) == "18");
    REQUIRE(fieldsMap["5"].at(1) == "stuff");
    REQUIRE(fieldsMap["5"].at(2) == std::to_string(offsetof(Square, stuff)));
    REQUIRE(fieldsMap["5"].at(3) == "10");
    REQUIRE(fieldsMap["5"].at(4) == "0");
    REQUIRE(fieldsMap["5"].at(5) == little_endian);


    REQUIRE(fieldsMap["6"].at(0) == "18");
    REQUIRE(fieldsMap["6"].at(1) == "length");
    REQUIRE(fieldsMap["6"].at(2) == std::to_string(offsetof(Square, length)));
    REQUIRE(fieldsMap["6"].at(3) == "6");
    REQUIRE(fieldsMap["6"].at(4) == "0");
    REQUIRE(fieldsMap["6"].at(5) == little_endian);

    REQUIRE(fieldsMap["7"].at(0) == "18");
    REQUIRE(fieldsMap["7"].at(1) == "more_stuff");
    REQUIRE(fieldsMap["7"].at(2) == std::to_string(offsetof(Square, more_stuff)));
    REQUIRE(fieldsMap["7"].at(3) == "10");
    REQUIRE(fieldsMap["7"].at(4) == "0");
    REQUIRE(fieldsMap["7"].at(5) == little_endian);


    REQUIRE(fieldsMap["8"].at(0) == "18");
    REQUIRE(fieldsMap["8"].at(1) == "floating_stuff");
    REQUIRE(fieldsMap["8"].at(2) == std::to_string(offsetof(Square, floating_stuff)));
    REQUIRE(fieldsMap["8"].at(3) == "17");
    REQUIRE(fieldsMap["8"].at(4) == "0");
    REQUIRE(fieldsMap["8"].at(5) == little_endian);

    /**
     *Check the correctness of the types
     */
    std::string getFieldsSymbols{"SELECT * FROM symbols WHERE id = 6;"
    						     "SELECT * FROM symbols WHERE id = 10;"
		 	 	 	 	 	 	 "SELECT * FROM symbols WHERE id = 17;"};

    std::map<std::string, std::vector<std::string>> typesMap{};

    rc = sqlite3_exec(database, getFieldsSymbols.c_str(), callback, &typesMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(typesMap["6"].at(0) == "1");
    REQUIRE(typesMap["6"].at(1) == "int32_t");
    REQUIRE(typesMap["6"].at(2) == std::to_string(sizeof(int32_t)));

    REQUIRE(typesMap["10"].at(0) == "1");
    REQUIRE(typesMap["10"].at(1) == "uint8_t");
    REQUIRE(typesMap["10"].at(2) == std::to_string(sizeof(uint8_t)));

    REQUIRE(typesMap["17"].at(0) == "1");
    REQUIRE(typesMap["17"].at(1) == "float");
    REQUIRE(typesMap["17"].at(2) == std::to_string(sizeof(float)));

    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;
}


TEST_CASE("Test the correctness of the flat_array array after Juicer has processed it." ,"[Juicer]")
{
	/**
	 * This assumes that the test_file was compiled on
	 * gcc (Ubuntu 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609
	 *  little-endian machine.
	 */
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int 			rc;
    char* 			errorMessage = nullptr;
    std::string 	little_endian = is_little_endian()? "1": "0";

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{TEST_FILE_DIR};

    inputFile += "/test_file.o";

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(moduleName, inputFile);

    REQUIRE(rc == JUICER_OK);

    std::string getFlatArrayQuery{"SELECT * FROM fields WHERE name = \"flat_array\"; "};

    ((SQLiteDB*)(idc))->close();

    sqlite3 *database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::map<std::string, std::vector<std::string>> flatArrayMap{};

    rc = sqlite3_exec(database, getFlatArrayQuery.c_str(), callback, &flatArrayMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     * Check the correctness of Circle struct.
     */

    REQUIRE(flatArrayMap["2"].at(0) == "5");
    REQUIRE(flatArrayMap["2"].at(1) == "flat_array");
    REQUIRE(flatArrayMap["2"].at(2) == "0");
    REQUIRE(flatArrayMap["2"].at(3) == "5");
    REQUIRE(flatArrayMap["2"].at(4) == "6");
    REQUIRE(flatArrayMap["2"].at(5) == little_endian);

    /**
     *Check the correctness of the type
     */
    std::string getFieldsSymbols{"SELECT * FROM symbols WHERE id = 5;"};

    std::map<std::string, std::vector<std::string>> typesMap{};

    rc = sqlite3_exec(database, getFieldsSymbols.c_str(), callback, &typesMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(typesMap["5"].at(0) == "1");
    REQUIRE(typesMap["5"].at(1) == "int");
    REQUIRE(typesMap["5"].at(2) == std::to_string(sizeof(int)));

    /**
     * *Clean up our database handle and objects in memory.
     */

    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;
}

TEST_CASE("Write keys to database that already exist" ,"[Juicer]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{TEST_FILE_DIR};

    inputFile += "/test_file.o";

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
    std::string inputFile{TEST_FILE_DIR};

    inputFile += "/test_file.o";

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
    std::string inputFile{TEST_FILE_DIR};

    inputFile += "/test_file.o";

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

TEST_CASE("Write Elf File to database with invalid verbosity")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{-1};

    logger.logWarning("This is just a test.");

    std::string moduleName{"ABC"};
    std::string inputFile{TEST_FILE_DIR};

    inputFile += "/test_file.o";

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

