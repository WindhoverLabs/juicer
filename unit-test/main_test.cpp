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
#include <limits.h>
#include <cstdlib>
#include <strstream>


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
static int selectCallbackUsingCustomColNameAsKey(void *veryUsed, int argc, char **argv, char **azColName)
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

/**
 * Scans every column in the record and stores as a vector of map objects in veryUsed.
 *
 * The map looks like this:
 *
 * [ {"columnName1": "value", "columnName2": "value", "columnName3": "value"},
 * 	 {"columnName1": "value", "columnName2": "value", "columnName3": "value"},
 * 	 {"columnName1": "value", "columnName2": "value", "columnName3": "value"}
 * ]
 *
 * columns that are of value NULL the value will be set to "NULL". Note that every map in the vector
 * is a record in the database.
 *
 * For example; the symbol record {"19"	"1"	"char"	"1"} will be added to the vector as:
 *
 *   REQUIRE(circleRecords.at(0)["name"] == "Circle");
 *   REQUIRE(circleRecords.at(0)["byte_size"] == std::to_string(sizeof(Circle)));
 *
 *[ {"id:" "elf": "1", "name": "char", "byte_size":"1"} ]
 *
 */
static int selectCallbackUsingColNameAsKey(void *veryUsed, int argc, char **argv, char **azColName)
{
	auto* allRecords = (std::vector<std::map<std::string, std::string>>*) veryUsed;

	std::map<std::string, std::string> newRecord{};

	std::vector<std::string> tableData{};

	for(int i=0; i<argc; i++)
	{
	  std::string tempData {TEST_NULL_STR};
	  if(argv[i] != nullptr)
	  {
		  tempData.assign(argv[i]);
	  }

	newRecord[azColName[i]] = tempData;
	}

	allRecords->push_back(newRecord);

	return 0;
}


