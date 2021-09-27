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

//DO NOT rename this macro to something like SQLITE_NULL as that is a macro that exists in sqlite3
#define TEST_NULL_STR "NULL"

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
 *@brief This callback returns the record inside of a std::vector<std::string>
 *
 */
static int selectVectorCallback(void *veryUsed, int argc, char **argv, char **azColName)
{
  int   i;
  auto* tableData = ( std::vector<std::vector<std::string>>*)veryUsed;

  std::vector<std::string> recordVector{};

  for(i=0; i<argc; i++)
  {
	  std::string tempData {TEST_NULL_STR};
	  if(argv[i] != nullptr)
	  {
		  tempData = argv[i];
	  }

	  recordVector.push_back(tempData);
  }

  tableData->push_back(recordVector);
  return 0;
}

struct columnNameToRowMap
{
	std::string colName{};
	std::map<std::string, std::vector<std::string> > recordMap{};
};

/**
 * Scans every column in the record and stores as a map in veryUsed.
 *
 * The map looks like this:
 *
 * {"symbol": ["col_val_1", "col_val_2", "col_val_3", col_val_N]}
 *
 * columns that are of value NULL the value will be set to "NULL".
 *
 * For example; the symbol record {"19"	"1"	"char"	"1"}, assuming that
 * columnNameToRowMap.colName = "name", will be added to the map as:
 * {"char": ["19", "1", "char", "1"]}
 *
 * the one and only key to the map is configurable via the colName field of columnNameToRowMap structure.
 */
