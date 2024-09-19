/*
 * main_test.cpp
 *
 *  Created on: Aug 20, 2020
 *      Author: vagrant
 */
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include <cstdlib>
#include <map>
#include <string>
#include <strstream>

#include "IDataContainer.h"
#include "Juicer.h"
#include "SQLiteDB.h"
#include "catch.hpp"
#include "test_file1.h"

/**
 *These test file locations assumes that the tests are run
 * with "make run-tests".
 */
#define TEST_FILE_1   "ut_obj/test_file1.o"
#define TEST_FILE_2   "ut_obj/test_file2.o"
#define TEST_FILE_3   "ut_obj_32/test_file1.o"

#define TEST_FILE_4   "ut_obj/macro_test.o"

// DO NOT rename this macro to something like SQLITE_NULL as that is a macro that exists in sqlite3
#define TEST_NULL_STR "NULL"

/**
 *Checks if the platform is little endian.
 *This is used as a source of truth for our unit tests.
 */
static bool is_little_endian()
{
    int n = 1;
    return (*(char*)&n == 1);
}

/**
 *@brief This callback returns the record inside of a std::vector<std::string>
 *
 */
static int selectVectorCallback(void* veryUsed, int argc, char** argv, char** azColName)
{
    int                      i;
    auto*                    tableData = (std::vector<std::vector<std::string>>*)veryUsed;

    std::vector<std::string> recordVector{};

    for (i = 0; i < argc; i++)
    {
        std::string tempData{TEST_NULL_STR};
        if (argv[i] != nullptr)
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
    std::string                                     colName{};
    std::map<std::string, std::vector<std::string>> recordMap{};
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
static int selectCallbackUsingCustomColNameAsKey(void* veryUsed, int argc, char** argv, char** azColName)
{
    columnNameToRowMap*      mappingData = (columnNameToRowMap*)veryUsed;
    auto*                    row         = (std::map<std::string, std::vector<std::string>>*)(&mappingData->recordMap);
    int                      key_index   = 0;
    std::vector<std::string> tableData{};

    for (int i = 0; i < argc; i++)
    {
        std::string tempData{TEST_NULL_STR};
        if (argv[i] != nullptr)
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
static int selectCallbackUsingColNameAsKey(void* veryUsed, int argc, char** argv, char** azColName)
{
    auto*                              allRecords = (std::vector<std::map<std::string, std::string>>*)veryUsed;

    std::map<std::string, std::string> newRecord{};

    std::vector<std::string>           tableData{};

    for (int i = 0; i < argc; i++)
    {
        std::string tempData{TEST_NULL_STR};
        if (argv[i] != nullptr)
        {
            tempData.assign(argv[i]);
        }

        newRecord[azColName[i]] = tempData;
    }

    allRecords->push_back(newRecord);

    return 0;
}

static std::string getmd5sumFromSystem(char resolvedPath[PATH_MAX])
{
    //	TODO:Unfortunately the redirect is adding junk(a "\n" character at the end) at the end of the crc.
    std::string MD5CommandStr{"md5sum "};
    MD5CommandStr += resolvedPath;
    MD5CommandStr += " >MD5.txt";
    std::system(MD5CommandStr.c_str());  // executes the UNIX command "md5sum resolvedPath[PATH_MAX] >MD5.txt"
    std::strstream expectedMD5{};
    expectedMD5 << std::ifstream("MD5.txt").rdbuf();
    REQUIRE(remove("./MD5.txt") == 0);
    std::string expectedMD5Str{expectedMD5.str()};

    //	Size should be size of hash(16 bytes)
    expectedMD5Str = expectedMD5Str.substr(0, 32);
    REQUIRE(expectedMD5Str.size() == 32);
    return expectedMD5Str;
}

static std::map<std::string, std::string> followTargetSymbol(sqlite3* database, std::string symbolID)
{
    std::string symbolQuery{"SELECT * FROM symbols where id="};
    int         rc  = 0;

    symbolQuery    += symbolID;
    symbolQuery    += ";";

    std::map<std::string, std::string>              targetSymbolRecord{};

    std::vector<std::map<std::string, std::string>> symbolRecords{};

    char*                                           errorMessage = nullptr;

    rc = sqlite3_exec(database, symbolQuery.c_str(), selectCallbackUsingColNameAsKey, &symbolRecords, &errorMessage);

    REQUIRE(rc == JUICER_OK);

    if (symbolRecords.at(0).at("target_symbol") != "NULL")
    {
        targetSymbolRecord = followTargetSymbol(database, symbolRecords.at(0).at("target_symbol"));
    }
    else
    {
        return symbolRecords.at(0);
    }
    return targetSymbolRecord;
}

TEST_CASE("Test Juicer at the highest level with SQLiteDB", "[main_test#1]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");

    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    REQUIRE(juicer.parse(inputFile) == JUICER_OK);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}

TEST_CASE("Test the correctness of the Circle struct after Juicer has processed it.", "[main_test#2]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    sqlite3* database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::string                                     getAllEncodings{"SELECT * FROM encodings"};

    std::vector<std::map<std::string, std::string>> encodingsRecords{};

    rc = sqlite3_exec(database, getAllEncodings.c_str(), selectCallbackUsingColNameAsKey, &encodingsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(encodingsRecords.size() == 18);

    /**
     * @brief encodingMap A map of row_id -> encoding.
     * i.e {"1":"DW_ATE_address", "2":"DW_ATE_boolean", etc}. Useful for avoiding having to look up the encoding by foreign of a symbol
     * every time.
     */
    std::map<std::string, std::string> encodingMap{};

    for (auto encodingRecord : encodingsRecords)
    {
        encodingMap[encodingRecord["id"]] = encodingRecord["encoding"];
    }

    std::string                                     getCircleStructQuery{"SELECT * FROM symbols WHERE name = \"Circle\"; "};

    std::vector<std::map<std::string, std::string>> circleRecords{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingColNameAsKey, &circleRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(circleRecords.size() == 1);
    /**
     * Check the correctness of Circle struct.
     */

    REQUIRE(circleRecords.at(0).find("name") != circleRecords.at(0).end());
    REQUIRE(circleRecords.at(0).find("byte_size") != circleRecords.at(0).end());
    REQUIRE(circleRecords.at(0).find("id") != circleRecords.at(0).end());
    REQUIRE(circleRecords.at(0).find("target_symbol") != circleRecords.at(0).end());
    REQUIRE(circleRecords.at(0).find("encoding") != circleRecords.at(0).end());
    REQUIRE(circleRecords.at(0).find("short_description") != circleRecords.at(0).end());
    REQUIRE(circleRecords.at(0).find("long_description") != circleRecords.at(0).end());

    REQUIRE(circleRecords.at(0)["name"] == "Circle");
    REQUIRE(circleRecords.at(0)["byte_size"] == std::to_string(sizeof(Circle)));
    REQUIRE(circleRecords.at(0)["target_symbol"] == "NULL");
    REQUIRE(circleRecords.at(0)["encoding"] == "NULL");
    REQUIRE(circleRecords.at(0)["short_description"] == "");
    REQUIRE(circleRecords.at(0)["long_description"] == "");

    /**
     *Check the fields of the Circle struct.
     */

    std::string circle_id = circleRecords.at(0)["id"];

    std::string getCircleFields{"SELECT * FROM fields WHERE symbol = "};

    getCircleFields += circle_id;
    getCircleFields += ";";

    std::vector<std::map<std::string, std::string>> fieldsRecords{};

    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingColNameAsKey, &fieldsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(fieldsRecords.size() == 5);

    // Enforce order of records by offset
    std::sort(fieldsRecords.begin(), fieldsRecords.end(), [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
              { return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]); });

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for (auto record : fieldsRecords)
    {
        REQUIRE(record.find("symbol") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_offset") != record.end());
        REQUIRE(record.find("type") != record.end());

        REQUIRE(record.find("little_endian") != record.end());
        REQUIRE(record.find("bit_size") != record.end());
        REQUIRE(record.find("bit_offset") != record.end());
        REQUIRE(record.find("short_description") != record.end());
        REQUIRE(record.find("long_description") != record.end());
    }

    REQUIRE(fieldsRecords.at(0)["name"] == "diameter");
    /**
     *Check the correctness of the fields
     */

    std::string getDiameterType{"SELECT * FROM symbols where id="};

    getDiameterType += fieldsRecords.at(0)["type"];
    getDiameterType += ";";

    std::vector<std::map<std::string, std::string>> dimaterSymbolRecords{};

    rc = sqlite3_exec(database, getDiameterType.c_str(), selectCallbackUsingColNameAsKey, &dimaterSymbolRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(dimaterSymbolRecords.size() == 1);

    std::string diameterType{dimaterSymbolRecords.at(0).at("id")};

    REQUIRE(fieldsRecords.at(0)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(fieldsRecords.at(0)["name"] == "diameter");
    REQUIRE(fieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(fieldsRecords.at(0)["type"] == diameterType);
    REQUIRE(fieldsRecords.at(0)["little_endian"] == little_endian);
    REQUIRE(fieldsRecords.at(0)["bit_size"] == "0");
    REQUIRE(fieldsRecords.at(0)["bit_offset"] == "0");
    REQUIRE(fieldsRecords.at(0)["short_description"] == "");
    REQUIRE(fieldsRecords.at(0)["long_description"] == "");

    REQUIRE(fieldsRecords.at(1)["name"] == "radius");

    std::string getRadiusType{"SELECT * FROM symbols where id="};

    getRadiusType += fieldsRecords.at(1)["type"];
    getRadiusType += ";";

    std::vector<std::map<std::string, std::string>> radiusSymbolRecord{};

    rc = sqlite3_exec(database, getRadiusType.c_str(), selectCallbackUsingColNameAsKey, &radiusSymbolRecord, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    std::string radiusType{radiusSymbolRecord.at(0)["id"]};

    REQUIRE(fieldsRecords.at(1)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(fieldsRecords.at(1)["name"] == "radius");
    REQUIRE(fieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(Circle, radius)));
    REQUIRE(fieldsRecords.at(1)["type"] == radiusType);
    REQUIRE(fieldsRecords.at(1)["little_endian"] == little_endian);
    REQUIRE(fieldsRecords.at(1)["bit_size"] == "0");
    REQUIRE(fieldsRecords.at(1)["bit_offset"] == "0");
    REQUIRE(fieldsRecords.at(1)["short_description"] == "");
    REQUIRE(fieldsRecords.at(1)["long_description"] == "");

    std::string getPointsType{"SELECT * FROM symbols where id="};

    getPointsType += fieldsRecords.at(2)["type"];
    getPointsType += ";";

    std::vector<std::map<std::string, std::string>> pointsSymbolRecord{};

    rc = sqlite3_exec(database, getPointsType.c_str(), selectCallbackUsingColNameAsKey, &pointsSymbolRecord, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    std::string PointsType{pointsSymbolRecord.at(0)["id"]};

    REQUIRE(fieldsRecords.at(2)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(fieldsRecords.at(2)["name"] == "points");
    REQUIRE(fieldsRecords.at(2)["byte_offset"] == std::to_string(offsetof(Circle, points)));
    REQUIRE(fieldsRecords.at(2)["type"] == PointsType);
    REQUIRE(fieldsRecords.at(2)["little_endian"] == little_endian);
    REQUIRE(fieldsRecords.at(2)["bit_size"] == "0");
    REQUIRE(fieldsRecords.at(2)["bit_offset"] == "0");
    REQUIRE(fieldsRecords.at(2)["short_description"] == "");
    REQUIRE(fieldsRecords.at(2)["long_description"] == "");

    /**
     *Check the correctness of the types
     */
    std::string getDiameterFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getDiameterFieldTypes += diameterType;
    getDiameterFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> diameterFieldSymbolRecord{};

    rc = sqlite3_exec(database, getDiameterFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &diameterFieldSymbolRecord, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(diameterFieldSymbolRecord.size() == 1);

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for (auto record : diameterFieldSymbolRecord)
    {
        REQUIRE(record.find("id") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_size") != record.end());
        REQUIRE(record.find("elf") != record.end());
        REQUIRE(record.find("target_symbol") != record.end());
        REQUIRE(record.find("encoding") != record.end());
        REQUIRE(record.find("short_description") != record.end());
        REQUIRE(record.find("long_description") != record.end());
    }

    REQUIRE(diameterFieldSymbolRecord.at(0)["name"] == "float");
    REQUIRE(diameterFieldSymbolRecord.at(0)["byte_size"] == std::to_string(sizeof(float)));
    REQUIRE(diameterFieldSymbolRecord.at(0)["target_symbol"] == "NULL");
    REQUIRE(encodingMap.at(diameterFieldSymbolRecord.at(0).at("encoding")) == "DW_ATE_float");
    REQUIRE(diameterFieldSymbolRecord.at(0)["short_description"] == "");
    REQUIRE(diameterFieldSymbolRecord.at(0)["long_description"] == "");

    std::string getRadiusFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getRadiusFieldTypes += radiusType;
    getRadiusFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> radiusFieldTypesRecords{};

    rc = sqlite3_exec(database, getRadiusFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &radiusFieldTypesRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(radiusFieldTypesRecords.size() == 1);
    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for (auto record : radiusFieldTypesRecords)
    {
        REQUIRE(record.find("id") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_size") != record.end());
        REQUIRE(record.find("elf") != record.end());
        REQUIRE(record.find("target_symbol") != record.end());
        REQUIRE(record.find("encoding") != record.end());
        REQUIRE(record.find("short_description") != record.end());
        REQUIRE(record.find("long_description") != record.end());
    }

    REQUIRE(radiusFieldTypesRecords.at(0)["name"] == "float");
    REQUIRE(radiusFieldTypesRecords.at(0)["byte_size"] == std::to_string(sizeof(float)));
    REQUIRE(radiusFieldTypesRecords.at(0)["target_symbol"] == "NULL");
    REQUIRE(encodingMap.at(radiusFieldTypesRecords.at(0).at("encoding")) == "DW_ATE_float");
    REQUIRE(radiusFieldTypesRecords.at(0)["short_description"] == "");
    REQUIRE(radiusFieldTypesRecords.at(0)["long_description"] == "");

    std::string getPointsFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getPointsFieldTypes += PointsType;
    getPointsFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> pointsFieldTypesRecords{};

    rc = sqlite3_exec(database, getPointsFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &pointsFieldTypesRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(pointsFieldTypesRecords.size() == 1);

    REQUIRE(pointsFieldTypesRecords.at(0)["name"] == "int");
    REQUIRE(pointsFieldTypesRecords.at(0)["byte_size"] == std::to_string(sizeof(int)));

    REQUIRE(pointsFieldTypesRecords.at(0)["target_symbol"] == "NULL");
    REQUIRE(encodingMap.at(pointsFieldTypesRecords.at(0).at("encoding")) == "DW_ATE_signed");
    REQUIRE(pointsFieldTypesRecords.at(0)["short_description"] == "");
    REQUIRE(pointsFieldTypesRecords.at(0)["long_description"] == "");

    REQUIRE(fieldsRecords.at(3)["name"] == "mode");

    std::string getModeType{"SELECT * FROM symbols where id="};

    getModeType += fieldsRecords.at(3)["type"];
    getModeType += ";";

    std::vector<std::map<std::string, std::string>> modeSymbolRecord{};

    rc = sqlite3_exec(database, getModeType.c_str(), selectCallbackUsingColNameAsKey, &modeSymbolRecord, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    std::string modeType{modeSymbolRecord.at(0)["id"]};

    /*
     *Verify the mode field is an enumeration
     */
    std::string getModeFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getModeFieldTypes   += modeType;
    getRadiusFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> modeFieldTypesRecords{};

    rc = sqlite3_exec(database, getModeFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &modeFieldTypesRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(modeFieldTypesRecords.size() == 1);

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for (auto record : modeFieldTypesRecords)
    {
        REQUIRE(record.find("id") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_size") != record.end());
        REQUIRE(record.find("elf") != record.end());
        REQUIRE(record.find("target_symbol") != record.end());
        REQUIRE(record.find("encoding") != record.end());
        REQUIRE(record.find("short_description") != record.end());
        REQUIRE(record.find("long_description") != record.end());
    }

    std::string getModeEnums{"SELECT * FROM enumerations WHERE symbol = "};
    getModeEnums += modeType;
    getModeEnums += ";";

    std::vector<std::map<std::string, std::string>> modeEnumsRecords{};

    rc = sqlite3_exec(database, getModeEnums.c_str(), selectCallbackUsingColNameAsKey, &modeEnumsRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    // REQUIRE(modeType == "40");

    REQUIRE(modeEnumsRecords.size() == 8);

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for (auto record : modeEnumsRecords)
    {
        REQUIRE(record.find("id") != record.end());
        REQUIRE(record.find("symbol") != record.end());
        REQUIRE(record.find("value") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("short_description") != record.end());
        REQUIRE(record.find("long_description") != record.end());
    }

    // Enforce order of records by value
    std::sort(modeEnumsRecords.begin(), modeEnumsRecords.end(),
              [](std::map<std::string, std::string> a, std::map<std::string, std::string> b) { return std::stoi(a["value"]) < std::stoi(b["value"]); });

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




    REQUIRE(fieldsRecords.at(4)["name"] == "_spare_end");
    /**
     *Check the correctness of the fields
     */

    std::string getSpareEndType{"SELECT * FROM symbols where id="};

    getSpareEndType += fieldsRecords.at(4)["type"];
    getSpareEndType += ";";

    std::vector<std::map<std::string, std::string>> spareEndSymbolRecords{};

    rc = sqlite3_exec(database, getSpareEndType.c_str(), selectCallbackUsingColNameAsKey, &spareEndSymbolRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(spareEndSymbolRecords.size() == 1);

    std::string spareEndType{spareEndSymbolRecords.at(0).at("id")};

    // TODO:Add support for unions first before adding these tests.

    // REQUIRE(fieldsRecords.at(4)["symbol"] == circleRecords.at(0)["id"]);
    // REQUIRE(fieldsRecords.at(4)["name"] == "_spare_end");
    // REQUIRE(fieldsRecords.at(4)["byte_offset"] == std::to_string( sizeof(float) + sizeof(float) + (sizeof(int) * 128) + sizeof(ModeSlot_t) ));
    // REQUIRE(fieldsRecords.at(4)["type"] == spareEndType);
    // REQUIRE(fieldsRecords.at(4)["little_endian"] == little_endian);
    // REQUIRE(fieldsRecords.at(4)["bit_size"] == "0");
    // REQUIRE(fieldsRecords.at(4)["bit_offset"] == "0");
    // REQUIRE(fieldsRecords.at(4)["short_description"] == "");
    // REQUIRE(fieldsRecords.at(4)["long_description"] == "");



    /**
     * *Clean up our database handle and objects in memory.
     */
    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}

TEST_CASE(
    "Test the correctness of the Circle struct after Juicer has processed it on two"
    " different elf files.",
    "[main_test#3]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);
    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    inputFile = TEST_FILE_2;

    rc        = juicer.parse(inputFile);
    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    std::string getCircleStructQuery{"SELECT * FROM symbols WHERE name = \"Circle\"; "};

    sqlite3*    database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::string                                     getAllEncodings{"SELECT * FROM encodings"};

    std::vector<std::map<std::string, std::string>> encodingsRecords{};

    rc = sqlite3_exec(database, getAllEncodings.c_str(), selectCallbackUsingColNameAsKey, &encodingsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(encodingsRecords.size() == 18);

    /**
     * @brief encodingMap A map of row_id -> encoding.
     * i.e {"1":"DW_ATE_address", "2":"DW_ATE_boolean", etc}. Useful for avoiding having to look up the encoding by foreign of a symbol
     * every time.
     */
    std::map<std::string, std::string> encodingMap{};

    for (auto encodingRecord : encodingsRecords)
    {
        encodingMap[encodingRecord["id"]] = encodingRecord["encoding"];
    }


    std::vector<std::map<std::string, std::string>> circleRecords{};

    rc = sqlite3_exec(database, getCircleStructQuery.c_str(), selectCallbackUsingColNameAsKey, &circleRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(circleRecords.size() == 1);
    /**
     * Check the correctness of Circle struct.
     */

    REQUIRE(circleRecords.at(0)["name"] == "Circle");
    REQUIRE(circleRecords.at(0)["byte_size"] == std::to_string(sizeof(Circle)));

    REQUIRE(circleRecords.at(0)["target_symbol"] == "NULL");
    REQUIRE(circleRecords.at(0).at("encoding") == "NULL");
    REQUIRE(circleRecords.at(0)["short_description"] == "");
    REQUIRE(circleRecords.at(0)["long_description"] == "");

    /**
     *Check the fields of the Circle struct.
     */

    std::string circle_id           = circleRecords.at(0)["id"];

    std::string ciircle_artifact_id = circleRecords.at(0)["artifact"];

    REQUIRE(!ciircle_artifact_id.empty());

    std::string getCircleArtifact{"SELECT * FROM artifacts WHERE id = "};

    getCircleArtifact += ciircle_artifact_id;
    getCircleArtifact += ";";

    std::vector<std::map<std::string, std::string>> circleArtifactRecords{};

    rc = sqlite3_exec(database, getCircleArtifact.c_str(), selectCallbackUsingColNameAsKey, &circleArtifactRecords, &errorMessage);

    REQUIRE(circleArtifactRecords.size() == 1);

    std::string path{};
    char        resolvedPath[PATH_MAX];

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

    rc = sqlite3_exec(database, getCircleElf.c_str(), selectCallbackUsingColNameAsKey, &circleElftRecords, &errorMessage);

    REQUIRE(circleElftRecords.size() == 1);

    uint32_t numberOfColumns = 0;

    for (auto pair : circleElftRecords.at(0))
    {
        numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 5);

    memset(&resolvedPath, '\0', PATH_MAX);

    realpath(TEST_FILE_1, resolvedPath);

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

    rc = sqlite3_exec(database, getCircleFields.c_str(), selectCallbackUsingColNameAsKey, &circleFieldsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(circleFieldsRecords.size() == 5);

    // Enforce order of records by offset
    std::sort(circleFieldsRecords.begin(), circleFieldsRecords.end(), [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
              { return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]); });

    /**
     *Check the correctness of the fields
     */

    std::string getDiameterType{"SELECT * FROM symbols where id="};

    getDiameterType += circleFieldsRecords.at(0)["type"];
    getDiameterType += ";";

    std::vector<std::map<std::string, std::string>> diameterTypeRecords{};

    rc = sqlite3_exec(database, getDiameterType.c_str(), selectCallbackUsingColNameAsKey, &diameterTypeRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(diameterTypeRecords.size() == 1);

    std::string diameterType{diameterTypeRecords.at(0)["id"]};

    REQUIRE(circleFieldsRecords.at(0)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(circleFieldsRecords.at(0)["name"] == "diameter");
    REQUIRE(circleFieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(Circle, diameter)));
    REQUIRE(circleFieldsRecords.at(0)["type"] == diameterType);
    REQUIRE(circleFieldsRecords.at(0)["little_endian"] == little_endian);

    REQUIRE(circleFieldsRecords.at(0)["short_description"] == "");
    REQUIRE(circleFieldsRecords.at(0)["long_description"] == "");

    std::string getRadiusType{"SELECT * FROM symbols where id="};

    getRadiusType += circleFieldsRecords.at(1)["type"];
    getRadiusType += ";";

    std::vector<std::map<std::string, std::string>> radiusTypeRecords{};

    rc = sqlite3_exec(database, getRadiusType.c_str(), selectCallbackUsingColNameAsKey, &radiusTypeRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(radiusTypeRecords.size() == 1);
    REQUIRE(radiusTypeRecords.at(0).find("id") != radiusTypeRecords.at(0).end());

    std::string radiusType{radiusTypeRecords.at(0)["id"]};

    REQUIRE(circleFieldsRecords.at(1)["symbol"] == circleRecords.at(0)["id"]);
    REQUIRE(circleFieldsRecords.at(1)["name"] == "radius");
    REQUIRE(circleFieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(Circle, radius)));
    REQUIRE(circleFieldsRecords.at(1)["type"] == radiusType);
    REQUIRE(circleFieldsRecords.at(1)["little_endian"] == little_endian);

    REQUIRE(circleFieldsRecords.at(1)["short_description"] == "");
    REQUIRE(circleFieldsRecords.at(1)["long_description"] == "");

    /**
     *Check the correctness of the types
     */
    std::string getDiameterFieldTypes{"SELECT * FROM symbols WHERE id = "};
    getDiameterFieldTypes += diameterType;
    getDiameterFieldTypes += ";";

    std::vector<std::map<std::string, std::string>> diameterFieldTypeRecords{};

    rc = sqlite3_exec(database, getDiameterFieldTypes.c_str(), selectCallbackUsingColNameAsKey, &diameterFieldTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);

    REQUIRE(diameterFieldTypeRecords.size() == 1);

    REQUIRE(diameterFieldTypeRecords.at(0)["name"] == "float");
    REQUIRE(diameterFieldTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(float)));

    REQUIRE(diameterFieldTypeRecords.at(0)["target_symbol"] == "NULL");
    REQUIRE(encodingMap.at(diameterFieldTypeRecords.at(0).at("encoding")) == "DW_ATE_float");
    REQUIRE(diameterFieldTypeRecords.at(0)["short_description"] == "");
    REQUIRE(diameterFieldTypeRecords.at(0)["long_description"] == "");

    /**
     * *Clean up our database handle and objects in memory.
     */
    REQUIRE(remove("./test_db.sqlite") == 0);
    ((SQLiteDB*)(idc))->close();
    delete idc;
}

TEST_CASE("Test the correctness of the Square struct after Juicer has processed it.", "[main_test#4]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc            = 0;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    std::string getSquareStructQuery{"SELECT * FROM symbols WHERE name = \"Square\"; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3* database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::string                                     getAllEncodings{"SELECT * FROM encodings"};

    std::vector<std::map<std::string, std::string>> encodingsRecords{};

    rc = sqlite3_exec(database, getAllEncodings.c_str(), selectCallbackUsingColNameAsKey, &encodingsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(encodingsRecords.size() == 18);

    /**
     * @brief encodingMap A map of row_id -> encoding.
     * i.e {"1":"DW_ATE_address", "2":"DW_ATE_boolean", etc}. Useful for avoiding having to look up the encoding by foreign of a symbol
     * every time.
     */
    std::map<std::string, std::string> encodingMap{};

    for (auto encodingRecord : encodingsRecords)
    {
        encodingMap[encodingRecord["id"]] = encodingRecord["encoding"];
    }

    std::vector<std::map<std::string, std::string>> squareRecords{};

    rc = sqlite3_exec(database, getSquareStructQuery.c_str(), selectCallbackUsingColNameAsKey, &squareRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(squareRecords.size() == 1);

    uint32_t numberOfColumns = 0;

    for (auto pair : squareRecords.at(0))
    {
        numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 9);

    /**
     * Check the correctness of Square struct.
     */

    REQUIRE(squareRecords.at(0)["name"] == "Square");
    REQUIRE(squareRecords.at(0)["byte_size"] == std::to_string(sizeof(Square)));

    REQUIRE(squareRecords.at(0)["target_symbol"] == "NULL");
    REQUIRE(squareRecords.at(0).at("encoding") == "NULL");
    REQUIRE(squareRecords.at(0)["short_description"] == "");
    REQUIRE(squareRecords.at(0)["long_description"] == "");

    std::string square_id          = squareRecords.at(0)["id"];

    std::string square_artifact_id = squareRecords.at(0)["artifact"];

    REQUIRE(!square_artifact_id.empty());

    std::string getSquareArtifact{"SELECT * FROM artifacts WHERE id = "};

    getSquareArtifact += square_artifact_id;
    getSquareArtifact += ";";

    std::vector<std::map<std::string, std::string>> squareArtifactRecords{};

    rc = sqlite3_exec(database, getSquareArtifact.c_str(), selectCallbackUsingColNameAsKey, &squareArtifactRecords, &errorMessage);

    REQUIRE(squareArtifactRecords.size() == 1);

    std::string path{};
    char        resolvedPath[PATH_MAX];

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

    rc = sqlite3_exec(database, getSquareFields.c_str(), selectCallbackUsingColNameAsKey, &squareFieldsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(squareFieldsRecords.size() == 11);

    // Enforce order of records by offset
    std::sort(squareFieldsRecords.begin(), squareFieldsRecords.end(), [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
              { return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]); });

    std::string getWidthType{"SELECT * FROM symbols where id="};
    getWidthType += squareFieldsRecords.at(0)["type"];
    getWidthType += ";";

    std::vector<std::map<std::string, std::string>> widthTypeRecords{};

    rc = sqlite3_exec(database, getWidthType.c_str(), selectCallbackUsingColNameAsKey, &widthTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(widthTypeRecords.size() == 1);

    std::string widthType{widthTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(0)["symbol"] == squareRecords.at(0)["id"]);
    REQUIRE(squareFieldsRecords.at(0)["name"] == "width");
    REQUIRE(squareFieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(Square, width)));
    REQUIRE(squareFieldsRecords.at(0)["type"] == widthType);
    REQUIRE(squareFieldsRecords.at(0)["little_endian"] == little_endian);

    std::string getStuffType{"SELECT * FROM symbols where id="};
    getStuffType += squareFieldsRecords.at(1)["type"];
    getStuffType += ";";

    std::vector<std::map<std::string, std::string>> stuffTypeRecords{};

    rc = sqlite3_exec(database, getStuffType.c_str(), selectCallbackUsingColNameAsKey, &stuffTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(stuffTypeRecords.size() == 1);

    REQUIRE(stuffTypeRecords.at(0)["name"] == "uint16_t");
    REQUIRE(stuffTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(uint16_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, stuffTypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_unsigned");
    REQUIRE(stuffTypeRecords.at(0)["short_description"] == "");
    REQUIRE(stuffTypeRecords.at(0)["long_description"] == "");

    std::string stuffType{stuffTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(1)["name"] == "stuff");
    REQUIRE(squareFieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(Square, stuff)));
    REQUIRE(squareFieldsRecords.at(1)["type"] == stuffType);
    REQUIRE(squareFieldsRecords.at(1)["little_endian"] == little_endian);

    std::string getPadding1Type{"SELECT * FROM symbols where id="};
    getPadding1Type += squareFieldsRecords.at(2)["type"];
    getPadding1Type += ";";

    std::vector<std::map<std::string, std::string>> padding1TypeRecords{};

    rc = sqlite3_exec(database, getPadding1Type.c_str(), selectCallbackUsingColNameAsKey, &padding1TypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(padding1TypeRecords.size() == 1);

    REQUIRE(padding1TypeRecords.at(0)["name"] == "uint16_t");
    REQUIRE(padding1TypeRecords.at(0)["byte_size"] == std::to_string(sizeof(uint16_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, padding1TypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_unsigned");
    REQUIRE(padding1TypeRecords.at(0)["short_description"] == "");
    REQUIRE(padding1TypeRecords.at(0)["long_description"] == "");

    std::string padding1Type{padding1TypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(2)["name"] == "padding1");
    REQUIRE(squareFieldsRecords.at(2)["byte_offset"] == std::to_string(offsetof(Square, padding1)));
    REQUIRE(squareFieldsRecords.at(2)["type"] == padding1Type);
    REQUIRE(squareFieldsRecords.at(2)["little_endian"] == little_endian);

    std::string getLengthType{"SELECT * FROM symbols where id="};
    getLengthType += squareFieldsRecords.at(3)["type"];
    getLengthType += ";";

    std::vector<std::map<std::string, std::string>> lengthTypeRecords{};

    rc = sqlite3_exec(database, getLengthType.c_str(), selectCallbackUsingColNameAsKey, &lengthTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(lengthTypeRecords.size() == 1);

    REQUIRE(lengthTypeRecords.at(0)["name"] == "int32_t");
    REQUIRE(lengthTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(int32_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, lengthTypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_signed");
    REQUIRE(lengthTypeRecords.at(0)["short_description"] == "");
    REQUIRE(lengthTypeRecords.at(0)["long_description"] == "");

    std::string lengthType{lengthTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(3)["name"] == "length");
    REQUIRE(squareFieldsRecords.at(3)["byte_offset"] == std::to_string(offsetof(Square, length)));
    REQUIRE(squareFieldsRecords.at(3)["type"] == lengthType);
    REQUIRE(squareFieldsRecords.at(3)["little_endian"] == little_endian);

    std::string getMoreStuffType{"SELECT * FROM symbols where id="};
    getMoreStuffType += squareFieldsRecords.at(4)["type"];
    getMoreStuffType += ";";

    std::vector<std::map<std::string, std::string>> moreStuffTypeRecords{};

    rc = sqlite3_exec(database, getMoreStuffType.c_str(), selectCallbackUsingColNameAsKey, &moreStuffTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(moreStuffTypeRecords.size() == 1);

    REQUIRE(moreStuffTypeRecords.at(0)["name"] == "uint16_t");
    REQUIRE(moreStuffTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(uint16_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, moreStuffTypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_unsigned");
    REQUIRE(moreStuffTypeRecords.at(0)["short_description"] == "");
    REQUIRE(moreStuffTypeRecords.at(0)["long_description"] == "");

    std::string moreStuffType{moreStuffTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(4)["name"] == "more_stuff");
    REQUIRE(squareFieldsRecords.at(4)["byte_offset"] == std::to_string(offsetof(Square, more_stuff)));
    REQUIRE(squareFieldsRecords.at(4)["type"] == moreStuffType);
    REQUIRE(squareFieldsRecords.at(4)["little_endian"] == little_endian);

    std::string getPadding2Type{"SELECT * FROM symbols where id="};
    getPadding2Type += squareFieldsRecords.at(5)["type"];
    getPadding2Type += ";";

    std::vector<std::map<std::string, std::string>> padding2TypeRecords{};

    rc = sqlite3_exec(database, getPadding2Type.c_str(), selectCallbackUsingColNameAsKey, &padding2TypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(padding2TypeRecords.size() == 1);

    REQUIRE(padding2TypeRecords.at(0)["name"] == "uint16_t");
    REQUIRE(padding2TypeRecords.at(0)["byte_size"] == std::to_string(sizeof(uint16_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, padding2TypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_unsigned");
    REQUIRE(padding2TypeRecords.at(0)["short_description"] == "");
    REQUIRE(padding2TypeRecords.at(0)["long_description"] == "");

    std::string padding2Type{padding2TypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(5)["name"] == "padding2");
    REQUIRE(squareFieldsRecords.at(5)["byte_offset"] == std::to_string(offsetof(Square, padding2)));
    REQUIRE(squareFieldsRecords.at(5)["type"] == padding2Type);
    REQUIRE(squareFieldsRecords.at(5)["little_endian"] == little_endian);

    std::string getFloatingStuffType{"SELECT * FROM symbols where id="};
    getFloatingStuffType += squareFieldsRecords.at(6)["type"];
    getFloatingStuffType += ";";

    std::vector<std::map<std::string, std::string>> floatingStuffTypeRecords{};

    rc = sqlite3_exec(database, getFloatingStuffType.c_str(), selectCallbackUsingColNameAsKey, &floatingStuffTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(floatingStuffTypeRecords.size() == 1);

    REQUIRE(floatingStuffTypeRecords.at(0)["name"] == "float");
    REQUIRE(floatingStuffTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(float)));
    REQUIRE(floatingStuffTypeRecords.at(0)["target_symbol"] == "NULL");

    REQUIRE(encodingMap.at(floatingStuffTypeRecords.at(0)["encoding"]) == "DW_ATE_float");
    REQUIRE(floatingStuffTypeRecords.at(0)["short_description"] == "");
    REQUIRE(floatingStuffTypeRecords.at(0)["long_description"] == "");

    std::string floatingStuffType{floatingStuffTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(6)["name"] == "floating_stuff");
    REQUIRE(squareFieldsRecords.at(6)["byte_offset"] == std::to_string(offsetof(Square, floating_stuff)));
    REQUIRE(squareFieldsRecords.at(6)["type"] == floatingStuffType);
    REQUIRE(squareFieldsRecords.at(6)["little_endian"] == little_endian);

    // Test matrix3D[2][4][4]
    std::string getMatrix3DDimensionLists{"SELECT * FROM dimension_lists WHERE field_id="};
    getMatrix3DDimensionLists += squareFieldsRecords.at(7)["id"];
    getMatrix3DDimensionLists += ";";

    std::vector<std::map<std::string, std::string>> matrix3DDimensionListsRecords{};

    rc = sqlite3_exec(database, getMatrix3DDimensionLists.c_str(), selectCallbackUsingColNameAsKey, &matrix3DDimensionListsRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(matrix3DDimensionListsRecords.size() == 3);

    // Enforce order of records by dim_order
    std::sort(matrix3DDimensionListsRecords.begin(), matrix3DDimensionListsRecords.end(),
              [](std::map<std::string, std::string> a, std::map<std::string, std::string> b) { return std::stoi(a["dim_order"]) < std::stoi(b["dim_order"]); });

    REQUIRE(matrix3DDimensionListsRecords.at(0)["field_id"] == squareFieldsRecords.at(7)["id"]);
    REQUIRE(matrix3DDimensionListsRecords.at(0)["dim_order"] == "0");
    REQUIRE(matrix3DDimensionListsRecords.at(0)["upper_bound"] == "1");

    REQUIRE(matrix3DDimensionListsRecords.at(1)["field_id"] == squareFieldsRecords.at(7)["id"]);
    REQUIRE(matrix3DDimensionListsRecords.at(1)["dim_order"] == "1");
    REQUIRE(matrix3DDimensionListsRecords.at(1)["upper_bound"] == "3");

    REQUIRE(matrix3DDimensionListsRecords.at(2)["field_id"] == squareFieldsRecords.at(7)["id"]);
    REQUIRE(matrix3DDimensionListsRecords.at(2)["dim_order"] == "2");
    REQUIRE(matrix3DDimensionListsRecords.at(2)["upper_bound"] == "3");

    // Test matrix3D[2][4][4]
    std::string getMatrix1DDimensionLists{"SELECT * FROM dimension_lists WHERE field_id="};
    getMatrix1DDimensionLists += squareFieldsRecords.at(8)["id"];
    getMatrix1DDimensionLists += ";";

    std::vector<std::map<std::string, std::string>> matrix1DDimensionListsRecords{};

    rc = sqlite3_exec(database, getMatrix1DDimensionLists.c_str(), selectCallbackUsingColNameAsKey, &matrix1DDimensionListsRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(matrix1DDimensionListsRecords.size() == 1);

    // Enforce order of records by dim_order
    std::sort(matrix1DDimensionListsRecords.begin(), matrix1DDimensionListsRecords.end(),
              [](std::map<std::string, std::string> a, std::map<std::string, std::string> b) { return std::stoi(a["dim_order"]) < std::stoi(b["dim_order"]); });

    REQUIRE(matrix1DDimensionListsRecords.at(0)["field_id"] == squareFieldsRecords.at(8)["id"]);
    REQUIRE(matrix1DDimensionListsRecords.at(0)["dim_order"] == "0");
    REQUIRE(matrix1DDimensionListsRecords.at(0)["upper_bound"] == "1");

    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}

TEST_CASE("Write keys to database that already exist", "[main_test#5]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;

    logger.logWarning("This is just a test.");

    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
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
    REQUIRE(remove("./test_db.sqlite") == 0);
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
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    REQUIRE(juicer.parse(inputFile) == JUICER_OK);

    logger.setLogFile("logFile");

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    delete idc;
    // Logger::instance->closeLogFile();
    REQUIRE(remove("./logFile") == 0);
    REQUIRE(remove("./test_db.sqlite") == 0);
}

TEST_CASE("Write Elf File to database with verbosity set to INFO", "[main_test#7]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{LOGGER_VERBOSITY_INFO};

    std::string     inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);
    juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    delete idc;
    REQUIRE(remove("./test_db.sqlite") == 0);
}

TEST_CASE("Write Elf File to database with invalid verbosity", "[main_test#8]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{-1};

    logger.logWarning("This is just a test.");

    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);
    juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    delete idc;
    REQUIRE(remove("./test_db.sqlite") == 0);
}

TEST_CASE(
    "Test the correctness of the CFE_ES_HousekeepingTlm_Payload_t struct after Juicer has processed it. "
    "This also tests the \"extras\" features such as ELF image data.",
    "[main_test#9]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc            = 0;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);
    juicer.setExtras(true);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    std::string getSquareStructQuery{"SELECT * FROM symbols WHERE name = \"CFE_ES_HousekeepingTlm_Payload_t\"; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3* database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::vector<std::map<std::string, std::string>> hkRecords{};

    rc = sqlite3_exec(database, getSquareStructQuery.c_str(), selectCallbackUsingColNameAsKey, &hkRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(hkRecords.size() == 1);

    uint32_t numberOfColumns = 0;

    for (auto pair : hkRecords.at(0))
    {
        numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 9);

    /**
     * Check the correctness of Square struct.
     */

    REQUIRE(hkRecords.at(0)["name"] == "CFE_ES_HousekeepingTlm_Payload_t");

    REQUIRE(hkRecords.at(0)["target_symbol"] != "NULL");
    REQUIRE(hkRecords.at(0)["byte_size"] == std::to_string(sizeof(CFE_ES_HousekeepingTlm_Payload_t)));

    std::string hkTargetSymbolId = followTargetSymbol(database, hkRecords.at(0)["id"]).at("id");

    std::string squareArtifactId = hkRecords.at(0)["artifact"];

    REQUIRE(!squareArtifactId.empty());

    std::string getSquareArtifact{"SELECT * FROM artifacts WHERE id = "};

    getSquareArtifact += squareArtifactId;
    getSquareArtifact += ";";

    std::vector<std::map<std::string, std::string>> squareArtifactRecords{};

    rc = sqlite3_exec(database, getSquareArtifact.c_str(), selectCallbackUsingColNameAsKey, &squareArtifactRecords, &errorMessage);

    REQUIRE(squareArtifactRecords.size() == 1);

    std::string path{};
    char        resolvedPath[PATH_MAX];

    realpath("../unit-test/test_file1.h", resolvedPath);

    path.clear();
    path.insert(0, resolvedPath);

    REQUIRE(squareArtifactRecords.at(0)["path"] == path);

    std::string expectedMD5Str = getmd5sumFromSystem(resolvedPath);
    REQUIRE(expectedMD5Str == squareArtifactRecords.at(0)["md5"]);

    std::string getHKFields{"SELECT * FROM fields WHERE symbol = "};

    getHKFields += hkTargetSymbolId;
    getHKFields += ";";

    std::vector<std::map<std::string, std::string>> hkFieldsRecords{};

    rc = sqlite3_exec(database, getHKFields.c_str(), selectCallbackUsingColNameAsKey, &hkFieldsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(hkFieldsRecords.size() == 42);

    // Enforce order of records by offset
    std::sort(hkFieldsRecords.begin(), hkFieldsRecords.end(), [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
              { return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]); });

    std::string getCommandCounterType{"SELECT * FROM symbols where id="};
    getCommandCounterType += hkFieldsRecords.at(0)["type"];
    getCommandCounterType += ";";

    std::vector<std::map<std::string, std::string>> CommandCounterTypeRecords{};

    rc = sqlite3_exec(database, getCommandCounterType.c_str(), selectCallbackUsingColNameAsKey, &CommandCounterTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(CommandCounterTypeRecords.size() == 1);

    std::string widthType{CommandCounterTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(0)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(0)["name"] == "CommandCounter");
    REQUIRE(hkFieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, CommandCounter)));
    REQUIRE(hkFieldsRecords.at(0)["type"] == widthType);
    REQUIRE(hkFieldsRecords.at(0)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(0)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(0)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(0)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(0)["long_description"] == "");

    std::string getCommandErrorCounterType{"SELECT * FROM symbols where id="};
    getCommandErrorCounterType += hkFieldsRecords.at(1)["type"];
    getCommandErrorCounterType += ";";

    std::vector<std::map<std::string, std::string>> CommandErrorCounterTypeRecords{};

    rc = sqlite3_exec(database, getCommandErrorCounterType.c_str(), selectCallbackUsingColNameAsKey, &CommandErrorCounterTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(CommandErrorCounterTypeRecords.size() == 1);

    std::string CommandErrorCounterType{CommandErrorCounterTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(1)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(1)["name"] == "CommandErrorCounter");
    REQUIRE(hkFieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, CommandErrorCounter)));
    REQUIRE(hkFieldsRecords.at(1)["type"] == CommandErrorCounterType);
    REQUIRE(hkFieldsRecords.at(1)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(1)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(1)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(1)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(1)["long_description"] == "");

    std::string getCFECoreChecksumTypeType{"SELECT * FROM symbols where id="};
    getCFECoreChecksumTypeType += hkFieldsRecords.at(2)["type"];
    getCFECoreChecksumTypeType += ";";

    std::vector<std::map<std::string, std::string>> CFECoreChecksumTypeTypeRecords{};

    rc = sqlite3_exec(database, getCFECoreChecksumTypeType.c_str(), selectCallbackUsingColNameAsKey, &CFECoreChecksumTypeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(CFECoreChecksumTypeTypeRecords.size() == 1);

    std::string CFECoreChecksumType{CFECoreChecksumTypeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(2)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(2)["name"] == "CFECoreChecksum");
    REQUIRE(hkFieldsRecords.at(2)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, CFECoreChecksum)));
    REQUIRE(hkFieldsRecords.at(2)["type"] == CFECoreChecksumType);
    REQUIRE(hkFieldsRecords.at(2)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(2)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(2)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(2)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(2)["long_description"] == "");

    std::string getCFEMajorVersionType{"SELECT * FROM symbols where id="};
    getCFEMajorVersionType += hkFieldsRecords.at(3)["type"];
    getCFEMajorVersionType += ";";

    std::vector<std::map<std::string, std::string>> CFEMajorVersionTypeRecords{};

    rc = sqlite3_exec(database, getCFEMajorVersionType.c_str(), selectCallbackUsingColNameAsKey, &CFEMajorVersionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(CFEMajorVersionTypeRecords.size() == 1);

    std::string CFEMajorVersionType{CFEMajorVersionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(3)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(3)["name"] == "CFEMajorVersion");
    REQUIRE(hkFieldsRecords.at(3)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, CFEMajorVersion)));
    REQUIRE(hkFieldsRecords.at(3)["type"] == CFEMajorVersionType);
    REQUIRE(hkFieldsRecords.at(3)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(3)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(3)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(3)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(3)["long_description"] == "");

    std::string getMoreStuffType{"SELECT * FROM symbols where id="};
    getMoreStuffType += hkFieldsRecords.at(4)["type"];
    getMoreStuffType += ";";

    std::vector<std::map<std::string, std::string>> CFEMinorVersionTypeTypeRecords{};

    rc = sqlite3_exec(database, getMoreStuffType.c_str(), selectCallbackUsingColNameAsKey, &CFEMinorVersionTypeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(CFEMinorVersionTypeTypeRecords.size() == 1);

    std::string CFEMinorVersionType{CFEMinorVersionTypeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(4)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(4)["name"] == "CFEMinorVersion");
    REQUIRE(hkFieldsRecords.at(4)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, CFEMinorVersion)));
    REQUIRE(hkFieldsRecords.at(4)["type"] == CFEMinorVersionType);
    REQUIRE(hkFieldsRecords.at(4)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(4)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(4)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(4)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(4)["long_description"] == "");

    std::string getPadding2Type{"SELECT * FROM symbols where id="};
    getPadding2Type += hkFieldsRecords.at(5)["type"];
    getPadding2Type += ";";

    std::vector<std::map<std::string, std::string>> padding2TypeRecords{};

    rc = sqlite3_exec(database, getPadding2Type.c_str(), selectCallbackUsingColNameAsKey, &padding2TypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(padding2TypeRecords.size() == 1);

    std::string padding2Type{padding2TypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(5)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(5)["name"] == "CFERevision");
    REQUIRE(hkFieldsRecords.at(5)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, CFERevision)));
    REQUIRE(hkFieldsRecords.at(5)["type"] == padding2Type);
    REQUIRE(hkFieldsRecords.at(5)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(5)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(5)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(5)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(5)["long_description"] == "");

    std::string getCFEMissionRevisioType{"SELECT * FROM symbols where id="};
    getCFEMissionRevisioType += hkFieldsRecords.at(6)["type"];
    getCFEMissionRevisioType += ";";

    std::vector<std::map<std::string, std::string>> CFEMissionRevisionTypeRecords{};

    rc = sqlite3_exec(database, getCFEMissionRevisioType.c_str(), selectCallbackUsingColNameAsKey, &CFEMissionRevisionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(CFEMissionRevisionTypeRecords.size() == 1);

    std::string CFEMissionRevisionType{CFEMissionRevisionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(6)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(6)["name"] == "CFEMissionRevision");
    REQUIRE(hkFieldsRecords.at(6)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, CFEMissionRevision)));
    REQUIRE(hkFieldsRecords.at(6)["type"] == CFEMissionRevisionType);
    REQUIRE(hkFieldsRecords.at(6)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(6)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(6)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(6)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(6)["long_description"] == "");

    std::string getOSALMajorVersionType{"SELECT * FROM symbols where id="};
    getOSALMajorVersionType += hkFieldsRecords.at(7)["type"];
    getOSALMajorVersionType += ";";

    std::vector<std::map<std::string, std::string>> OSALMajorVersionTypeRecords{};

    rc = sqlite3_exec(database, getOSALMajorVersionType.c_str(), selectCallbackUsingColNameAsKey, &OSALMajorVersionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(OSALMajorVersionTypeRecords.size() == 1);

    std::string OSALMajorVersionType{OSALMajorVersionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(7)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(7)["name"] == "OSALMajorVersion");
    REQUIRE(hkFieldsRecords.at(7)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, OSALMajorVersion)));
    REQUIRE(hkFieldsRecords.at(7)["type"] == OSALMajorVersionType);
    REQUIRE(hkFieldsRecords.at(7)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(7)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(7)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(7)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(7)["long_description"] == "");

    std::string getOSALMinorVersionType{"SELECT * FROM symbols where id="};
    getOSALMinorVersionType += hkFieldsRecords.at(8)["type"];
    getOSALMinorVersionType += ";";

    std::vector<std::map<std::string, std::string>> OSALMinorVersionTypeRecords{};

    rc = sqlite3_exec(database, getOSALMinorVersionType.c_str(), selectCallbackUsingColNameAsKey, &OSALMinorVersionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(OSALMinorVersionTypeRecords.size() == 1);

    std::string OSALMinorVersionType{OSALMinorVersionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(8)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(8)["name"] == "OSALMinorVersion");
    REQUIRE(hkFieldsRecords.at(8)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, OSALMinorVersion)));
    REQUIRE(hkFieldsRecords.at(8)["type"] == OSALMinorVersionType);
    REQUIRE(hkFieldsRecords.at(8)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(8)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(8)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(8)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(8)["long_description"] == "");

    std::string getOSALRevisionType{"SELECT * FROM symbols where id="};
    getOSALRevisionType += hkFieldsRecords.at(9)["type"];
    getOSALRevisionType += ";";

    std::vector<std::map<std::string, std::string>> OSALRevisionTypeRecords{};

    rc = sqlite3_exec(database, getOSALRevisionType.c_str(), selectCallbackUsingColNameAsKey, &OSALRevisionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(OSALRevisionTypeRecords.size() == 1);

    std::string OSALRevisionType{OSALRevisionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(9)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(9)["name"] == "OSALRevision");
    REQUIRE(hkFieldsRecords.at(9)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, OSALRevision)));
    REQUIRE(hkFieldsRecords.at(9)["type"] == OSALRevisionType);
    REQUIRE(hkFieldsRecords.at(9)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(9)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(9)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(9)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(9)["long_description"] == "");

    std::string getOSALMissionRevisionType{"SELECT * FROM symbols where id="};
    getOSALMissionRevisionType += hkFieldsRecords.at(10)["type"];
    getOSALMissionRevisionType += ";";

    std::vector<std::map<std::string, std::string>> OSALMissionRevisionTypeRecords{};

    rc = sqlite3_exec(database, getOSALMissionRevisionType.c_str(), selectCallbackUsingColNameAsKey, &OSALMissionRevisionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(OSALMissionRevisionTypeRecords.size() == 1);

    std::string OSALMissionRevisionType{OSALMissionRevisionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(10)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(10)["name"] == "OSALMissionRevision");
    REQUIRE(hkFieldsRecords.at(10)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, OSALMissionRevision)));
    REQUIRE(hkFieldsRecords.at(10)["type"] == OSALMissionRevisionType);
    REQUIRE(hkFieldsRecords.at(10)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(10)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(10)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(10)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(10)["long_description"] == "");

    std::string getPSPMajorVersionType{"SELECT * FROM symbols where id="};
    getPSPMajorVersionType += hkFieldsRecords.at(11)["type"];
    getPSPMajorVersionType += ";";

    std::vector<std::map<std::string, std::string>> PSPMajorVersionTypeRecords{};

    rc = sqlite3_exec(database, getPSPMajorVersionType.c_str(), selectCallbackUsingColNameAsKey, &PSPMajorVersionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PSPMajorVersionTypeRecords.size() == 1);

    std::string PSPMajorVersionType{PSPMajorVersionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(11)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(11)["name"] == "PSPMajorVersion");
    REQUIRE(hkFieldsRecords.at(11)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PSPMajorVersion)));
    REQUIRE(hkFieldsRecords.at(11)["type"] == PSPMajorVersionType);
    REQUIRE(hkFieldsRecords.at(11)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(11)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(11)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(11)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(11)["long_description"] == "");

    std::string getPSPMinorVersionType{"SELECT * FROM symbols where id="};
    getPSPMinorVersionType += hkFieldsRecords.at(12)["type"];
    getPSPMinorVersionType += ";";

    std::vector<std::map<std::string, std::string>> PSPMinorVersionTypeRecords{};

    rc = sqlite3_exec(database, getPSPMinorVersionType.c_str(), selectCallbackUsingColNameAsKey, &PSPMinorVersionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PSPMinorVersionTypeRecords.size() == 1);

    std::string PSPMinorVersionType{PSPMinorVersionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(12)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(12)["name"] == "PSPMinorVersion");
    REQUIRE(hkFieldsRecords.at(12)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PSPMinorVersion)));
    REQUIRE(hkFieldsRecords.at(12)["type"] == PSPMinorVersionType);
    REQUIRE(hkFieldsRecords.at(12)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(12)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(12)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(12)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(12)["long_description"] == "");

    std::string getPSPRevisionType{"SELECT * FROM symbols where id="};
    getPSPRevisionType += hkFieldsRecords.at(13)["type"];
    getPSPRevisionType += ";";

    std::vector<std::map<std::string, std::string>> PSPRevisionTypeRecords{};

    rc = sqlite3_exec(database, getPSPRevisionType.c_str(), selectCallbackUsingColNameAsKey, &PSPRevisionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PSPRevisionTypeRecords.size() == 1);

    std::string PSPRevisionType{PSPRevisionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(13)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(13)["name"] == "PSPRevision");
    REQUIRE(hkFieldsRecords.at(13)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PSPRevision)));
    REQUIRE(hkFieldsRecords.at(13)["type"] == PSPRevisionType);
    REQUIRE(hkFieldsRecords.at(13)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(13)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(13)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(13)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(13)["long_description"] == "");

    std::string getPSPMissionRevisionType{"SELECT * FROM symbols where id="};
    getPSPMissionRevisionType += hkFieldsRecords.at(14)["type"];
    getPSPMissionRevisionType += ";";

    std::vector<std::map<std::string, std::string>> PSPMissionRevisionTypeRecords{};

    rc = sqlite3_exec(database, getPSPMissionRevisionType.c_str(), selectCallbackUsingColNameAsKey, &PSPMissionRevisionTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PSPMissionRevisionTypeRecords.size() == 1);

    std::string PSPMissionRevisionType{PSPMissionRevisionTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(14)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(14)["name"] == "PSPMissionRevision");
    REQUIRE(hkFieldsRecords.at(14)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PSPMissionRevision)));
    REQUIRE(hkFieldsRecords.at(14)["type"] == PSPMissionRevisionType);
    REQUIRE(hkFieldsRecords.at(14)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(14)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(14)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(14)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(14)["long_description"] == "");

    std::string getSysLogBytesUsedType{"SELECT * FROM symbols where id="};
    getSysLogBytesUsedType += hkFieldsRecords.at(15)["type"];
    getSysLogBytesUsedType += ";";

    std::vector<std::map<std::string, std::string>> SysLogBytesUsedTypeRecords{};

    rc = sqlite3_exec(database, getSysLogBytesUsedType.c_str(), selectCallbackUsingColNameAsKey, &SysLogBytesUsedTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(SysLogBytesUsedTypeRecords.size() == 1);

    std::string SysLogBytesUsedType{SysLogBytesUsedTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(15)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(15)["name"] == "SysLogBytesUsed");
    REQUIRE(hkFieldsRecords.at(15)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, SysLogBytesUsed)));
    REQUIRE(hkFieldsRecords.at(15)["type"] == SysLogBytesUsedType);
    REQUIRE(hkFieldsRecords.at(15)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(15)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(15)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(15)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(15)["long_description"] == "");

    std::string getSysLogSizeType{"SELECT * FROM symbols where id="};
    getSysLogSizeType += hkFieldsRecords.at(16)["type"];
    getSysLogSizeType += ";";

    std::vector<std::map<std::string, std::string>> SysLogSizeTypeRecords{};

    rc = sqlite3_exec(database, getSysLogSizeType.c_str(), selectCallbackUsingColNameAsKey, &SysLogSizeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(SysLogSizeTypeRecords.size() == 1);

    std::string SysLogSizeType{SysLogSizeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(16)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(16)["name"] == "SysLogSize");
    REQUIRE(hkFieldsRecords.at(16)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, SysLogSize)));
    REQUIRE(hkFieldsRecords.at(16)["type"] == SysLogSizeType);
    REQUIRE(hkFieldsRecords.at(16)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(16)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(16)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(16)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(16)["long_description"] == "");

    std::string getSysLogEntriesType{"SELECT * FROM symbols where id="};
    getSysLogEntriesType += hkFieldsRecords.at(17)["type"];
    getSysLogEntriesType += ";";

    std::vector<std::map<std::string, std::string>> SysLogEntriesTypeRecords{};

    rc = sqlite3_exec(database, getSysLogEntriesType.c_str(), selectCallbackUsingColNameAsKey, &SysLogEntriesTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(SysLogEntriesTypeRecords.size() == 1);

    std::string SysLogEntriesType{SysLogEntriesTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(17)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(17)["name"] == "SysLogEntries");
    REQUIRE(hkFieldsRecords.at(17)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, SysLogEntries)));
    REQUIRE(hkFieldsRecords.at(17)["type"] == SysLogEntriesType);
    REQUIRE(hkFieldsRecords.at(17)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(17)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(17)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(17)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(17)["long_description"] == "");

    std::string getSysLogModeType{"SELECT * FROM symbols where id="};
    getSysLogModeType += hkFieldsRecords.at(18)["type"];
    getSysLogModeType += ";";

    std::vector<std::map<std::string, std::string>> SysLogModeTypeRecords{};

    rc = sqlite3_exec(database, getSysLogModeType.c_str(), selectCallbackUsingColNameAsKey, &SysLogModeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(SysLogModeTypeRecords.size() == 1);

    std::string SysLogModeType{SysLogModeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(18)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(18)["name"] == "SysLogMode");
    REQUIRE(hkFieldsRecords.at(18)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, SysLogMode)));
    REQUIRE(hkFieldsRecords.at(18)["type"] == SysLogModeType);
    REQUIRE(hkFieldsRecords.at(18)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(18)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(18)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(18)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(18)["long_description"] == "");

    std::string getERLogIndexType{"SELECT * FROM symbols where id="};
    getERLogIndexType += hkFieldsRecords.at(19)["type"];
    getERLogIndexType += ";";

    std::vector<std::map<std::string, std::string>> ERLogIndexTypeRecords{};

    rc = sqlite3_exec(database, getERLogIndexType.c_str(), selectCallbackUsingColNameAsKey, &ERLogIndexTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(ERLogIndexTypeRecords.size() == 1);

    std::string ERLogIndexType{ERLogIndexTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(19)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(19)["name"] == "ERLogIndex");
    REQUIRE(hkFieldsRecords.at(19)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, ERLogIndex)));
    REQUIRE(hkFieldsRecords.at(19)["type"] == ERLogIndexType);
    REQUIRE(hkFieldsRecords.at(19)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(19)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(19)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(19)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(19)["long_description"] == "");

    std::string getERLogEntriesType{"SELECT * FROM symbols where id="};
    getERLogEntriesType += hkFieldsRecords.at(20)["type"];
    getERLogEntriesType += ";";

    std::vector<std::map<std::string, std::string>> ERLogEntriesTypeRecords{};

    rc = sqlite3_exec(database, getERLogEntriesType.c_str(), selectCallbackUsingColNameAsKey, &ERLogEntriesTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(ERLogEntriesTypeRecords.size() == 1);

    std::string ERLogEntriesType{ERLogEntriesTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(20)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(20)["name"] == "ERLogEntries");
    REQUIRE(hkFieldsRecords.at(20)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, ERLogEntries)));
    REQUIRE(hkFieldsRecords.at(20)["type"] == ERLogEntriesType);
    REQUIRE(hkFieldsRecords.at(20)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(20)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(20)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(20)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(20)["long_description"] == "");

    std::string getRegisteredCoreAppsType{"SELECT * FROM symbols where id="};
    getRegisteredCoreAppsType += hkFieldsRecords.at(21)["type"];
    getRegisteredCoreAppsType += ";";

    std::vector<std::map<std::string, std::string>> RegisteredCoreAppsTypeRecords{};

    rc = sqlite3_exec(database, getRegisteredCoreAppsType.c_str(), selectCallbackUsingColNameAsKey, &RegisteredCoreAppsTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(RegisteredCoreAppsTypeRecords.size() == 1);

    std::string RegisteredCoreAppsType{RegisteredCoreAppsTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(21)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(21)["name"] == "RegisteredCoreApps");
    REQUIRE(hkFieldsRecords.at(21)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, RegisteredCoreApps)));
    REQUIRE(hkFieldsRecords.at(21)["type"] == RegisteredCoreAppsType);
    REQUIRE(hkFieldsRecords.at(21)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(21)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(21)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(21)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(21)["long_description"] == "");

    std::string getRegisteredExternalAppsType{"SELECT * FROM symbols where id="};
    getRegisteredExternalAppsType += hkFieldsRecords.at(22)["type"];
    getRegisteredExternalAppsType += ";";

    std::vector<std::map<std::string, std::string>> RegisteredExternalAppsTypeRecords{};

    rc = sqlite3_exec(database, getRegisteredExternalAppsType.c_str(), selectCallbackUsingColNameAsKey, &RegisteredExternalAppsTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(RegisteredExternalAppsTypeRecords.size() == 1);

    std::string RegisteredExternalAppsType{RegisteredExternalAppsTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(22)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(22)["name"] == "RegisteredExternalApps");
    REQUIRE(hkFieldsRecords.at(22)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, RegisteredExternalApps)));
    REQUIRE(hkFieldsRecords.at(22)["type"] == RegisteredExternalAppsType);
    REQUIRE(hkFieldsRecords.at(22)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(22)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(22)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(22)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(22)["long_description"] == "");

    std::string getRegisteredTasksType{"SELECT * FROM symbols where id="};
    getRegisteredTasksType += hkFieldsRecords.at(23)["type"];
    getRegisteredTasksType += ";";

    std::vector<std::map<std::string, std::string>> RegisteredTasksTypeRecords{};

    rc = sqlite3_exec(database, getRegisteredTasksType.c_str(), selectCallbackUsingColNameAsKey, &RegisteredTasksTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(RegisteredTasksTypeRecords.size() == 1);

    std::string RegisteredTasksType{RegisteredTasksTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(23)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(23)["name"] == "RegisteredTasks");
    REQUIRE(hkFieldsRecords.at(23)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, RegisteredTasks)));
    REQUIRE(hkFieldsRecords.at(23)["type"] == RegisteredTasksType);
    REQUIRE(hkFieldsRecords.at(23)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(23)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(23)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(23)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(23)["long_description"] == "");

    std::string getRegisteredLibsType{"SELECT * FROM symbols where id="};
    getRegisteredLibsType += hkFieldsRecords.at(24)["type"];
    getRegisteredLibsType += ";";

    std::vector<std::map<std::string, std::string>> RegisteredLibsTypeRecords{};

    rc = sqlite3_exec(database, getRegisteredLibsType.c_str(), selectCallbackUsingColNameAsKey, &RegisteredLibsTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(RegisteredLibsTypeRecords.size() == 1);

    std::string RegisteredLibsType{RegisteredLibsTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(24)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(24)["name"] == "RegisteredLibs");
    REQUIRE(hkFieldsRecords.at(24)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, RegisteredLibs)));
    REQUIRE(hkFieldsRecords.at(24)["type"] == RegisteredLibsType);
    REQUIRE(hkFieldsRecords.at(24)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(24)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(24)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(24)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(24)["long_description"] == "");

    std::string getResetTypeType{"SELECT * FROM symbols where id="};
    getResetTypeType += hkFieldsRecords.at(25)["type"];
    getResetTypeType += ";";

    std::vector<std::map<std::string, std::string>> ResetTypeTypeRecords{};

    rc = sqlite3_exec(database, getResetTypeType.c_str(), selectCallbackUsingColNameAsKey, &ResetTypeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(ResetTypeTypeRecords.size() == 1);

    std::string ResetTypeType{ResetTypeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(25)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(25)["name"] == "ResetType");
    REQUIRE(hkFieldsRecords.at(25)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, ResetType)));
    REQUIRE(hkFieldsRecords.at(25)["type"] == ResetTypeType);
    REQUIRE(hkFieldsRecords.at(25)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(25)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(25)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(25)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(25)["long_description"] == "");

    std::string getResetSubtypeType{"SELECT * FROM symbols where id="};
    getResetSubtypeType += hkFieldsRecords.at(26)["type"];
    getResetSubtypeType += ";";

    std::vector<std::map<std::string, std::string>> ResetSubtypeTypeRecords{};

    rc = sqlite3_exec(database, getResetSubtypeType.c_str(), selectCallbackUsingColNameAsKey, &ResetSubtypeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(ResetSubtypeTypeRecords.size() == 1);

    std::string ResetSubtypeType{ResetSubtypeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(26)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(26)["name"] == "ResetSubtype");
    REQUIRE(hkFieldsRecords.at(26)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, ResetSubtype)));
    REQUIRE(hkFieldsRecords.at(26)["type"] == ResetSubtypeType);
    REQUIRE(hkFieldsRecords.at(26)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(26)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(26)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(26)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(26)["long_description"] == "");

    std::string getProcessorResetsType{"SELECT * FROM symbols where id="};
    getProcessorResetsType += hkFieldsRecords.at(27)["type"];
    getProcessorResetsType += ";";

    std::vector<std::map<std::string, std::string>> ProcessorResetsTypeRecords{};

    rc = sqlite3_exec(database, getProcessorResetsType.c_str(), selectCallbackUsingColNameAsKey, &ProcessorResetsTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(ProcessorResetsTypeRecords.size() == 1);

    std::string ProcessorResetsType{ProcessorResetsTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(27)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(27)["name"] == "ProcessorResets");
    REQUIRE(hkFieldsRecords.at(27)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, ProcessorResets)));
    REQUIRE(hkFieldsRecords.at(27)["type"] == ProcessorResetsType);
    REQUIRE(hkFieldsRecords.at(27)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(27)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(27)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(27)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(27)["long_description"] == "");

    std::string getMaxProcessorResetsType{"SELECT * FROM symbols where id="};
    getMaxProcessorResetsType += hkFieldsRecords.at(28)["type"];
    getMaxProcessorResetsType += ";";

    std::vector<std::map<std::string, std::string>> MaxProcessorResetsTypeRecords{};

    rc = sqlite3_exec(database, getMaxProcessorResetsType.c_str(), selectCallbackUsingColNameAsKey, &MaxProcessorResetsTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(MaxProcessorResetsTypeRecords.size() == 1);

    std::string MaxProcessorResetsType{MaxProcessorResetsTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(28)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(28)["name"] == "MaxProcessorResets");
    REQUIRE(hkFieldsRecords.at(28)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, MaxProcessorResets)));
    REQUIRE(hkFieldsRecords.at(28)["type"] == MaxProcessorResetsType);
    REQUIRE(hkFieldsRecords.at(28)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(28)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(28)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(28)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(28)["long_description"] == "");

    std::string getBootSourceType{"SELECT * FROM symbols where id="};
    getBootSourceType += hkFieldsRecords.at(29)["type"];
    getBootSourceType += ";";

    std::vector<std::map<std::string, std::string>> BootSourceTypeRecords{};

    rc = sqlite3_exec(database, getBootSourceType.c_str(), selectCallbackUsingColNameAsKey, &BootSourceTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(BootSourceTypeRecords.size() == 1);

    std::string BootSourceType{BootSourceTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(29)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(29)["name"] == "BootSource");
    REQUIRE(hkFieldsRecords.at(29)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, BootSource)));
    REQUIRE(hkFieldsRecords.at(29)["type"] == BootSourceType);
    REQUIRE(hkFieldsRecords.at(29)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(29)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(29)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(29)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(29)["long_description"] == "");

    std::string getPerfStateType{"SELECT * FROM symbols where id="};
    getPerfStateType += hkFieldsRecords.at(30)["type"];
    getPerfStateType += ";";

    std::vector<std::map<std::string, std::string>> PerfStateTypeRecords{};

    rc = sqlite3_exec(database, getPerfStateType.c_str(), selectCallbackUsingColNameAsKey, &PerfStateTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfStateTypeRecords.size() == 1);

    std::string PerfStateType{PerfStateTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(30)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(30)["name"] == "PerfState");
    REQUIRE(hkFieldsRecords.at(30)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfState)));
    REQUIRE(hkFieldsRecords.at(30)["type"] == PerfStateType);
    REQUIRE(hkFieldsRecords.at(30)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(30)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(30)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(30)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(30)["long_description"] == "");

    std::string getPerfModeType{"SELECT * FROM symbols where id="};
    getPerfModeType += hkFieldsRecords.at(31)["type"];
    getPerfModeType += ";";

    std::vector<std::map<std::string, std::string>> PerfModeTypeRecords{};

    rc = sqlite3_exec(database, getPerfModeType.c_str(), selectCallbackUsingColNameAsKey, &PerfModeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfModeTypeRecords.size() == 1);

    std::string PerfModeType{PerfModeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(31)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(31)["name"] == "PerfMode");
    REQUIRE(hkFieldsRecords.at(31)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfMode)));
    REQUIRE(hkFieldsRecords.at(31)["type"] == PerfModeType);
    REQUIRE(hkFieldsRecords.at(31)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(31)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(31)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(31)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(31)["long_description"] == "");

    std::string getPerfTriggerCountType{"SELECT * FROM symbols where id="};
    getPerfTriggerCountType += hkFieldsRecords.at(32)["type"];
    getPerfTriggerCountType += ";";

    std::vector<std::map<std::string, std::string>> PerfTriggerCountTypeRecords{};

    rc = sqlite3_exec(database, getPerfTriggerCountType.c_str(), selectCallbackUsingColNameAsKey, &PerfTriggerCountTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfTriggerCountTypeRecords.size() == 1);

    std::string PerfTriggerCountType{PerfTriggerCountTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(32)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(32)["name"] == "PerfTriggerCount");
    REQUIRE(hkFieldsRecords.at(32)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfTriggerCount)));
    REQUIRE(hkFieldsRecords.at(32)["type"] == PerfTriggerCountType);
    REQUIRE(hkFieldsRecords.at(32)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(32)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(32)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(32)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(32)["long_description"] == "");

    std::string getPerfFilterMaskType{"SELECT * FROM symbols where id="};
    getPerfFilterMaskType += hkFieldsRecords.at(33)["type"];
    getPerfFilterMaskType += ";";

    std::vector<std::map<std::string, std::string>> PerfFilterMaskTypeRecords{};

    rc = sqlite3_exec(database, getPerfFilterMaskType.c_str(), selectCallbackUsingColNameAsKey, &PerfFilterMaskTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfFilterMaskTypeRecords.size() == 1);

    std::string PerfFilterMaskType{PerfFilterMaskTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(33)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(33)["name"] == "PerfFilterMask");
    REQUIRE(hkFieldsRecords.at(33)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfFilterMask)));
    REQUIRE(hkFieldsRecords.at(33)["type"] == PerfFilterMaskType);
    REQUIRE(hkFieldsRecords.at(33)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(33)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(33)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(33)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(33)["long_description"] == "");

    std::string getDimensionLists{"SELECT * FROM dimension_lists WHERE field_id="};
    getDimensionLists += hkFieldsRecords.at(33)["id"];
    getDimensionLists += ";";

    std::vector<std::map<std::string, std::string>> DimensionListsRecords{};

    rc = sqlite3_exec(database, getDimensionLists.c_str(), selectCallbackUsingColNameAsKey, &DimensionListsRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(DimensionListsRecords.size() == 1);

    // Enforce order of records by dim_order
    std::sort(DimensionListsRecords.begin(), DimensionListsRecords.end(),
              [](std::map<std::string, std::string> a, std::map<std::string, std::string> b) { return std::stoi(a["dim_order"]) < std::stoi(b["dim_order"]); });

    REQUIRE(DimensionListsRecords.at(0)["field_id"] == hkFieldsRecords.at(33)["id"]);
    REQUIRE(DimensionListsRecords.at(0)["dim_order"] == "0");
    REQUIRE(DimensionListsRecords.at(0)["upper_bound"] == "3");

    std::string getPerfTriggerMaskType{"SELECT * FROM symbols where id="};
    getPerfTriggerMaskType += hkFieldsRecords.at(34)["type"];
    getPerfTriggerMaskType += ";";

    std::vector<std::map<std::string, std::string>> PerfTriggerMaskTypeRecords{};

    rc = sqlite3_exec(database, getPerfTriggerMaskType.c_str(), selectCallbackUsingColNameAsKey, &PerfTriggerMaskTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfTriggerMaskTypeRecords.size() == 1);

    std::string PerfTriggerMaskType{PerfTriggerMaskTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(34)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(34)["name"] == "PerfTriggerMask");
    REQUIRE(hkFieldsRecords.at(34)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfTriggerMask)));
    REQUIRE(hkFieldsRecords.at(34)["type"] == PerfTriggerMaskType);
    REQUIRE(hkFieldsRecords.at(34)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(34)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(34)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(34)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(34)["long_description"] == "");

    getDimensionLists  = "SELECT * FROM dimension_lists WHERE field_id=";
    getDimensionLists += hkFieldsRecords.at(34)["id"];
    getDimensionLists += ";";

    DimensionListsRecords.clear();

    rc = sqlite3_exec(database, getDimensionLists.c_str(), selectCallbackUsingColNameAsKey, &DimensionListsRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(DimensionListsRecords.size() == 1);

    // Enforce order of records by dim_order
    std::sort(DimensionListsRecords.begin(), DimensionListsRecords.end(),
              [](std::map<std::string, std::string> a, std::map<std::string, std::string> b) { return std::stoi(a["dim_order"]) < std::stoi(b["dim_order"]); });

    REQUIRE(DimensionListsRecords.at(0)["field_id"] == hkFieldsRecords.at(34)["id"]);
    REQUIRE(DimensionListsRecords.at(0)["dim_order"] == "0");
    REQUIRE(DimensionListsRecords.at(0)["upper_bound"] == "3");

    std::string getPerfDataStartType{"SELECT * FROM symbols where id="};
    getPerfDataStartType += hkFieldsRecords.at(35)["type"];
    getPerfDataStartType += ";";

    std::vector<std::map<std::string, std::string>> PerfDataStartTypeRecords{};

    rc = sqlite3_exec(database, getPerfDataStartType.c_str(), selectCallbackUsingColNameAsKey, &PerfDataStartTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfDataStartTypeRecords.size() == 1);

    std::string PerfDataStartType{PerfDataStartTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(35)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(35)["name"] == "PerfDataStart");
    REQUIRE(hkFieldsRecords.at(35)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfDataStart)));
    REQUIRE(hkFieldsRecords.at(35)["type"] == PerfDataStartType);
    REQUIRE(hkFieldsRecords.at(35)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(35)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(35)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(35)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(35)["long_description"] == "");

    std::string getPerfDataEndType{"SELECT * FROM symbols where id="};
    getPerfDataEndType += hkFieldsRecords.at(36)["type"];
    getPerfDataEndType += ";";

    std::vector<std::map<std::string, std::string>> PerfDataEndTypeRecords{};

    rc = sqlite3_exec(database, getPerfDataEndType.c_str(), selectCallbackUsingColNameAsKey, &PerfDataEndTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfDataEndTypeRecords.size() == 1);

    std::string PerfDataEndType{PerfDataEndTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(36)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(36)["name"] == "PerfDataEnd");
    REQUIRE(hkFieldsRecords.at(36)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfDataEnd)));
    REQUIRE(hkFieldsRecords.at(36)["type"] == PerfDataEndType);
    REQUIRE(hkFieldsRecords.at(36)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(36)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(36)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(36)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(36)["long_description"] == "");

    std::string getPerfDataCountType{"SELECT * FROM symbols where id="};
    getPerfDataCountType += hkFieldsRecords.at(37)["type"];
    getPerfDataCountType += ";";

    std::vector<std::map<std::string, std::string>> PerfDataCountTypeRecords{};

    rc = sqlite3_exec(database, getPerfDataCountType.c_str(), selectCallbackUsingColNameAsKey, &PerfDataCountTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfDataCountTypeRecords.size() == 1);

    std::string PerfDataCountType{PerfDataCountTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(37)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(37)["name"] == "PerfDataCount");
    REQUIRE(hkFieldsRecords.at(37)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfDataCount)));
    REQUIRE(hkFieldsRecords.at(37)["type"] == PerfDataCountType);
    REQUIRE(hkFieldsRecords.at(37)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(37)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(37)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(37)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(37)["long_description"] == "");

    std::string getPerfDataToWriteType{"SELECT * FROM symbols where id="};
    getPerfDataToWriteType += hkFieldsRecords.at(38)["type"];
    getPerfDataToWriteType += ";";

    std::vector<std::map<std::string, std::string>> PerfDataToWriteTypeRecords{};

    rc = sqlite3_exec(database, getPerfDataToWriteType.c_str(), selectCallbackUsingColNameAsKey, &PerfDataToWriteTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(PerfDataToWriteTypeRecords.size() == 1);

    std::string PerfDataToWriteType{PerfDataToWriteTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(38)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(38)["name"] == "PerfDataToWrite");
    REQUIRE(hkFieldsRecords.at(38)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, PerfDataToWrite)));
    REQUIRE(hkFieldsRecords.at(38)["type"] == PerfDataToWriteType);
    REQUIRE(hkFieldsRecords.at(38)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(38)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(38)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(38)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(38)["long_description"] == "");

    std::string getHeapBytesFreeType{"SELECT * FROM symbols where id="};
    getHeapBytesFreeType += hkFieldsRecords.at(39)["type"];
    getHeapBytesFreeType += ";";

    std::vector<std::map<std::string, std::string>> HeapBytesFreeTypeRecords{};

    rc = sqlite3_exec(database, getHeapBytesFreeType.c_str(), selectCallbackUsingColNameAsKey, &HeapBytesFreeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(HeapBytesFreeTypeRecords.size() == 1);

    std::string HeapBytesFreeType{HeapBytesFreeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(39)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(39)["name"] == "HeapBytesFree");
    REQUIRE(hkFieldsRecords.at(39)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, HeapBytesFree)));
    REQUIRE(hkFieldsRecords.at(39)["type"] == HeapBytesFreeType);
    REQUIRE(hkFieldsRecords.at(39)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(39)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(39)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(39)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(39)["long_description"] == "");

    std::string getHeapBlocksFreeType{"SELECT * FROM symbols where id="};
    getHeapBlocksFreeType += hkFieldsRecords.at(40)["type"];
    getHeapBlocksFreeType += ";";

    std::vector<std::map<std::string, std::string>> HeapBlocksFreeTypeRecords{};

    rc = sqlite3_exec(database, getHeapBlocksFreeType.c_str(), selectCallbackUsingColNameAsKey, &HeapBlocksFreeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(HeapBlocksFreeTypeRecords.size() == 1);

    std::string HeapBlocksFreeType{HeapBlocksFreeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(40)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(40)["name"] == "HeapBlocksFree");
    REQUIRE(hkFieldsRecords.at(40)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, HeapBlocksFree)));
    REQUIRE(hkFieldsRecords.at(40)["type"] == HeapBlocksFreeType);
    REQUIRE(hkFieldsRecords.at(40)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(40)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(40)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(40)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(40)["long_description"] == "");

    std::string getHeapMaxBlockSizeType{"SELECT * FROM symbols where id="};
    getHeapMaxBlockSizeType += hkFieldsRecords.at(41)["type"];
    getHeapMaxBlockSizeType += ";";

    std::vector<std::map<std::string, std::string>> HeapMaxBlockSizeTypeRecords{};

    rc = sqlite3_exec(database, getHeapMaxBlockSizeType.c_str(), selectCallbackUsingColNameAsKey, &HeapMaxBlockSizeTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(HeapMaxBlockSizeTypeRecords.size() == 1);

    std::string HeapMaxBlockSizeType{HeapMaxBlockSizeTypeRecords.at(0)["id"]};

    REQUIRE(hkFieldsRecords.at(41)["symbol"] == hkTargetSymbolId);
    REQUIRE(hkFieldsRecords.at(41)["name"] == "HeapMaxBlockSize");
    REQUIRE(hkFieldsRecords.at(41)["byte_offset"] == std::to_string(offsetof(CFE_ES_HousekeepingTlm_Payload, HeapMaxBlockSize)));
    REQUIRE(hkFieldsRecords.at(41)["type"] == HeapMaxBlockSizeType);
    REQUIRE(hkFieldsRecords.at(41)["little_endian"] == little_endian);
    REQUIRE(hkFieldsRecords.at(41)["bit_size"] == "0");
    REQUIRE(hkFieldsRecords.at(41)["bit_offset"] == "0");
    REQUIRE(hkFieldsRecords.at(41)["short_description"] == "");
    REQUIRE(hkFieldsRecords.at(41)["long_description"] == "");

    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}