std::string getmd5sumFromSystem(char resolvedPath[PATH_MAX]) {
//	TODO:Unfortunately the redirect is adding junk(a "\n" character at the end) at the end of the crc.
	std::string MD5CommandStr { "md5sum " };
	MD5CommandStr += resolvedPath;
	MD5CommandStr += " >MD5.txt";
	std::system(MD5CommandStr.c_str()); // executes the UNIX command "ls -l >test.txt"
	std::strstream expectedMD5 { };
	expectedMD5 << std::ifstream("MD5.txt").rdbuf();
	REQUIRE(remove("./MD5.txt") == 0);
	std::string expectedMD5Str { expectedMD5.str() };

    //	Size should be size of hash(16 bytes)
	expectedMD5Str = expectedMD5Str.substr(0, 32);
	REQUIRE(expectedMD5Str.size() == 32);
	return expectedMD5Str;
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
	 * gcc (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0
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

    std::vector<std::map<std::string, std::string>> circleRecords{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingColNameAsKey, &circleRecords,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(circleRecords.size() == 1);
    /**
     * Check the correctness of Circle struct.
     */

   REQUIRE(circleRecords.at(0).find("name") != circleRecords.at(0).end());
   REQUIRE(circleRecords.at(0).find("byte_size") != circleRecords.at(0).end());
   REQUIRE(circleRecords.at(0).find("id") != circleRecords.at(0).end());

   REQUIRE(circleRecords.at(0)["name"] == "Circle");
   REQUIRE(circleRecords.at(0)["byte_size"] == std::to_string(sizeof(Circle)));

    /**
     *Check the fields of the Circle struct.
     */

    std::string circle_id = circleRecords.at(0)["id"];

    std::string getCircleFields{"SELECT * FROM fields WHERE symbol = "};

    getCircleFields += circle_id;
    getCircleFields += ";";

    std::vector<std::map<std::string, std::string>> fieldsRecords{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingColNameAsKey, &fieldsRecords,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(fieldsRecords.size() == 4);

    //Enforce order of records by offset
    std::sort(fieldsRecords.begin(), fieldsRecords.end(),
    		  [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
			  {
    			return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]);
			  });

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for(auto record: fieldsRecords)
    {
        REQUIRE(record.find("symbol") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_offset") != record.end());
        REQUIRE(record.find("type") != record.end());
    }

    REQUIRE(fieldsRecords.at(0)["name"] == "diameter");
    /**
     *Check the correctness of the fields
     */

    std::string getDiameterType{"SELECT * FROM symbols where id="};

    getDiameterType += fieldsRecords.at(0)["type"];
    getDiameterType += ";";

    std::vector<std::map<std::string, std::string>> dimaterSymbolRecords{};

    rc = sqlite3_exec(database, getDiameterType.c_str(), selectCallbackUsingColNameAsKey, &dimaterSymbolRecords,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(dimaterSymbolRecords.size() == 1);

    std::string  diameterType{dimaterSymbolRecords.at(0).at("id")};

    REQUIRE(fieldsRecords.at(0)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(fieldsRecords.at(0)["name"]   == "diameter");
    REQUIRE(fieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(fieldsRecords.at(0)["type"] == diameterType);
    REQUIRE(fieldsRecords.at(0)["little_endian"] == little_endian);

    REQUIRE(fieldsRecords.at(1)["name"] == "radius");

    std::string getRadiusType{"SELECT * FROM symbols where id="};

    getRadiusType += fieldsRecords.at(1)["type"];
    getRadiusType += ";";

    std::vector<std::map<std::string, std::string>> radiusSymbolRecord{};

    rc = sqlite3_exec(database, getRadiusType.c_str(), selectCallbackUsingColNameAsKey, &radiusSymbolRecord,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    std::string  radiusType{radiusSymbolRecord.at(0)["id"]};

    REQUIRE(fieldsRecords.at(1)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(fieldsRecords.at(1)["name"] == "radius");
    REQUIRE(fieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(Circle, radius)));
    REQUIRE(fieldsRecords.at(1)["type"] == radiusType);
    REQUIRE(fieldsRecords.at(1)["little_endian"] == little_endian);

    std::string getPointsType{"SELECT * FROM symbols where id="};

    getPointsType += fieldsRecords.at(2)["type"];
    getPointsType += ";";

    std::vector<std::map<std::string, std::string>> pointsSymbolRecord{};

    rc = sqlite3_exec(database, getPointsType.c_str(), selectCallbackUsingColNameAsKey, &pointsSymbolRecord,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);


    std::string  PointsType{pointsSymbolRecord.at(0)["id"]};

    REQUIRE(fieldsRecords.at(2)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(fieldsRecords.at(2)["name"] == "points");
    REQUIRE(fieldsRecords.at(2)["byte_offset"] == std::to_string(offsetof(Circle, points)));
    REQUIRE(fieldsRecords.at(2)["type"] == PointsType);
    REQUIRE(fieldsRecords.at(2)["little_endian"] == little_endian);

    /**
     *Check the correctness of the types
     */
    std::string getDiameterFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getDiameterFieldTypes += diameterType;
    getDiameterFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> diameterFieldSymbolRecord{};

    rc = sqlite3_exec(database, getDiameterFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &diameterFieldSymbolRecord,
                             &errorMessage);


    REQUIRE(rc == SQLITE_OK);
    REQUIRE(diameterFieldSymbolRecord.size() == 1);

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for(auto record: diameterFieldSymbolRecord)
    {
        REQUIRE(record.find("id") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_size") != record.end());
        REQUIRE(record.find("elf") != record.end());
    }

    REQUIRE(diameterFieldSymbolRecord.at(0)["name"] == "float");
    REQUIRE(diameterFieldSymbolRecord.at(0)["byte_size"] == std::to_string(sizeof(float)));

    std::string getRadiusFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getRadiusFieldTypes += radiusType;
    getRadiusFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> radiusFieldTypesRecords{};

    rc = sqlite3_exec(database, getRadiusFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &radiusFieldTypesRecords,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(radiusFieldTypesRecords.size() == 1);
    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for(auto record: radiusFieldTypesRecords)
    {
        REQUIRE(record.find("id") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_size") != record.end());
        REQUIRE(record.find("elf") != record.end());
    }

    REQUIRE(radiusFieldTypesRecords.at(0)["name"] == "float");
    REQUIRE(radiusFieldTypesRecords.at(0)["byte_size"] == std::to_string(sizeof(float)));

    std::string getPointsFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getPointsFieldTypes += PointsType;
    getPointsFieldTypes += ";";


    std::vector<std::map<std::string, std::string>> pointsFieldTypesRecords{};

    rc = sqlite3_exec(database, getPointsFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &pointsFieldTypesRecords,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(pointsFieldTypesRecords.size() == 1);

    REQUIRE(pointsFieldTypesRecords.at(0)["name"] == "int");
    REQUIRE(pointsFieldTypesRecords.at(0)["byte_size"] == std::to_string(sizeof(int)));


    REQUIRE(fieldsRecords.at(3)["name"] == "mode");

    std::string getModeType{"SELECT * FROM symbols where id="};

    getModeType += fieldsRecords.at(3)["type"];
    getModeType += ";";

    std::vector<std::map<std::string, std::string>> modeSymbolRecord{};

    rc = sqlite3_exec(database, getModeType.c_str(), selectCallbackUsingColNameAsKey, &modeSymbolRecord,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    std::string  modeType{modeSymbolRecord.at(0)["id"]};

    /*
     *Verify the mode field is an enumeration
     */
    std::string getModeFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getModeFieldTypes += modeType;
    getRadiusFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> modeFieldTypesRecords{};

    rc = sqlite3_exec(database, getModeFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &modeFieldTypesRecords,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(modeFieldTypesRecords.size() == 1);

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for(auto record: modeFieldTypesRecords)
    {
        REQUIRE(record.find("id") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_size") != record.end());
        REQUIRE(record.find("elf") != record.end());
    }

    std::string getModeEnums{"SELECT * FROM enumerations WHERE symbol = "};
    getModeEnums += modeType;
    getModeEnums += ";";

    std::vector<std::map<std::string, std::string>> modeEnumsRecords{};

    rc = sqlite3_exec(database, getModeEnums.c_str(), selectCallbackUsingColNameAsKey, &modeEnumsRecords,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(modeType == "31");

    REQUIRE(modeEnumsRecords.size() == 8);

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for(auto record: modeEnumsRecords)
    {
        REQUIRE(record.find("id") != record.end());
        REQUIRE(record.find("symbol") != record.end());
        REQUIRE(record.find("value") != record.end());
        REQUIRE(record.find("name") != record.end());
    }

    //Enforce order of records by value
    std::sort(modeEnumsRecords.begin(), modeEnumsRecords.end(),
              [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
              {
                return std::stoi(a["value"]) < std::stoi(b["value"]);
              });

    REQUIRE(modeEnumsRecords[0]["name"] == "MODE_SLOT_NONE");
    REQUIRE(modeEnumsRecords[0]["value"] == "-1");
    REQUIRE(modeEnumsRecords[1]["name"] == "MODE_SLOT_1");
    REQUIRE(modeEnumsRecords[1]["value"] == "0");
    REQUIRE(modeEnumsRecords[2]["name"] == "MODE_SLOT_2");
    REQUIRE(modeEnumsRecords[2]["value"] == "1");
    REQUIRE(modeEnumsRecords[3]["name"] == "MODE_SLOT_3");
    REQUIRE(modeEnumsRecords[3]["value"] == "2");
    REQUIRE(modeEnumsRecords[4]["name"] == "MODE_SLOT_4");
    REQUIRE(modeEnumsRecords[4]["value"] == "3");
    REQUIRE(modeEnumsRecords[5]["name"] == "MODE_SLOT_5");
    REQUIRE(modeEnumsRecords[5]["value"] == "4");
    REQUIRE(modeEnumsRecords[6]["name"] == "MODE_SLOT_6");
    REQUIRE(modeEnumsRecords[6]["value"] == "5");
    REQUIRE(modeEnumsRecords[7]["name"] == "MODE_SLOT_MAX");
    REQUIRE(modeEnumsRecords[7]["value"] == "6");

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
	 * gcc (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0
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

    std::vector<std::map<std::string, std::string>> circleRecords{};


    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingColNameAsKey, &circleRecords,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(circleRecords.size() == 1);
    /**
     * Check the correctness of Circle struct.
     */

    REQUIRE(circleRecords.at(0)["name"] == "Circle");
    REQUIRE(circleRecords.at(0)["byte_size"] == std::to_string(sizeof(Circle)));

    /**
     *Check the fields of the Circle struct.
    */

    std::string circle_id = circleRecords.at(0)["id"];

    std::string ciircle_artifact_id = circleRecords.at(0)["artifact"];

    REQUIRE(!ciircle_artifact_id.empty());

    std::string getCircleArtifact{"SELECT * FROM artifacts WHERE id = "};

    getCircleArtifact += ciircle_artifact_id;
    getCircleArtifact += ";";


    std::vector<std::map<std::string, std::string>> circleArtifactRecords{};

    rc = sqlite3_exec(database, getCircleArtifact.c_str(), selectCallbackUsingColNameAsKey, &circleArtifactRecords,
                             &errorMessage);

    REQUIRE(circleArtifactRecords.size() == 1);

    std::string path{};
    char resolvedPath[PATH_MAX];

    realpath("../unit-test/test_file1.h", resolvedPath);

    path.clear();
    path.insert(0, resolvedPath);

    REQUIRE(circleArtifactRecords.at(0)["path"] == path);

	std::string expectedMD5Str = getmd5sumFromSystem(resolvedPath);
    REQUIRE(expectedMD5Str == circleArtifactRecords.at(0)["md5"]);


    REQUIRE(!circleArtifactRecords.at(0)["elf"].empty());


    std::string getCircleElf{"SELECT * FROM elfs WHERE id = "};

    getCircleElf += circleArtifactRecords.at(0)["elf"];
    getCircleElf += ";";


    std::vector<std::map<std::string, std::string>> circleElftRecords{};

    rc = sqlite3_exec(database, getCircleElf.c_str(), selectCallbackUsingColNameAsKey, &circleElftRecords,
                             &errorMessage);

    REQUIRE(circleElftRecords.size() == 1);

    uint32_t numberOfColumns = 0;

    for(auto pair: circleElftRecords.at(0))
    {
    	numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 5);


    memset(&resolvedPath, '\0', PATH_MAX);

    realpath("./ut_obj/test_file1.o", resolvedPath);

    path.clear();
    path.insert(0, resolvedPath);

    REQUIRE(circleElftRecords.at(0)["name"] == path);

    expectedMD5Str.clear();

	std::string expectedMD5Str2 = getmd5sumFromSystem(resolvedPath);
	REQUIRE(expectedMD5Str2 == circleElftRecords.at(0)["md5"]);

    std::string getCircleFields{"SELECT * FROM fields WHERE symbol = "};

    getCircleFields += circle_id;
    getCircleFields += ";";


    std::vector<std::map<std::string, std::string>> circleFieldsRecords{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingColNameAsKey, &circleFieldsRecords,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(circleFieldsRecords.size() == 4);

    //Enforce order of records by offset
    std::sort(circleFieldsRecords.begin(), circleFieldsRecords.end(),
    		  [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
			  {
    			return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]);
			  });

    /**
     *Check the correctness of the fields
     */

    std::string getDiameterType{"SELECT * FROM symbols where id="};

    getDiameterType += circleFieldsRecords.at(0)["type"];
    getDiameterType += ";";

    std::vector<std::map<std::string, std::string>> diameterTypeRecords{};

    rc = sqlite3_exec(database, getDiameterType.c_str(), selectCallbackUsingColNameAsKey, &diameterTypeRecords,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(diameterTypeRecords.size() == 1);

    std::string  diameterType{diameterTypeRecords.at(0)["id"]};

    REQUIRE(circleFieldsRecords.at(0)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(circleFieldsRecords.at(0)["name"] == "diameter");
    REQUIRE(circleFieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(circleFieldsRecords.at(0)["type"] == diameterType);
    REQUIRE(circleFieldsRecords.at(0)["little_endian"] == little_endian);

    std::string getRadiusType{"SELECT * FROM symbols where id="};

    getRadiusType += circleFieldsRecords.at(1)["type"];
    getRadiusType += ";";

    std::vector<std::map<std::string, std::string>> radiusTypeRecords{};

    rc = sqlite3_exec(database, getRadiusType.c_str(), selectCallbackUsingColNameAsKey, &radiusTypeRecords,
                               &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(radiusTypeRecords.size() == 1);
    REQUIRE(radiusTypeRecords.at(0).find("id") != radiusTypeRecords.at(0).end());

    std::string  radiusType{radiusTypeRecords.at(0)["id"]};

    REQUIRE(circleFieldsRecords.at(1)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(circleFieldsRecords.at(1)["name"] == "radius");
    REQUIRE(circleFieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(Circle, radius)));
    REQUIRE(circleFieldsRecords.at(1)["type"] == radiusType);
    REQUIRE(circleFieldsRecords.at(1)["little_endian"] == little_endian);

    /**
     *Check the correctness of the types
     */
    std::string getDiameterFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getDiameterFieldTypes += diameterType;
    getDiameterFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> diameterFieldTypeRecords{};

    rc = sqlite3_exec(database, getDiameterFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &diameterFieldTypeRecords,
                             &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(diameterFieldTypeRecords.size() == 1);

    REQUIRE(diameterFieldTypeRecords.at(0)["name"] == "float");
    REQUIRE(diameterFieldTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(float)));

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
	 * gcc (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0
	 *  little-endian machine.
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

    std::vector<std::map<std::string, std::string>> squareRecords{};

    rc = sqlite3_exec(database, getSquareStructQuery.c_str(), selectCallbackUsingColNameAsKey, &squareRecords,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(squareRecords.size() ==  1);

    uint32_t numberOfColumns = 0;

    for(auto pair: squareRecords.at(0))
    {
    	numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 7);

    /**
     * Check the correctness of Square struct.
     */

    REQUIRE(squareRecords.at(0)["name"] == "Square");
    REQUIRE(squareRecords.at(0)["byte_size"] == std::to_string(sizeof(Square)));


    std::string square_id = squareRecords.at(0)["id"];

    std::string square_artifact_id = squareRecords.at(0)["artifact"];

    REQUIRE(!square_artifact_id.empty());

    std::string getSquareArtifact{"SELECT * FROM artifacts WHERE id = "};

    getSquareArtifact += square_artifact_id;
    getSquareArtifact += ";";

    std::vector<std::map<std::string, std::string>> squareArtifactRecords{};

    rc = sqlite3_exec(database, getSquareArtifact.c_str(), selectCallbackUsingColNameAsKey, &squareArtifactRecords,
                             &errorMessage);

    REQUIRE(squareArtifactRecords.size() == 1);

    std::string path{};
    char resolvedPath[PATH_MAX];

    realpath("../unit-test/test_file1.h", resolvedPath);

    path.clear();
    path.insert(0, resolvedPath);

    REQUIRE(squareArtifactRecords.at(0)["path"] == path);

	std::string expectedMD5Str = getmd5sumFromSystem(resolvedPath);
    REQUIRE(expectedMD5Str == squareArtifactRecords.at(0)["md5"]);

    std::string getSquareFields{"SELECT * FROM fields WHERE symbol = "};

    getSquareFields += square_id;
    getSquareFields += ";";

    std::vector<std::map<std::string, std::string>> squareFieldsRecords{};

    rc = sqlite3_exec(database, getSquareFields.c_str(), selectCallbackUsingColNameAsKey, &squareFieldsRecords,
                             &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(squareFieldsRecords.size() == 9);

    //Enforce order of records by offset
    std::sort(squareFieldsRecords.begin(), squareFieldsRecords.end(),
    		  [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
			  {
    			return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]);
			  });

    std::string getWidthType{"SELECT * FROM symbols where id="};
    getWidthType += squareFieldsRecords.at(0)["type"];
    getWidthType += ";";

    std::vector<std::map<std::string, std::string>> widthTypeRecords{};

    rc = sqlite3_exec(database, getWidthType.c_str(), selectCallbackUsingColNameAsKey, &widthTypeRecords,
                               &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(widthTypeRecords.size() == 1);

    std::string  widthType{widthTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(0)["symbol"] == squareRecords.at(0)["id"]);
    REQUIRE(squareFieldsRecords.at(0)["name"] == "width");
    REQUIRE(squareFieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(Square, width)));
    REQUIRE(squareFieldsRecords.at(0)["type"] == widthType);
    REQUIRE(squareFieldsRecords.at(0)["little_endian"] == little_endian);


    std::string getStuffType{"SELECT * FROM symbols where id="};
    getStuffType += squareFieldsRecords.at(1)["type"];
    getStuffType += ";";

    std::vector<std::map<std::string, std::string>> stuffTypeRecords{};

    rc = sqlite3_exec(database, getStuffType.c_str(), selectCallbackUsingColNameAsKey, &stuffTypeRecords,
                                &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(stuffTypeRecords.size() == 1);

    std::string  stuffType{stuffTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(1)["name"] == "stuff");
    REQUIRE(squareFieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(Square, stuff)));
	REQUIRE(squareFieldsRecords.at(1)["type"] == stuffType);
	REQUIRE(squareFieldsRecords.at(1)["little_endian"] == little_endian);


	std::string getPadding1Type{"SELECT * FROM symbols where id="};
	getPadding1Type += squareFieldsRecords.at(2)["type"];
	getPadding1Type += ";";

	std::vector<std::map<std::string, std::string>> padding1TypeRecords{};

	rc = sqlite3_exec(database, getPadding1Type.c_str(), selectCallbackUsingColNameAsKey, &padding1TypeRecords,
							 &errorMessage);
	REQUIRE(rc == SQLITE_OK);
	REQUIRE(padding1TypeRecords.size() == 1);

	std::string  padding1Type{padding1TypeRecords.at(0)["id"]};

	REQUIRE(squareFieldsRecords.at(2)["name"] == "padding1");
	REQUIRE(squareFieldsRecords.at(2)["byte_offset"] == std::to_string(offsetof(Square, padding1)));
	REQUIRE(squareFieldsRecords.at(2)["type"] == padding1Type);
	REQUIRE(squareFieldsRecords.at(2)["little_endian"] == little_endian);

	std::string getLengthType{"SELECT * FROM symbols where id="};
	getLengthType += squareFieldsRecords.at(3)["type"];
	getLengthType += ";";

	std::vector<std::map<std::string, std::string>> lengthTypeRecords{};

	rc = sqlite3_exec(database, getLengthType.c_str(), selectCallbackUsingColNameAsKey, &lengthTypeRecords,
							  &errorMessage);
	REQUIRE(rc == SQLITE_OK);
	REQUIRE(lengthTypeRecords.size() == 1);

	std::string  lengthType{lengthTypeRecords.at(0)["id"]};

	REQUIRE(squareFieldsRecords.at(3)["name"] == "length");
	REQUIRE(squareFieldsRecords.at(3)["byte_offset"] == std::to_string(offsetof(Square, length)));
	REQUIRE(squareFieldsRecords.at(3)["type"] == lengthType);
	REQUIRE(squareFieldsRecords.at(3)["little_endian"] == little_endian);

	std::string getMoreStuffType{"SELECT * FROM symbols where id="};
	getMoreStuffType += squareFieldsRecords.at(4)["type"];
	getMoreStuffType += ";";

	std::vector<std::map<std::string, std::string>> moreStuffTypeRecords{};

	rc = sqlite3_exec(database, getMoreStuffType.c_str(), selectCallbackUsingColNameAsKey, &moreStuffTypeRecords,
							  &errorMessage);
	REQUIRE(rc == SQLITE_OK);
	REQUIRE(moreStuffTypeRecords.size() == 1);

	std::string  moreStuffType{moreStuffTypeRecords.at(0)["id"]};

	REQUIRE(squareFieldsRecords.at(4)["name"] == "more_stuff");
	REQUIRE(squareFieldsRecords.at(4)["byte_offset"] == std::to_string(offsetof(Square, more_stuff)));
	REQUIRE(squareFieldsRecords.at(4)["type"] == moreStuffType);
	REQUIRE(squareFieldsRecords.at(4)["little_endian"] == little_endian);


	std::string getPadding2Type{"SELECT * FROM symbols where id="};
	getPadding2Type += squareFieldsRecords.at(5)["type"];
	getPadding2Type += ";";

	std::vector<std::map<std::string, std::string>> padding2TypeRecords{};

	rc = sqlite3_exec(database, getPadding2Type.c_str(), selectCallbackUsingColNameAsKey, &padding2TypeRecords,
							  &errorMessage);
	REQUIRE(rc == SQLITE_OK);
	REQUIRE(padding2TypeRecords.size() == 1);

	std::string  padding2Type{padding2TypeRecords.at(0)["id"]};

	REQUIRE(squareFieldsRecords.at(5)["name"] == "padding2");
	REQUIRE(squareFieldsRecords.at(5)["byte_offset"] == std::to_string(offsetof(Square, padding2)));
	REQUIRE(squareFieldsRecords.at(5)["type"] == padding2Type);
	REQUIRE(squareFieldsRecords.at(5)["little_endian"] == little_endian);

	std::string getFloatingStuffType{"SELECT * FROM symbols where id="};
	getFloatingStuffType += squareFieldsRecords.at(6)["type"];
	getFloatingStuffType += ";";

	std::vector<std::map<std::string, std::string>> floatingStuffTypeRecords{};

	rc = sqlite3_exec(database, getFloatingStuffType.c_str(), selectCallbackUsingColNameAsKey, &floatingStuffTypeRecords,
							  &errorMessage);
	REQUIRE(rc == SQLITE_OK);
	REQUIRE(floatingStuffTypeRecords.size() == 1);

	std::string  floatingStuffType{floatingStuffTypeRecords.at(0)["id"]};

	REQUIRE(squareFieldsRecords.at(6)["name"] == "floating_stuff");
	REQUIRE(squareFieldsRecords.at(6)["byte_offset"] == std::to_string(offsetof(Square, floating_stuff)));
	REQUIRE(squareFieldsRecords.at(6)["type"] == floatingStuffType);
	REQUIRE(squareFieldsRecords.at(6)["little_endian"] == little_endian);

	//Test matrix3D[2][4][4]
	std::string getMatrix3DDimensionLists{"SELECT * FROM dimension_lists WHERE field_id="};
	getMatrix3DDimensionLists += squareFieldsRecords.at(7)["id"];
	getMatrix3DDimensionLists += ";";

	std::vector<std::map<std::string, std::string>> matrix3DDimensionListsRecords{};

	rc = sqlite3_exec(database, getMatrix3DDimensionLists.c_str(), selectCallbackUsingColNameAsKey, &matrix3DDimensionListsRecords,
						 &errorMessage);
	REQUIRE(rc == SQLITE_OK);
	REQUIRE(matrix3DDimensionListsRecords.size() == 3);

	//Enforce order of records by dim_order
	std::sort(matrix3DDimensionListsRecords.begin(), matrix3DDimensionListsRecords.end(),
		  [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
		  {
			return std::stoi(a["dim_order"]) < std::stoi(b["dim_order"]);
		  });

	REQUIRE(matrix3DDimensionListsRecords.at(0)["field_id"] == squareFieldsRecords.at(7)["id"]);
	REQUIRE(matrix3DDimensionListsRecords.at(0)["dim_order"] == "0");
	REQUIRE(matrix3DDimensionListsRecords.at(0)["upper_bound"] == "1");

	REQUIRE(matrix3DDimensionListsRecords.at(1)["field_id"] == squareFieldsRecords.at(7)["id"]);
	REQUIRE(matrix3DDimensionListsRecords.at(1)["dim_order"] == "1");
	REQUIRE(matrix3DDimensionListsRecords.at(1)["upper_bound"] == "3");

	REQUIRE(matrix3DDimensionListsRecords.at(2)["field_id"] == squareFieldsRecords.at(7)["id"]);
	REQUIRE(matrix3DDimensionListsRecords.at(2)["dim_order"] == "2");
	REQUIRE(matrix3DDimensionListsRecords.at(2)["upper_bound"] == "3");


	//Test matrix3D[2][4][4]
	std::string getMatrix1DDimensionLists{"SELECT * FROM dimension_lists WHERE field_id="};
	getMatrix1DDimensionLists += squareFieldsRecords.at(8)["id"];
	getMatrix1DDimensionLists += ";";

	std::vector<std::map<std::string, std::string>> matrix1DDimensionListsRecords{};

	rc = sqlite3_exec(database, getMatrix1DDimensionLists.c_str(), selectCallbackUsingColNameAsKey, &matrix1DDimensionListsRecords,
						 &errorMessage);
	REQUIRE(rc == SQLITE_OK);
	REQUIRE(matrix1DDimensionListsRecords.size() == 1);

	//Enforce order of records by dim_order
	std::sort(matrix1DDimensionListsRecords.begin(), matrix1DDimensionListsRecords.end(),
		  [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
		  {
			return std::stoi(a["dim_order"]) < std::stoi(b["dim_order"]);
		  });

	REQUIRE(matrix1DDimensionListsRecords.at(0)["field_id"] == squareFieldsRecords.at(8)["id"]);
	REQUIRE(matrix1DDimensionListsRecords.at(0)["dim_order"] == "0");
	REQUIRE(matrix1DDimensionListsRecords.at(0)["upper_bound"] == "1");

    REQUIRE(remove("./test_db.sqlite")==0);
    delete idc;
}

TEST_CASE("Write keys to database that already exist" ,"[main_test#5]")
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
TEST_CASE("Write Elf File to database with a log file", "[main_test#6]")
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

TEST_CASE("Write Elf File to database with verbosity set to INFO", "[main_test#7]")
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

TEST_CASE("Write Elf File to database with invalid verbosity", "[main_test#8]")
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

