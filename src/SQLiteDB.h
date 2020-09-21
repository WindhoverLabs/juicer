/*
 * SQLiteStructure.h
 *
 *  Created on: Aug 3, 2020
 *      Author: vagrant
 */

#ifndef SQLITEDB_H_
#define SQLITEDB_H_

#include "IDataContainer.h"
#include "Symbol.h"
#include "ElfFile.h"
#include "BitField.h"
#include "Enumeration.h"
#include <sqlite3.h>
#include "ElfObj.h"

#define SQLITEDB_ERROR -1
#define SQLITEDB_OK 0


#define CREATE_ELF_TABLE         "CREATE TABLE IF NOT EXISTS elfs (\
                                  id INTEGER PRIMARY KEY,\
                                  name TEXT UNIQUE NOT NULL,\
                                  checksum TEXT NOT NULL,\
                                  date DATETIME NOT NULL DEFAULT(CURRENT_TIMESTAMP),\
                                  little_endian BOOLEAN NOT NULL);"

#define CREATE_SYMBOL_TABLE      "CREATE TABLE IF NOT EXISTS symbols(\
                                  id INTEGER PRIMARY KEY,\
                                  elf INTEGER NOT NULL,\
                                  name TEXT UNIQUE NOT NULL,\
                                  byte_size INTEGER NOT NULL,\
                                  FOREIGN KEY(elf) REFERENCES elfs(id)\
                                  UNIQUE(name));"

#define CREATE_FIELD_TABLE       "CREATE TABLE IF NOT EXISTS fields(\
                                  id INTEGER PRIMARY KEY,\
                                  symbol INTEGER NOT NULL,\
                                  name TEXT NOT NULL,\
                                  byte_offset INTEGER NOT NULL,\
                                  type INTEGER,\
                                  multiplicity INTEGER NOT NULL,\
                                  little_endian BOOLEAN,\
                                  FOREIGN KEY (symbol) REFERENCES symbols(id),\
                                  FOREIGN KEY (type) REFERENCES symbols(id),\
                                  UNIQUE (symbol, name));"

#define CREATE_BITFIELD_TABLE    "CREATE TABLE IF NOT EXISTS bit_fields(\
                                  field INTEGER PRIMARY KEY,\
                                  bit_size INTEGER NOT NULL,\
                                  bit_offset INTEGER NOT NULL,\
                                  FOREIGN KEY (field) REFERENCES fields(id)\
                                  );"

#define CREATE_ENUMERATION_TABLE "CREATE TABLE IF NOT EXISTS enumerations(\
                                  symbol INTEGER NOT NULL,\
                                  value INTEGER NOT NULL,\
                                  name TEXT NOT NULL,\
                                  FOREIGN KEY (symbol) REFERENCES symbols(id),\
                                  PRIMARY KEY (symbol, value)\
                                  );"

#define SQLiteDB_TRUE 1
#define SQLiteDB_FALSE 0

/**
 *@brief classSQLiteStructure's goal is to have all the utilities
 *to write data such as endianness about ELF files to
 *a SQLite database.
 */
class SQLiteDB: public IDataContainer
{

private:
    sqlite3 *database;
    Logger logger;
    int openDatabase(std::string &databaseName);
    int createElfSchema(void);
    int createSymbolSchema(void);
    int createSchemas(void);
    int createFiledSchema(void);
    int createBitFiledSchema(void);
    int createEnumerationSchema(void);
    int writeElfToDatabase(ElfObj& inModule);
    int writeSymbolsToDatabase(ElfObj& inModule);
    int writeFieldsToDatabase(ElfObj& inModule);
    int writeBitFieldsToDatabase(ElfObj& inModule);
    int writeEnumerationsToDatabase(ElfObj& inModule);

public:
    SQLiteDB();
    int initialize(std::string &initString);
    int close(void);
    virtual int write(ElfObj& inModule);
    virtual
    ~SQLiteDB();
};

#endif /* SQLITEDB_H_ */