TEST_CASE("Test 32-bit binary.", "[main_test#10]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc            = 0;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_3};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);
    juicer.setExtras(true);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    std::string getSquareStructQuery{"SELECT * FROM symbols WHERE name = \"Square\"; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3* database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::string                                     getAllEncodings{"SELECT * FROM encodings"};

    std::vector<std::map<std::string, std::string>> encodingsRecords{};

    rc = sqlite3_exec(database, getAllEncodings.c_str(), selectCallbackUsingColNameAsKey, &encodingsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(encodingsRecords.size() == 18);

    /**
     * @brief encodingMap A map of row_id -> encoding.
     * i.e {"1":"DW_ATE_address", "2":"DW_ATE_boolean", etc}. Useful for avoiding having to look up the encoding by foreign of a symbol
     * every time.
     */
    std::map<std::string, std::string> encodingMap{};

    for (auto encodingRecord : encodingsRecords)
    {
        encodingMap[encodingRecord["id"]] = encodingRecord["encoding"];
    }

    std::vector<std::map<std::string, std::string>> squareRecords{};

    rc = sqlite3_exec(database, getSquareStructQuery.c_str(), selectCallbackUsingColNameAsKey, &squareRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(squareRecords.size() == 1);

    uint32_t numberOfColumns = 0;

    for (auto pair : squareRecords.at(0))
    {
        numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 9);

    /**
     * Check the correctness of Square struct.
     */

    REQUIRE(squareRecords.at(0)["name"] == "Square");
    REQUIRE(squareRecords.at(0)["byte_size"] == std::to_string(sizeof(Square)));

    REQUIRE(squareRecords.at(0)["target_symbol"] == "NULL");
    REQUIRE(squareRecords.at(0).at("encoding") == "NULL");
    REQUIRE(squareRecords.at(0)["short_description"] == "");
    REQUIRE(squareRecords.at(0)["long_description"] == "");

    std::string square_id          = squareRecords.at(0)["id"];

    std::string square_artifact_id = squareRecords.at(0)["artifact"];

    REQUIRE(!square_artifact_id.empty());

    std::string getSquareArtifact{"SELECT * FROM artifacts WHERE id = "};

    getSquareArtifact += square_artifact_id;
    getSquareArtifact += ";";

    std::vector<std::map<std::string, std::string>> squareArtifactRecords{};

    rc = sqlite3_exec(database, getSquareArtifact.c_str(), selectCallbackUsingColNameAsKey, &squareArtifactRecords, &errorMessage);

    REQUIRE(squareArtifactRecords.size() == 1);

    std::string path{};
    char        resolvedPath[PATH_MAX];

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

    rc = sqlite3_exec(database, getSquareFields.c_str(), selectCallbackUsingColNameAsKey, &squareFieldsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(squareFieldsRecords.size() == 11);

    // // Enforce order of records by offset
    std::sort(squareFieldsRecords.begin(), squareFieldsRecords.end(), [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
              { return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]); });

    std::string getWidthType{"SELECT * FROM symbols where id="};
    getWidthType += squareFieldsRecords.at(0)["type"];
    getWidthType += ";";

    std::vector<std::map<std::string, std::string>> widthTypeRecords{};

    rc = sqlite3_exec(database, getWidthType.c_str(), selectCallbackUsingColNameAsKey, &widthTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(widthTypeRecords.size() == 1);

    std::string widthType{widthTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(0)["symbol"] == squareRecords.at(0)["id"]);
    REQUIRE(squareFieldsRecords.at(0)["name"] == "width");
    REQUIRE(squareFieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(Square, width)));
    REQUIRE(squareFieldsRecords.at(0)["type"] == widthType);
    REQUIRE(squareFieldsRecords.at(0)["little_endian"] == little_endian);

    std::string getStuffType{"SELECT * FROM symbols where id="};
    getStuffType += squareFieldsRecords.at(1)["type"];
    getStuffType += ";";

    std::vector<std::map<std::string, std::string>> stuffTypeRecords{};

    rc = sqlite3_exec(database, getStuffType.c_str(), selectCallbackUsingColNameAsKey, &stuffTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(stuffTypeRecords.size() == 1);

    REQUIRE(stuffTypeRecords.at(0)["name"] == "uint16_t");
    REQUIRE(stuffTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(uint16_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, stuffTypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_unsigned");
    REQUIRE(stuffTypeRecords.at(0)["short_description"] == "");
    REQUIRE(stuffTypeRecords.at(0)["long_description"] == "");

    std::string stuffType{stuffTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(1)["name"] == "stuff");
    REQUIRE(squareFieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(Square, stuff)));
    REQUIRE(squareFieldsRecords.at(1)["type"] == stuffType);
    REQUIRE(squareFieldsRecords.at(1)["little_endian"] == little_endian);

    std::string getPadding1Type{"SELECT * FROM symbols where id="};
    getPadding1Type += squareFieldsRecords.at(2)["type"];
    getPadding1Type += ";";

    std::vector<std::map<std::string, std::string>> padding1TypeRecords{};

    rc = sqlite3_exec(database, getPadding1Type.c_str(), selectCallbackUsingColNameAsKey, &padding1TypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(padding1TypeRecords.size() == 1);

    REQUIRE(padding1TypeRecords.at(0)["name"] == "uint16_t");
    REQUIRE(padding1TypeRecords.at(0)["byte_size"] == std::to_string(sizeof(uint16_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, padding1TypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_unsigned");
    REQUIRE(padding1TypeRecords.at(0)["short_description"] == "");
    REQUIRE(padding1TypeRecords.at(0)["long_description"] == "");

    std::string padding1Type{padding1TypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(2)["name"] == "padding1");
    REQUIRE(squareFieldsRecords.at(2)["byte_offset"] == std::to_string(offsetof(Square, padding1)));
    REQUIRE(squareFieldsRecords.at(2)["type"] == padding1Type);
    REQUIRE(squareFieldsRecords.at(2)["little_endian"] == little_endian);

    std::string getLengthType{"SELECT * FROM symbols where id="};
    getLengthType += squareFieldsRecords.at(3)["type"];
    getLengthType += ";";

    std::vector<std::map<std::string, std::string>> lengthTypeRecords{};

    rc = sqlite3_exec(database, getLengthType.c_str(), selectCallbackUsingColNameAsKey, &lengthTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(lengthTypeRecords.size() == 1);

    REQUIRE(lengthTypeRecords.at(0)["name"] == "int32_t");
    REQUIRE(lengthTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(int32_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, lengthTypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_signed");
    REQUIRE(lengthTypeRecords.at(0)["short_description"] == "");
    REQUIRE(lengthTypeRecords.at(0)["long_description"] == "");

    std::string lengthType{lengthTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(3)["name"] == "length");
    REQUIRE(squareFieldsRecords.at(3)["byte_offset"] == std::to_string(offsetof(Square, length)));
    REQUIRE(squareFieldsRecords.at(3)["type"] == lengthType);
    REQUIRE(squareFieldsRecords.at(3)["little_endian"] == little_endian);

    std::string getMoreStuffType{"SELECT * FROM symbols where id="};
    getMoreStuffType += squareFieldsRecords.at(4)["type"];
    getMoreStuffType += ";";

    std::vector<std::map<std::string, std::string>> moreStuffTypeRecords{};

    rc = sqlite3_exec(database, getMoreStuffType.c_str(), selectCallbackUsingColNameAsKey, &moreStuffTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(moreStuffTypeRecords.size() == 1);

    REQUIRE(moreStuffTypeRecords.at(0)["name"] == "uint16_t");
    REQUIRE(moreStuffTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(uint16_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, moreStuffTypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_unsigned");
    REQUIRE(moreStuffTypeRecords.at(0)["short_description"] == "");
    REQUIRE(moreStuffTypeRecords.at(0)["long_description"] == "");

    std::string moreStuffType{moreStuffTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(4)["name"] == "more_stuff");
    REQUIRE(squareFieldsRecords.at(4)["byte_offset"] == std::to_string(offsetof(Square, more_stuff)));
    REQUIRE(squareFieldsRecords.at(4)["type"] == moreStuffType);
    REQUIRE(squareFieldsRecords.at(4)["little_endian"] == little_endian);

    std::string getPadding2Type{"SELECT * FROM symbols where id="};
    getPadding2Type += squareFieldsRecords.at(5)["type"];
    getPadding2Type += ";";

    std::vector<std::map<std::string, std::string>> padding2TypeRecords{};

    rc = sqlite3_exec(database, getPadding2Type.c_str(), selectCallbackUsingColNameAsKey, &padding2TypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(padding2TypeRecords.size() == 1);

    REQUIRE(padding2TypeRecords.at(0)["name"] == "uint16_t");
    REQUIRE(padding2TypeRecords.at(0)["byte_size"] == std::to_string(sizeof(uint16_t)));

    REQUIRE(encodingMap.at(followTargetSymbol(database, padding2TypeRecords.at(0)["target_symbol"]).at("encoding")) == "DW_ATE_unsigned");
    REQUIRE(padding2TypeRecords.at(0)["short_description"] == "");
    REQUIRE(padding2TypeRecords.at(0)["long_description"] == "");

    std::string padding2Type{padding2TypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(5)["name"] == "padding2");
    REQUIRE(squareFieldsRecords.at(5)["byte_offset"] == std::to_string(offsetof(Square, padding2)));
    REQUIRE(squareFieldsRecords.at(5)["type"] == padding2Type);
    REQUIRE(squareFieldsRecords.at(5)["little_endian"] == little_endian);

    std::string getFloatingStuffType{"SELECT * FROM symbols where id="};
    getFloatingStuffType += squareFieldsRecords.at(6)["type"];
    getFloatingStuffType += ";";

    std::vector<std::map<std::string, std::string>> floatingStuffTypeRecords{};

    rc = sqlite3_exec(database, getFloatingStuffType.c_str(), selectCallbackUsingColNameAsKey, &floatingStuffTypeRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(floatingStuffTypeRecords.size() == 1);

    REQUIRE(floatingStuffTypeRecords.at(0)["name"] == "float");
    REQUIRE(floatingStuffTypeRecords.at(0)["byte_size"] == std::to_string(sizeof(float)));
    REQUIRE(floatingStuffTypeRecords.at(0)["target_symbol"] == "NULL");

    REQUIRE(encodingMap.at(floatingStuffTypeRecords.at(0)["encoding"]) == "DW_ATE_float");
    REQUIRE(floatingStuffTypeRecords.at(0)["short_description"] == "");
    REQUIRE(floatingStuffTypeRecords.at(0)["long_description"] == "");

    std::string floatingStuffType{floatingStuffTypeRecords.at(0)["id"]};

    REQUIRE(squareFieldsRecords.at(6)["name"] == "floating_stuff");
    REQUIRE(squareFieldsRecords.at(6)["byte_offset"] == std::to_string(offsetof(Square, floating_stuff)));
    REQUIRE(squareFieldsRecords.at(6)["type"] == floatingStuffType);
    REQUIRE(squareFieldsRecords.at(6)["little_endian"] == little_endian);

    // Test matrix3D[2][4][4]
    std::string getMatrix3DDimensionLists{"SELECT * FROM dimension_lists WHERE field_id="};
    getMatrix3DDimensionLists += squareFieldsRecords.at(7)["id"];
    getMatrix3DDimensionLists += ";";

    std::vector<std::map<std::string, std::string>> matrix3DDimensionListsRecords{};

    rc = sqlite3_exec(database, getMatrix3DDimensionLists.c_str(), selectCallbackUsingColNameAsKey, &matrix3DDimensionListsRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(matrix3DDimensionListsRecords.size() == 3);

    // Enforce order of records by dim_order
    std::sort(matrix3DDimensionListsRecords.begin(), matrix3DDimensionListsRecords.end(),
              [](std::map<std::string, std::string> a, std::map<std::string, std::string> b) { return std::stoi(a["dim_order"]) < std::stoi(b["dim_order"]); });

    REQUIRE(matrix3DDimensionListsRecords.at(0)["field_id"] == squareFieldsRecords.at(7)["id"]);
    REQUIRE(matrix3DDimensionListsRecords.at(0)["dim_order"] == "0");
    REQUIRE(matrix3DDimensionListsRecords.at(0)["upper_bound"] == "1");

    REQUIRE(matrix3DDimensionListsRecords.at(1)["field_id"] == squareFieldsRecords.at(7)["id"]);
    REQUIRE(matrix3DDimensionListsRecords.at(1)["dim_order"] == "1");
    REQUIRE(matrix3DDimensionListsRecords.at(1)["upper_bound"] == "3");

    REQUIRE(matrix3DDimensionListsRecords.at(2)["field_id"] == squareFieldsRecords.at(7)["id"]);
    REQUIRE(matrix3DDimensionListsRecords.at(2)["dim_order"] == "2");
    REQUIRE(matrix3DDimensionListsRecords.at(2)["upper_bound"] == "3");

    // Test matrix3D[2][4][4]
    std::string getMatrix1DDimensionLists{"SELECT * FROM dimension_lists WHERE field_id="};
    getMatrix1DDimensionLists += squareFieldsRecords.at(8)["id"];
    getMatrix1DDimensionLists += ";";

    std::vector<std::map<std::string, std::string>> matrix1DDimensionListsRecords{};

    rc = sqlite3_exec(database, getMatrix1DDimensionLists.c_str(), selectCallbackUsingColNameAsKey, &matrix1DDimensionListsRecords, &errorMessage);
    REQUIRE(rc == SQLITE_OK);
    REQUIRE(matrix1DDimensionListsRecords.size() == 1);

    // Enforce order of records by dim_order
    std::sort(matrix1DDimensionListsRecords.begin(), matrix1DDimensionListsRecords.end(),
              [](std::map<std::string, std::string> a, std::map<std::string, std::string> b) { return std::stoi(a["dim_order"]) < std::stoi(b["dim_order"]); });

    REQUIRE(matrix1DDimensionListsRecords.at(0)["field_id"] == squareFieldsRecords.at(8)["id"]);
    REQUIRE(matrix1DDimensionListsRecords.at(0)["dim_order"] == "0");
    REQUIRE(matrix1DDimensionListsRecords.at(0)["upper_bound"] == "1");

    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}

TEST_CASE("Write Elf File to database with verbosity set to DEBUG", "[main_test#11]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{LOGGER_VERBOSITY_DEBUG};

    std::string     inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);
    juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    delete idc;
    REQUIRE(remove("./test_db.sqlite") == 0);
}
TEST_CASE("Write Elf File to database with verbosity set to WARNINGS", "[main_test#12]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{LOGGER_VERBOSITY_WARNINGS};

    std::string     inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);
    juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    delete idc;
    REQUIRE(remove("./test_db.sqlite") == 0);
}
TEST_CASE("Write Elf File to database with verbosity set to ERRORS", "[main_test#13]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{LOGGER_VERBOSITY_ERRORS};

    std::string     inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);
    juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    delete idc;
    REQUIRE(remove("./test_db.sqlite") == 0);
}
TEST_CASE("Write Elf File to database with verbosity set to SILENT", "[main_test#14]")
{
    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger{LOGGER_VERBOSITY_SILENT};

    std::string     inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);
    juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();
    delete idc;
    REQUIRE(remove("./test_db.sqlite") == 0);
}

TEST_CASE("Write Elf File to database with IDC set to IDC_TYPE_CCDD.", "[main_test#15]")
{
    IDataContainer* idc = 0;

    idc                 = IDataContainer::Create(IDC_TYPE_CCDD, "./test_db.sqlite");
    REQUIRE(idc == nullptr);

    delete idc;
}

TEST_CASE("Write Elf File to database with IDC set to an invalid value.", "[main_test#16]")
{
    IDataContainer* idc = 0;

    idc                 = IDataContainer::Create((IDataContainer_Type_t)100, "./test_db.sqlite");
    REQUIRE(idc == nullptr);

    delete idc;
}

TEST_CASE("Test the correctness of define macros.", "[main_test#17]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc            = 0;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    std::string getMacroQuery{"SELECT * FROM macros WHERE name = \"CFE_MISSION_ES_PERF_MAX_IDS\"; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3* database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::vector<std::map<std::string, std::string>> macroRecords{};

    rc = sqlite3_exec(database, getMacroQuery.c_str(), selectCallbackUsingColNameAsKey, &macroRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(macroRecords.size() == 1);

    uint32_t numberOfColumns = 0;

    for (auto pair : macroRecords.at(0))
    {
        numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 7);

    REQUIRE(macroRecords.at(0).at("name") == "CFE_MISSION_ES_PERF_MAX_IDS");
    REQUIRE(macroRecords.at(0).at("value") == "128");

    /**
     * Check the correctness of macro.
     */

    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}



TEST_CASE("Test the correctness of define macros across multiple groups.", "[main_test#18]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc            = 0;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_4};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    // Group5 has macros "MAC4", "MAC5"
    // Some more details about how this happens:https://lists.dwarfstd.org/pipermail/dwarf-discuss/2024-September/thread.html#2505
    juicer.setGroupNumber(5);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    std::string getMacroQuery{"SELECT * FROM macros; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3* database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::vector<std::map<std::string, std::string>> macroRecords{};

    rc = sqlite3_exec(database, getMacroQuery.c_str(), selectCallbackUsingColNameAsKey, &macroRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(macroRecords.size() == 5);

    uint32_t numberOfColumns = 0;

    for (auto pair : macroRecords.at(0))
    {
        numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 7);

    REQUIRE(macroRecords.at(0).at("name") == "MAC1");
    REQUIRE(macroRecords.at(0).at("value") == "2");
    
    REQUIRE(macroRecords.at(1).at("name") == "MAC2");
    REQUIRE(macroRecords.at(1).at("value") == "3");
    
    REQUIRE(macroRecords.at(2).at("name") == "MAC3");
    REQUIRE(macroRecords.at(2).at("value") == "4");
    
    REQUIRE(macroRecords.at(3).at("name") == "MAC4");
    REQUIRE(macroRecords.at(3).at("value") == "");
    
    REQUIRE(macroRecords.at(4).at("name") == "MAC5");
    REQUIRE(macroRecords.at(4).at("value") == "1");


    /**
     * Check the correctness of macro.
     */

    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}



TEST_CASE("Test the correctness of artifacts.", "[main_test#19]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc            = 0;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    REQUIRE(rc == JUICER_OK);

    std::string getArtifactQuery{"SELECT *  FROM artifacts where path= \"/usr/include/x86_64-linux-gnu/bits/types.h\" ; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3* database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::vector<std::map<std::string, std::string>> artiffactRecords{};

    rc = sqlite3_exec(database, getArtifactQuery.c_str(), selectCallbackUsingColNameAsKey, &artiffactRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(artiffactRecords.size() == 1);

    uint32_t numberOfColumns = 0;

    for (auto pair : artiffactRecords.at(0))
    {
        numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 4);


    REQUIRE(artiffactRecords.at(0).find("id") != artiffactRecords.at(0).end());
    REQUIRE(artiffactRecords.at(0).find("elf") != artiffactRecords.at(0).end());
    REQUIRE(artiffactRecords.at(0).find("path") != artiffactRecords.at(0).end());
    REQUIRE(artiffactRecords.at(0).find("md5") != artiffactRecords.at(0).end());


    REQUIRE(artiffactRecords.at(0).at("path") == "/usr/include/x86_64-linux-gnu/bits/types.h");


    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}





TEST_CASE("Test the correctness of bit fields.", "[main_test#20]")
{
    /**
     * This assumes that the test_file was compiled on
     * gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0 or   gcc (Ubuntu 9.4.0-1ubuntu1~20.04.2) 9.4.0
     *  little-endian machine.
     */

    Juicer          juicer;
    IDataContainer* idc = 0;
    Logger          logger;
    int             rc            = 0;
    char*           errorMessage  = nullptr;
    std::string     little_endian = is_little_endian() ? "1" : "0";

    logger.logWarning("This is just a test.");
    std::string inputFile{TEST_FILE_1};

    idc = IDataContainer::Create(IDC_TYPE_SQLITE, "./test_db.sqlite");
    REQUIRE(idc != nullptr);
    logger.logInfo("IDataContainer was constructed successfully for unit test.");

    juicer.setIDC(idc);

    rc = juicer.parse(inputFile);

    REQUIRE((juicer.getDwarfVersion() == 4 || juicer.getDwarfVersion() == 5));

    REQUIRE(rc == JUICER_OK);

    REQUIRE(rc == JUICER_OK);

    std::string getSymbolQuery{"SELECT *  FROM symbols where name= \"S\" ; "};

    /**
     *Clean up our database handle and objects in memory.
     */
    ((SQLiteDB*)(idc))->close();

    sqlite3* database;

    rc = sqlite3_open("./test_db.sqlite", &database);

    REQUIRE(rc == SQLITE_OK);

    std::vector<std::map<std::string, std::string>> symbolRecords{};

    rc = sqlite3_exec(database, getSymbolQuery.c_str(), selectCallbackUsingColNameAsKey, &symbolRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    REQUIRE(symbolRecords.size() == 1);

    uint32_t numberOfColumns = 0;

    for (auto pair : symbolRecords.at(0))
    {
        numberOfColumns++;
    }

    REQUIRE(numberOfColumns == 9);


    REQUIRE(symbolRecords.at(0).at("byte_size") == std::to_string(sizeof(S)));



     /**
     *Check the fields of the S struct.
     */

    std::string sId = symbolRecords.at(0)["id"];

    std::string getSFields{"SELECT * FROM fields WHERE symbol = "};

    getSFields += sId;
    getSFields += ";";

    std::vector<std::map<std::string, std::string>> fieldsRecords{};

    rc = sqlite3_exec(database, getSFields.c_str(), selectCallbackUsingColNameAsKey, &fieldsRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);
    
    // TODO:Incosistent across Ubuntu20 and Ubuntu22. Different compilers will have different padding schemes.
    REQUIRE(fieldsRecords.size() >= 5);

    // Enforce order of records by offset
    std::sort(fieldsRecords.begin(), fieldsRecords.end(), [](std::map<std::string, std::string> a, std::map<std::string, std::string> b)
              { return std::stoi(a["byte_offset"]) < std::stoi(b["byte_offset"]); });

    /**
     * Ensure that we have all of the expected keys in our map; these are the column names.
     * Don't love doing this kind of thing in tests...
     */
    for (auto record : fieldsRecords)
    {
        REQUIRE(record.find("symbol") != record.end());
        REQUIRE(record.find("name") != record.end());
        REQUIRE(record.find("byte_offset") != record.end());
        REQUIRE(record.find("type") != record.end());

        REQUIRE(record.find("little_endian") != record.end());
        REQUIRE(record.find("bit_size") != record.end());
        REQUIRE(record.find("bit_offset") != record.end());
        REQUIRE(record.find("short_description") != record.end());
        REQUIRE(record.find("long_description") != record.end());
    }

    REQUIRE(fieldsRecords.at(0)["name"] == "before");
    /**
     *Check the correctness of the fields
     */

    std::string getBeforeType{"SELECT * FROM symbols where id="};

    getBeforeType += fieldsRecords.at(0)["type"];
    getBeforeType += ";";

    std::vector<std::map<std::string, std::string>> beforeSymbolRecords{};

    rc = sqlite3_exec(database, getBeforeType.c_str(), selectCallbackUsingColNameAsKey, &beforeSymbolRecords, &errorMessage);

    REQUIRE(rc == SQLITE_OK);

    REQUIRE(beforeSymbolRecords.size() == 1);

    std::string beforeType{beforeSymbolRecords.at(0).at("id")};

    REQUIRE(fieldsRecords.at(0)["symbol"] == symbolRecords.at(0)["id"]);
    REQUIRE(fieldsRecords.at(0)["name"] == "before");
    REQUIRE(fieldsRecords.at(0)["byte_offset"] == std::to_string(offsetof(S, before)));
    REQUIRE(fieldsRecords.at(0)["type"] == beforeType);
    REQUIRE(fieldsRecords.at(0)["little_endian"] == little_endian);
    REQUIRE(fieldsRecords.at(0)["bit_size"] == "0");
    REQUIRE(fieldsRecords.at(0)["bit_offset"] == "0");
    REQUIRE(fieldsRecords.at(0)["short_description"] == "");
    REQUIRE(fieldsRecords.at(0)["long_description"] == "");

    // TODO:Inconsistent across Ubuntu20 and Ubuntu22. Different compilers will have different padding schemes.

    // REQUIRE(fieldsRecords.at(1)["name"] == "j");
    // /**
    //  *Check the correctness of the fields
    //  */

    // std::string getFieldType{"SELECT * FROM symbols where id="};

    // getFieldType += fieldsRecords.at(1)["type"];
    // getFieldType += ";";

    // std::vector<std::map<std::string, std::string>> fieldSymbolRecords{};

    // rc = sqlite3_exec(database, getFieldType.c_str(), selectCallbackUsingColNameAsKey, &fieldSymbolRecords, &errorMessage);

    // REQUIRE(rc == SQLITE_OK);

    // REQUIRE(fieldSymbolRecords.size() == 1);

    // std::string fieldType{fieldSymbolRecords.at(0).at("id")};

    // REQUIRE(fieldsRecords.at(1)["symbol"] == symbolRecords.at(0)["id"]);
    // REQUIRE(fieldsRecords.at(1)["name"] == "j");
    // // REQUIRE(fieldsRecords.at(1)["byte_offset"] == std::to_string(offsetof(S, j)));
    // REQUIRE(fieldsRecords.at(1)["type"] == fieldType);
    // REQUIRE(fieldsRecords.at(1)["little_endian"] == little_endian);
    // REQUIRE(fieldsRecords.at(1)["bit_size"] == "5");
    // REQUIRE(fieldsRecords.at(1)["bit_offset"] == "19");
    // REQUIRE(fieldsRecords.at(1)["short_description"] == "");
    // REQUIRE(fieldsRecords.at(1)["long_description"] == "");






    REQUIRE(remove("./test_db.sqlite") == 0);
    delete idc;
}