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
#include <map>
#include <string>
#include <stddef.h>
#include <string.h>
#include "test_file1.h"

/**
 *These test file locations assumes that the tests are run
 * with "make run-tests".
 */
#define TEST_FILE_1 "ut_obj/test_file1.o"
#define TEST_FILE_2 "ut_obj/test_file2.o"

/**
 *Checks if the platform is little endian.
 *This is used as a source of truth for our unit tests.
 */
bool is_little_endian()
{
	int n = 1;
	return (*(char *)&n == 1);
}


static int selectCallbackUsingNameAsKey(void *veryUsed, int argc, char **argv, char **azColName)
{
  auto* row = (std::map<std::string, std::vector<std::string> >*)veryUsed ;
  int key_index  = 0;
  std::vector<std::string> tableData{};

  for(int i=0; i<argc; i++)
  {
	  std::string tempData{argv[i]};

	  if (strcmp("name", azColName[i]) == 0)
	  {
		  key_index = i;
	  }
		  tableData.push_back(tempData);
  }

  std::string id{argv[key_index]};

  (*row)[id] = tableData;

  return 0;
}

TEST_CASE("Test Juicer at the highest level with SQLiteDB" ,"[main_test#1]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");

    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    REQUIRE(juicer.parse(inputFile) == JUICER_OK);

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;

}