static int selectCallbackUsingColNameAsKey(void *veryUsed, int argc, char **argv, char **azColName)
{
	columnNameToRowMap* mappingData = (columnNameToRowMap*) veryUsed;
	auto* row = (std::map<std::string, std::vector<std::string> >*)(&mappingData->recordMap);
	int key_index  = 0;
	std::vector<std::string> tableData{};

	for(int i=0; i<argc; i++)
	{
	  std::string tempData {TEST_NULL_STR};
	  if(argv[i] != nullptr)
	  {
		  tempData.assign(argv[i]);
	  }

	  if (strcmp(mappingData->colName.c_str(), azColName[i]) == 0)
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

    columnNameToRowMap circleDataMap{};
    circleDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> circleMap{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingColNameAsKey, &circleDataMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    circleMap = circleDataMap.recordMap;
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


    columnNameToRowMap fieldsDataMap{};
    fieldsDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> fieldsMap{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingColNameAsKey, &fieldsDataMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    fieldsMap = fieldsDataMap.recordMap;

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

    columnNameToRowMap diameterTypeDataMap{};
    diameterTypeDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> diameterTypeMap{};

    rc = sqlite3_exec(database, getDiameterType.c_str(), selectCallbackUsingColNameAsKey, &diameterTypeDataMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    diameterTypeMap = diameterTypeDataMap.recordMap;

    std::string  diameterType{diameterTypeMap["float"].at(0)};

    REQUIRE(fieldsMap["diameter"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["diameter"].at(2) == "diameter");
    REQUIRE(fieldsMap["diameter"].at(3) == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(fieldsMap["diameter"].at(4) == diameterType);
    REQUIRE(fieldsMap["diameter"].at(5) == TEST_NULL_STR);
    REQUIRE(fieldsMap["diameter"].at(6) == little_endian);

    std::string getRadiusType{"SELECT * FROM symbols where id="};

    getRadiusType += fieldsMap["diameter"].at(4);
    getRadiusType += ";";

    columnNameToRowMap radiusTypeDataMap{};
    radiusTypeDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> radiusTypeMap{};

    rc = sqlite3_exec(database, getRadiusType.c_str(), selectCallbackUsingColNameAsKey, &radiusTypeDataMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    radiusTypeMap = radiusTypeDataMap.recordMap;

    std::string  radiusType{radiusTypeMap["float"].at(0)};

    REQUIRE(fieldsMap["radius"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["radius"].at(2) == "radius");
    REQUIRE(fieldsMap["radius"].at(3) == std::to_string(offsetof(Circle, radius)));
    REQUIRE(fieldsMap["radius"].at(4) == radiusType);
    REQUIRE(fieldsMap["radius"].at(5) == TEST_NULL_STR);
    REQUIRE(fieldsMap["radius"].at(6) == little_endian);

    std::string getPointsType{"SELECT * FROM symbols where id="};

    getPointsType += fieldsMap["points"].at(4);
    getPointsType += ";";

    columnNameToRowMap pointsTypeDataMap{};
    pointsTypeDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> PointsTypeMap{};

    rc = sqlite3_exec(database, getPointsType.c_str(), selectCallbackUsingColNameAsKey, &pointsTypeDataMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    PointsTypeMap = pointsTypeDataMap.recordMap;

    std::string  PointsType{PointsTypeMap["int"].at(0)};

    std::string dimension_lists_id{fieldsMap["points"].at(5)};

    std::string getDimensionListsRecords{"SELECT * FROM symbols where id="};
    getDimensionListsRecords += dimension_lists_id;
    getDimensionListsRecords += ";";

    std::vector<std::vector<std::string>> dimensionListsList{};

    rc = sqlite3_exec(database, getDimensionListsRecords.c_str(), selectVectorCallback, &dimensionListsList,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    //Ensure order of all records by "dim_order" column
    std::sort(dimensionListsList.begin(),
    		  dimensionListsList.end(),
			  [](std::vector<std::string> a, std::vector<std::string> b)
			  {
    			return std::stoi(a.at(2)) <  std::stoi(b.at(2));
			  });

    //TODO:Test correctness of dimension_lists records

    REQUIRE(fieldsMap["points"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["points"].at(2) == "points");
    REQUIRE(fieldsMap["points"].at(3) == std::to_string(offsetof(Circle, points)));
    REQUIRE(fieldsMap["points"].at(4) == PointsType);
//    REQUIRE(fieldsMap["points"].at(5) == "128");
    REQUIRE(fieldsMap["points"].at(6) == little_endian);

    /**
     *Check the correctness of the types
     */
    std::string getDiameterFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getDiameterFieldTypes += diameterType;
    getDiameterFieldTypes += ";";

    columnNameToRowMap diameterFieldTypesDataMap{};
    diameterFieldTypesDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> diameterFieldTypesMap{};

    rc = sqlite3_exec(database, getDiameterFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &diameterFieldTypesDataMap,
                             &errorMessage);

    diameterFieldTypesMap = diameterFieldTypesDataMap.recordMap;

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(diameterFieldTypesMap.find("float") != diameterFieldTypesMap.end());

    REQUIRE(diameterFieldTypesMap["float"].at(2) == "float");
    REQUIRE(diameterFieldTypesMap["float"].at(3) == std::to_string(sizeof(float)));

    std::string getRadiusFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getRadiusFieldTypes += radiusType;
    getRadiusFieldTypes += ";";

    columnNameToRowMap radiusFieldTypesDataMap{};
    radiusFieldTypesDataMap.colName = "name";
    std::map<std::string, std::vector<std::string>> radiusFieldTypesMap{};

    rc = sqlite3_exec(database, getRadiusFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &radiusFieldTypesDataMap,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    radiusFieldTypesMap = radiusFieldTypesDataMap.recordMap;

    REQUIRE(radiusFieldTypesMap["float"].at(2) == "float");
    REQUIRE(radiusFieldTypesMap["float"].at(3) == std::to_string(sizeof(float)));

    std::string getPointsFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getPointsFieldTypes += PointsType;
    getPointsFieldTypes += ";";

    columnNameToRowMap pointsFieldTypesDataMap{};
    pointsFieldTypesDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> pointsFieldTypesMap{};

    rc = sqlite3_exec(database, getPointsFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &pointsFieldTypesDataMap,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    pointsFieldTypesMap = pointsFieldTypesDataMap.recordMap;

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

    columnNameToRowMap circleDataMap{};
    circleDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> circleMap{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingColNameAsKey, &circleDataMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    circleMap = circleDataMap.recordMap;

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

    columnNameToRowMap circleFieldsDataMap{};
    circleFieldsDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> fieldsMap{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingColNameAsKey, &circleFieldsDataMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    fieldsMap = circleFieldsDataMap.recordMap;


    /**
     *Check the correctness of the fields
     */

    std::string getDiameterType{"SELECT * FROM symbols where id="};

    getDiameterType += fieldsMap["diameter"].at(4);
    getDiameterType += ";";

    columnNameToRowMap diameterTypeDataMap{};
    diameterTypeDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> diameterTypeMap{};

    rc = sqlite3_exec(database, getDiameterType.c_str(), selectCallbackUsingColNameAsKey, &diameterTypeDataMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    diameterTypeMap = diameterTypeDataMap.recordMap;

    std::string  diameterType{diameterTypeMap["float"].at(0)};

    REQUIRE(fieldsMap["diameter"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["diameter"].at(2) == "diameter");
    REQUIRE(fieldsMap["diameter"].at(3) == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(fieldsMap["diameter"].at(4) == diameterType);
    REQUIRE(fieldsMap["diameter"].at(5) == TEST_NULL_STR);
    REQUIRE(fieldsMap["diameter"].at(6) == little_endian);

    std::string getRadiusType{"SELECT * FROM symbols where id="};

    getRadiusType += fieldsMap["diameter"].at(4);
    getRadiusType += ";";

    columnNameToRowMap radiusTypeMapDataMap{};
    radiusTypeMapDataMap.colName = "name";
    std::map<std::string, std::vector<std::string>> radiusTypeMap{};


    rc = sqlite3_exec(database, getRadiusType.c_str(), selectCallbackUsingColNameAsKey, &radiusTypeMapDataMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    radiusTypeMap = radiusTypeMapDataMap.recordMap;

    std::string  radiusType{radiusTypeMap.at("float").at(0)};

    REQUIRE(fieldsMap["radius"].at(1) == circleMap["Circle"].at(0));
    REQUIRE(fieldsMap["radius"].at(2) == "radius");
    REQUIRE(fieldsMap["radius"].at(3) == std::to_string(offsetof(Circle, radius)));
    REQUIRE(fieldsMap["radius"].at(4) == radiusType);
    REQUIRE(fieldsMap["radius"].at(5) == "NULL");
    REQUIRE(fieldsMap["radius"].at(6) == little_endian);

    /**
     *Check the correctness of the types
     */
    std::string getDiameterFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getDiameterFieldTypes += diameterType;
    getDiameterFieldTypes += ";";

    columnNameToRowMap diameterFieldTypesDataMap{};
    diameterFieldTypesDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> diameterFieldTypesMap{};

    rc = sqlite3_exec(database, getDiameterFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &diameterFieldTypesDataMap,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    diameterFieldTypesMap = diameterFieldTypesDataMap.recordMap;

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

    std::string getSquareStructQuery{"SELECT * FROM symbols WHERE name = \"Square\"; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3 *database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    columnNameToRowMap squareMapDataMap{};
    squareMapDataMap.colName = "name";
    std::map<std::string, std::vector<std::string>> squareMap{};

    rc = sqlite3_exec(database, getSquareStructQuery.c_str(), selectCallbackUsingColNameAsKey, &squareMapDataMap,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    /**
     * Check the correctness of Square struct.
     */
    squareMap = squareMapDataMap.recordMap;

    REQUIRE(squareMap["Square"].at(2) == "Square");
    REQUIRE(squareMap["Square"].at(3) == std::to_string(sizeof(Square)));


    std::string square_id = squareMap["Square"].at(0);

    std::string getSquareFields{"SELECT * FROM fields WHERE symbol = "};

    getSquareFields += square_id;
    getSquareFields += ";";

    columnNameToRowMap fieldsMapDataMap{};
    fieldsMapDataMap.colName = "name";
    std::map<std::string, std::vector<std::string>> fieldsMap{};

    rc = sqlite3_exec(database, getSquareFields.c_str(), selectCallbackUsingColNameAsKey, &fieldsMapDataMap,
                             &errorMessage);

    fieldsMap = fieldsMapDataMap.recordMap;
    REQUIRE(rc == SQLITE_OK);

    std::string getWidthType{"SELECT * FROM symbols where id="};

    getWidthType += fieldsMap["width"].at(4);
    getWidthType += ";";

    columnNameToRowMap widthTypeMapDataMap{};
    widthTypeMapDataMap.colName = "name";

    std::map<std::string, std::vector<std::string>> widthTypeMap{};

    rc = sqlite3_exec(database, getWidthType.c_str(), selectCallbackUsingColNameAsKey, &widthTypeMapDataMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    widthTypeMap = widthTypeMapDataMap.recordMap;

    REQUIRE(widthTypeMap.find("int32_t") != widthTypeMap.end());

    std::string  widthType{widthTypeMap["int32_t"].at(0)};

    REQUIRE(fieldsMap["width"].at(1) == squareMap["Square"].at(0));
    REQUIRE(fieldsMap["width"].at(2) == "width");
    REQUIRE(fieldsMap["width"].at(3) == std::to_string(offsetof(Square, width)));
    REQUIRE(fieldsMap["width"].at(4) == widthType);
    REQUIRE(fieldsMap["width"].at(5) == "NULL");
    REQUIRE(fieldsMap["width"].at(6) == little_endian);

    //Flat array test
    REQUIRE(rc == SQLITE_OK);

    std::string getmatrix1DType{"SELECT * FROM symbols where id="};

    getmatrix1DType += fieldsMap["matrix1D"].at(4);
    getmatrix1DType += ";";

    columnNameToRowMap matrix1DTypeTypeMapDataMap{};
    matrix1DTypeTypeMapDataMap.colName = "name";
    std::map<std::string, std::vector<std::string>> matrix1DTypeTypeMap{};

    rc = sqlite3_exec(database, getmatrix1DType.c_str(), selectCallbackUsingColNameAsKey, &matrix1DTypeTypeMapDataMap,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    matrix1DTypeTypeMap = matrix1DTypeTypeMapDataMap.recordMap;

    REQUIRE(matrix1DTypeTypeMap.find("float") != matrix1DTypeTypeMap.end());

    std::string matrix1DType{matrix1DTypeTypeMap["float"].at(0)};

    REQUIRE(fieldsMap["matrix1D"].at(5) != TEST_NULL_STR);

    std::string getDimensionLists{"SELECT * FROM dimension_lists WHERE id="};

    std::map<std::string, std::vector<std::string>> dimensionListsMap{};
//
//    rc = sqlite3_exec(database, getSquareFields.c_str(), selectCallbackUsingNameAsKey, &fieldsMap,
//                             &errorMessage);
//
//    std::string dimensionListsId{fieldsMap["matrix1D"].at(5)};
//
//    REQUIRE(fieldsMap["matrix1D"].at(1) == squareMap["Square"].at(0));
//    REQUIRE(fieldsMap["matrix1D"].at(2) == "matrix1D");
//    REQUIRE(fieldsMap["matrix1D"].at(3) == std::to_string(offsetof(Square, matrix1D)));
//    REQUIRE(fieldsMap["matrix1D"].at(4) == matrix1DType);
////    REQUIRE(fieldsMap["matrix1D"].at(5) == "27");
//    REQUIRE(fieldsMap["matrix1D"].at(6) == little_endian);

    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;
}

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