TEST_CASE("Test the correctness of the Circle struct after Juicer has processed it." ,"[main_test#2]")
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
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE(rc == JUICER_OK);

    std::string getCircleStructQuery{"SELECT * FROM symbols WHERE name = \"Circle\"; "};

    sqlite3 *database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::map<std::string, std::vector<std::string>> circleMap{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingNameAsKey, &circleMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     * Check the correctness of Circle struct.
     */

    REQUIRE(circleMap.find("Circle") != circleMap.end());

    REQUIRE(circleMap["Circle"].at(2) == "Circle");
    REQUIRE(circleMap["Circle"].at(3) == std::to_string(sizeof(Circle)));


    /**
     *Check the fields of the Circle struct.
     */

    std::string circle_id = circleMap["Circle"].at(0);

    std::string getCircleFields{"SELECT * FROM fields WHERE symbol = "};

    getCircleFields += circle_id;
    getCircleFields += ";";

    std::map<std::string, std::vector<std::string>> fieldsMap{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingNameAsKey, &fieldsMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     * Ensure that we have all of the expected keys in our map; these are the field names.
     */
    REQUIRE(fieldsMap.find("diameter") != fieldsMap.end());
    REQUIRE(fieldsMap.find("radius") != fieldsMap.end());
    REQUIRE(fieldsMap.find("points") != fieldsMap.end());

    /**
     *Check the correctness of the fields
     */

    std::string getDiameterType{"SELECT * FROM symbols where id="};

    getDiameterType += fieldsMap["diameter"].at(4);
    getDiameterType += ";";

    std::map<std::string, std::vector<std::string>> diameterTypeMap{};


    rc = sqlite3_exec(database, getDiameterType.c_str(), selectCallbackUsingNameAsKey, &diameterTypeMap,
                               &errorMessage);

      REQUIRE(rc == SQLITE_OK);

    std::string  diameterType{diameterTypeMap["float"].at(0)};

    REQUIRE(fieldsMap["diameter"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["diameter"].at(2) == "diameter");
    REQUIRE(fieldsMap["diameter"].at(3) == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(fieldsMap["diameter"].at(4) == diameterType);
    REQUIRE(fieldsMap["diameter"].at(5) == "0");
    REQUIRE(fieldsMap["diameter"].at(6) == little_endian);

    std::string getRadiusType{"SELECT * FROM symbols where id="};

    getRadiusType += fieldsMap["diameter"].at(4);
    getRadiusType += ";";

    std::map<std::string, std::vector<std::string>> radiusTypeMap{};


    rc = sqlite3_exec(database, getRadiusType.c_str(), selectCallbackUsingNameAsKey, &radiusTypeMap,
                               &errorMessage);

      REQUIRE(rc == SQLITE_OK);

    std::string  radiusType{radiusTypeMap["float"].at(0)};

    REQUIRE(fieldsMap["radius"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["radius"].at(2) == "radius");
    REQUIRE(fieldsMap["radius"].at(3) == std::to_string(offsetof(Circle, radius)));
    REQUIRE(fieldsMap["radius"].at(4) == radiusType);
    REQUIRE(fieldsMap["radius"].at(5) == "0");
    REQUIRE(fieldsMap["radius"].at(6) == little_endian);

    std::string getPointsType{"SELECT * FROM symbols where id="};

    getPointsType += fieldsMap["points"].at(4);
    getPointsType += ";";

    std::map<std::string, std::vector<std::string>> PointsTypeMap{};

    rc = sqlite3_exec(database, getPointsType.c_str(), selectCallbackUsingNameAsKey, &PointsTypeMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);



    std::string  PointsType{PointsTypeMap["int"].at(0)};

    REQUIRE(fieldsMap["points"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["points"].at(2) == "points");
    REQUIRE(fieldsMap["points"].at(3) == std::to_string(offsetof(Circle, points)));
    REQUIRE(fieldsMap["points"].at(4) == PointsType);
    REQUIRE(fieldsMap["points"].at(5) == "128");
    REQUIRE(fieldsMap["points"].at(6) == little_endian);

    /**
     *Check the correctness of the types
     */
    std::string getDiameterFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getDiameterFieldTypes += diameterType;
    getDiameterFieldTypes += ";";

    std::map<std::string, std::vector<std::string>> diameterFieldTypesMap{};

    rc = sqlite3_exec(database, getDiameterFieldTypes.c_str(), selectCallbackUsingNameAsKey, &diameterFieldTypesMap,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(diameterFieldTypesMap.find("float") != diameterFieldTypesMap.end());

    REQUIRE(diameterFieldTypesMap["float"].at(2) == "float");
    REQUIRE(diameterFieldTypesMap["float"].at(3) == std::to_string(sizeof(float)));

    std::string getRadiusFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getRadiusFieldTypes += radiusType;
    getRadiusFieldTypes += ";";

    std::map<std::string, std::vector<std::string>> radiusFieldTypesMap{};

    rc = sqlite3_exec(database, getRadiusFieldTypes.c_str(), selectCallbackUsingNameAsKey, &radiusFieldTypesMap,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(radiusFieldTypesMap["float"].at(2) == "float");
    REQUIRE(radiusFieldTypesMap["float"].at(3) == std::to_string(sizeof(float)));


    std::string getPointsFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getPointsFieldTypes += PointsType;
    getPointsFieldTypes += ";";

    std::map<std::string, std::vector<std::string>> pointsFieldTypesMap{};

    rc = sqlite3_exec(database, getPointsFieldTypes.c_str(), selectCallbackUsingNameAsKey, &pointsFieldTypesMap,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(pointsFieldTypesMap["int"].at(2) == "int");
    REQUIRE(pointsFieldTypesMap["int"].at(3) == std::to_string(sizeof(int)));

    /**
     * *Clean up our database handle and objects in memory.
     */
    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;
}

TEST_CASE("Test the correctness of the Circle struct after Juicer has processed it on two"
		  " different elf files." ,"[main_test#3]")
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
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE(rc == JUICER_OK);

    inputFile = TEST_FILE_2;

    rc = juicer.parse(inputFile);

    REQUIRE(rc == JUICER_OK);

    std::string getCircleStructQuery{"SELECT * FROM symbols WHERE name = \"Circle\"; "};

    sqlite3 *database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::map<std::string, std::vector<std::string>> circleMap{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingNameAsKey, &circleMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     * Check the correctness of Circle struct.
     */

    REQUIRE(circleMap["Circle"].at(2) == "Circle");
    REQUIRE(circleMap["Circle"].at(3) == std::to_string(sizeof(Circle)));

    /**
     *Check the fields of the Circle struct.
    */

    std::string circle_id = circleMap["Circle"].at(0);

    std::string getCircleFields{"SELECT * FROM fields WHERE symbol = "};

    getCircleFields += circle_id;
    getCircleFields += ";";

    std::map<std::string, std::vector<std::string>> fieldsMap{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingNameAsKey, &fieldsMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    /**
     *Check the correctness of the fields
     */

    std::string getDiameterType{"SELECT * FROM symbols where id="};

    getDiameterType += fieldsMap["diameter"].at(4);
    getDiameterType += ";";

    std::map<std::string, std::vector<std::string>> diameterTypeMap{};


    rc = sqlite3_exec(database, getDiameterType.c_str(), selectCallbackUsingNameAsKey, &diameterTypeMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    std::string  diameterType{diameterTypeMap["float"].at(0)};

    REQUIRE(fieldsMap["diameter"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["diameter"].at(2) == "diameter");
    REQUIRE(fieldsMap["diameter"].at(3) == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(fieldsMap["diameter"].at(4) == diameterType);
    REQUIRE(fieldsMap["diameter"].at(5) == "0");
    REQUIRE(fieldsMap["diameter"].at(6) == little_endian);

    std::string getRadiusType{"SELECT * FROM symbols where id="};

    getRadiusType += fieldsMap["diameter"].at(4);
    getRadiusType += ";";

    std::map<std::string, std::vector<std::string>> radiusTypeMap{};


    rc = sqlite3_exec(database, getRadiusType.c_str(), selectCallbackUsingNameAsKey, &radiusTypeMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    std::string  radiusType{radiusTypeMap["float"].at(0)};

    REQUIRE(fieldsMap["radius"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["radius"].at(2) == "radius");
    REQUIRE(fieldsMap["radius"].at(3) == std::to_string(offsetof(Circle, radius)));
    REQUIRE(fieldsMap["radius"].at(4) == radiusType);
    REQUIRE(fieldsMap["radius"].at(5) == "0");
    REQUIRE(fieldsMap["radius"].at(6) == little_endian);

    /**
     *Check the correctness of the types
     */
    std::string getDiameterFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getDiameterFieldTypes += diameterType;
    getDiameterFieldTypes += ";";

    std::map<std::string, std::vector<std::string>> diameterFieldTypesMap{};

    rc = sqlite3_exec(database, getDiameterFieldTypes.c_str(), selectCallbackUsingNameAsKey, &diameterFieldTypesMap,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(diameterFieldTypesMap["float"].at(2) == "float");
    REQUIRE(diameterFieldTypesMap["float"].at(3) == std::to_string(sizeof(float)));

    /**
     * *Clean up our database handle and objects in memory.
     */
    REQUIRE(remove("./test_db.sqlite")==0);
    ((SQLiteDB*)(idc))->close();
    delete idc;
}

TEST_CASE("Test the correctness of the Square struct after Juicer has processed it." ,"[main_test#4]")
{
	/**
	 * This assumes that the test_file was compiled on
	 * gcc (Ubuntu 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609
	 *  little-endian machine.
	 * @todo Implement
	 */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int 			rc = 0;
    char* 			errorMessage = nullptr;
    std::string 	little_endian = is_little_endian()? "1": "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE(rc == JUICER_OK);

    std::string getCircleStructQuery{"SELECT * FROM symbols WHERE name = \"Square\"; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3 *database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::map<std::string, std::vector<std::string>> squareMap{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingNameAsKey, &squareMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);
//
    /**
     * Check the correctness of Square struct.
     */
    REQUIRE(squareMap["Square"].at(2) == "Square");
    REQUIRE(squareMap["Square"].at(3) == std::to_string(sizeof(Square)));


    std::string square_id = squareMap["Square"].at(0);
//
    std::string getSquareFields{"SELECT * FROM fields WHERE symbol = "};
//
    getSquareFields += square_id;
    getSquareFields += ";";
//
    std::map<std::string, std::vector<std::string>> fieldsMap{};
//
    rc = sqlite3_exec(database, getSquareFields.c_str(), selectCallbackUsingNameAsKey, &fieldsMap,
                             &errorMessage);
//
    REQUIRE(rc == SQLITE_OK);

    std::string getWidthType{"SELECT * FROM symbols where id="};

    getWidthType += fieldsMap["width"].at(4);
    getWidthType += ";";

    std::map<std::string, std::vector<std::string>> widthTypeMap{};


    rc = sqlite3_exec(database, getWidthType.c_str(), selectCallbackUsingNameAsKey, &widthTypeMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    std::string  widthType{widthTypeMap["int32_t"].at(0)};

    REQUIRE(fieldsMap["width"].at(1) == squareMap["Square"].at(0));
    REQUIRE(fieldsMap["width"].at(2) == "width");
    REQUIRE(fieldsMap["width"].at(3) == std::to_string(offsetof(Square, width)));
    REQUIRE(fieldsMap["width"].at(4) == widthType);
    REQUIRE(fieldsMap["width"].at(5) == "0");
    REQUIRE(fieldsMap["width"].at(6) == little_endian);


//
//    /**
//     *Check the fields of the Square struct.
//     */
//    std::string getCircleFields{"SELECT * FROM fields WHERE symbol = 18;"};
//
//    std::map<std::string, std::vector<std::string>> fieldsMap{};
//
//    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingNameAsKey, &fieldsMap,
//                             &errorMessage);
//
//    REQUIRE(rc == SQLITE_OK);
//
//    /**
//     *Check the correctness of the fields
//     */
//    REQUIRE(fieldsMap.at("4").at(0) == "18");
//    REQUIRE(fieldsMap.at("4").at(1) == "width");
//    REQUIRE(fieldsMap.at("4").at(2) == std::to_string(offsetof(Square, width)));
//    REQUIRE(fieldsMap.at("4").at(3) == "6");
//    REQUIRE(fieldsMap.at("4").at(4) == "0");
//    REQUIRE(fieldsMap.at("4").at(5) == little_endian);
//
//    REQUIRE(fieldsMap["5"].at(0) == "18");
//    REQUIRE(fieldsMap["5"].at(1) == "stuff");
//    REQUIRE(fieldsMap["5"].at(2) == std::to_string(offsetof(Square, stuff)));
//    REQUIRE(fieldsMap["5"].at(3) == "10");
//    REQUIRE(fieldsMap["5"].at(4) == "0");
//    REQUIRE(fieldsMap["5"].at(5) == little_endian);
//
//
//    REQUIRE(fieldsMap["6"].at(0) == "18");
//    REQUIRE(fieldsMap["6"].at(1) == "length");
//    REQUIRE(fieldsMap["6"].at(2) == std::to_string(offsetof(Square, length)));
//    REQUIRE(fieldsMap["6"].at(3) == "6");
//    REQUIRE(fieldsMap["6"].at(4) == "0");
//    REQUIRE(fieldsMap["6"].at(5) == little_endian);
//
//    REQUIRE(fieldsMap["7"].at(0) == "18");
//    REQUIRE(fieldsMap["7"].at(1) == "more_stuff");
//    REQUIRE(fieldsMap["7"].at(2) == std::to_string(offsetof(Square, more_stuff)));
//    REQUIRE(fieldsMap["7"].at(3) == "10");
//    REQUIRE(fieldsMap["7"].at(4) == "0");
//    REQUIRE(fieldsMap["7"].at(5) == little_endian);
//
//
//    REQUIRE(fieldsMap["8"].at(0) == "18");
//    REQUIRE(fieldsMap["8"].at(1) == "floating_stuff");
//    REQUIRE(fieldsMap["8"].at(2) == std::to_string(offsetof(Square, floating_stuff)));
//    REQUIRE(fieldsMap["8"].at(3) == "17");
//    REQUIRE(fieldsMap["8"].at(4) == "0");
//    REQUIRE(fieldsMap["8"].at(5) == little_endian);
//
//    /**
//     *Check the correctness of the types
//     */
//    std::string getFieldsSymbols{"SELECT * FROM symbols WHERE id = 6;"
//    						     "SELECT * FROM symbols WHERE id = 10;"
//		 	 	 	 	 	 	 "SELECT * FROM symbols WHERE id = 17;"};
//
//    std::map<std::string, std::vector<std::string>> typesMap{};
//
//    rc = sqlite3_exec(database, getFieldsSymbols.c_str(), selectCallbackUsingNameAsKey, &typesMap,
//                             &errorMessage);
//
//    REQUIRE(rc == SQLITE_OK);
//
//    REQUIRE(typesMap["6"].at(0) == "1");
//    REQUIRE(typesMap["6"].at(1) == "int32_t");
//    REQUIRE(typesMap["6"].at(2) == std::to_string(sizeof(int32_t)));
//
//    REQUIRE(typesMap["10"].at(0) == "1");
//    REQUIRE(typesMap["10"].at(1) == "uint8_t");
//    REQUIRE(typesMap["10"].at(2) == std::to_string(sizeof(uint8_t)));
//
//    REQUIRE(typesMap["17"].at(0) == "1");
//    REQUIRE(typesMap["17"].at(1) == "float");
//    REQUIRE(typesMap["17"].at(2) == std::to_string(sizeof(float)));
//
    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;
}
//
//
//TEST_CASE("Test the correctness of the flat_array array after Juicer has processed it." ,"[main_test#5]")
//{
//	/**
//	 * This assumes that the test_file was compiled on
//	 * gcc (Ubuntu 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609
//	 *  little-endian machine.
//	 */
//    Juicer          juicer;
//    IDataContainer* idc = 0;
//    Logger          logger;
//    int 			rc;
//    char* 			errorMessage = nullptr;
//    std::string 	little_endian = is_little_endian()? "1": "0";
//
//    logger.logWarning("This is just a test.");
//
//    std::string inputFile{TEST_FILE_1};
//
//
//    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
//    REQUIRE(idc!=nullptr);
//    logger.logInfo("IDataContainer was constructed successfully for unit test.");
//
//    juicer.setIDC(idc);
//
//    rc = juicer.parse(inputFile);
//
//    REQUIRE(rc == JUICER_OK);
//
//    std::string getFlatArrayQuery{"SELECT * FROM fields WHERE name = \"flat_array\"; "};
//
//    ((SQLiteDB*)(idc))->close();
//
//    sqlite3 *database;
//
//    rc = sqlite3_open("./test_db.sqlite", &database);
//
//    REQUIRE(rc == SQLITE_OK);
//
//    std::map<std::string, std::vector<std::string>> flatArrayMap{};
//
//    rc = sqlite3_exec(database, getFlatArrayQuery.c_str(), selectCallbackUsingNameAsKey, &flatArrayMap,
//                             &errorMessage);
//
//    REQUIRE(rc == SQLITE_OK);
//
//    /**
//     * Check the correctness of Circle struct.
//     */
//
//    REQUIRE(flatArrayMap["2"].at(0) == "5");
//    REQUIRE(flatArrayMap["2"].at(1) == "flat_array");
//    REQUIRE(flatArrayMap["2"].at(2) == "0");
//    REQUIRE(flatArrayMap["2"].at(3) == "5");
//    REQUIRE(flatArrayMap["2"].at(4) == "6");
//    REQUIRE(flatArrayMap["2"].at(5) == little_endian);
//
//    /**
//     *Check the correctness of the type
//     */
//    std::string getFieldsSymbols{"SELECT * FROM symbols WHERE id = 5;"};
//
//    std::map<std::string, std::vector<std::string>> typesMap{};
//
//    rc = sqlite3_exec(database, getFieldsSymbols.c_str(), selectCallbackUsingNameAsKey, &typesMap,
//                             &errorMessage);
//
//    REQUIRE(rc == SQLITE_OK);
//
//    REQUIRE(typesMap["5"].at(0) == "1");
//    REQUIRE(typesMap["5"].at(1) == "int");
//    REQUIRE(typesMap["5"].at(2) == std::to_string(sizeof(int)));
//
//    /**
//     * *Clean up our database handle and objects in memory.
//     */
//
//    REQUIRE(remove("./test_db.sqlite")==0);
//    delete idc;
//}
//
TEST_CASE("Write keys to database that already exist" ,"[main_test#6]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");

    std::string inputFile{TEST_FILE_1};


    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc!=nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    /**
     * Parse twice to write to the database twice and cause
     * existing-keys errors in the database.
     */
    REQUIRE(juicer.parse(inputFile) == JUICER_OK);
    REQUIRE(juicer.parse(inputFile) == JUICER_OK);
    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;

}
//
TEST_CASE("Write Elf File to database with a log file", "[main_test#7]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");

    std::string inputFile{TEST_FILE_1};

	idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
	REQUIRE(idc!=nullptr);
	logger.logInfo("IDataContainer was constructed successfully for unit test.");

	juicer.setIDC(idc);

	REQUIRE(juicer.parse(inputFile)==JUICER_OK);

	logger.setLogFile("logFile");

	/**
	*Clean up our database handle and objects in memory.
	*/
	((SQLiteDB*)(idc))->close();
	delete idc;
	REQUIRE(remove("./logFile")==0);
	REQUIRE(remove("./test_db.sqlite")==0);
}

TEST_CASE("Write Elf File to database with verbosity set to INFO", "[main_test#8]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{LOGGER_VERBOSITY_INFO};

    std::string inputFile{TEST_FILE_1};

	idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
	REQUIRE(idc!=nullptr);
	logger.logInfo("IDataContainer was constructed successfully for unit test.");

	juicer.setIDC(idc);
	juicer.parse(inputFile);

	/**
	*Clean up our database handle and objects in memory.
	*/
	((SQLiteDB*)(idc))->close();
	delete idc;
	REQUIRE(remove("./test_db.sqlite")==0);
}

TEST_CASE("Write Elf File to database with invalid verbosity", "[main_test#9]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{-1};

    logger.logWarning("This is just a test.");

    std::string inputFile{TEST_FILE_1};

	idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
	REQUIRE(idc!=nullptr);
	logger.logInfo("IDataContainer was constructed successfully for unit test.");

	juicer.setIDC(idc);
	juicer.parse(inputFile);

	/**
	*Clean up our database handle and objects in memory.
	*/
	((SQLiteDB*)(idc))->close();
	delete idc;
	REQUIRE(remove("./test_db.sqlite")==0);
}

