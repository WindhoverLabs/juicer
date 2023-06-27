/*
 * SQLiteStructure.h
 *
 *  Created on: Aug 3, 2020
 *      Author: lgomez
 *      email:lgomez@windhoverlabs.com
 *
 * The sql schema defined in this file is modeled after DWARF4.
 * For more info on DWARF4:
 */

#ifndef SQLITEDB_H_
#define SQLITEDB_H_

#include "IDataContainer.h"
#include "Symbol.h"
#include "Enumeration.h"
#include "ElfFile.h"
#include <sqlite3.h>
#include <map>
#include <string>

#define SQLITEDB_ERROR -1
#define SQLITEDB_OK 0


#define CREATE_ELF_TABLE         "CREATE TABLE IF NOT EXISTS elfs (\
                                  id INTEGER PRIMARY KEY,\
                                  name TEXT UNIQUE NOT NULL,\
                                  md5 TEXT NOT NULL,\
                                  date DATETIME NOT NULL DEFAULT(CURRENT_TIMESTAMP),\
                                  little_endian BOOLEAN NOT NULL);"

#define CREATE_SYMBOL_TABLE      "CREATE TABLE IF NOT EXISTS symbols(\
                                  id INTEGER PRIMARY KEY,\
                                  elf INTEGER NOT NULL,\
                                  name TEXT UNIQUE NOT NULL,\
                                  byte_size INTEGER NOT NULL,\
								  artifact INTEGER,\
                                  FOREIGN KEY(elf) REFERENCES elfs(id),\
								  FOREIGN KEY(artifact) REFERENCES artifacts(id)\
                                  UNIQUE(name));"

#define CREATE_DIMENSION_TABLE   "CREATE TABLE IF NOT EXISTS dimension_lists (\
                                  id INTEGER PRIMARY KEY,\
								  field_id INTEGER NOT NULL,\
                                  dim_order INTEGER NOT NULL,\
                                  upper_bound INTEGER NOT NULL,\
								  FOREIGN KEY (field_id) REFERENCES fields(id),\
								  UNIQUE(field_id, dim_order, upper_bound));"

#define CREATE_FIELD_TABLE       "CREATE TABLE IF NOT EXISTS fields(\
                                  id INTEGER PRIMARY KEY,\
                                  symbol INTEGER NOT NULL,\
                                  name TEXT NOT NULL,\
                                  byte_offset INTEGER NOT NULL,\
                                  type INTEGER NOT NULL,\
                                  little_endian BOOLEAN,\
								  bit_size INTEGER NOT NULL,\
								  bit_offset INTEGER NOT NULL,\
                                  FOREIGN KEY (symbol) REFERENCES symbols(id),\
                                  FOREIGN KEY (type) REFERENCES symbols(id),\
                                  UNIQUE (symbol, name));"

#define CREATE_ENUMERATION_TABLE "CREATE TABLE IF NOT EXISTS enumerations(\
                                  id INTEGER PRIMARY KEY,\
                                  symbol INTEGER NOT NULL,\
                                  value INTEGER NOT NULL,\
                                  name TEXT NOT NULL,\
                                  FOREIGN KEY (symbol) REFERENCES symbols(id),\
                                  UNIQUE (symbol, name));"

#define CREATE_ARTIFACTS_TABLE   "CREATE TABLE IF NOT EXISTS artifacts(\
                                  id INTEGER PRIMARY KEY,\
                                  elf INTEGER NOT NULL,\
                                  path TEXT NOT NULL,\
								  md5 TEXT NOT NULL, \
                                  FOREIGN KEY (elf) REFERENCES elfs(id),\
                                  UNIQUE (path, md5));"

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
    std::vector<Symbol> symbols{};
    int openDatabase(std::string &databaseName);
    int createElfSchema(void);
    int createSymbolSchema(void);
    int createSchemas(void);
    int createFieldsSchema(void);
    int createDimensionsSchema(void);
    int createEnumerationSchema(void);
    int createArtifactsSchema(void);
    int writeElfToDatabase(ElfFile& inModule);
    int writeArtifactsToDatabase(ElfFile& inModule);
    int writeSymbolsToDatabase(ElfFile& inModule);
    int writeFieldsToDatabase(ElfFile& inModule);
    int writeEnumerationsToDatabase(ElfFile& inModule);
    int writeDimensionsListToDatabase(ElfFile& inElf);
    static int doesRowExistCallback(void *veryUsed, int argc, char **argv, char **azColName);
    bool doesSymbolExist(std::string name);
    bool doesArtifactExist(std::string name);

public:
    SQLiteDB();
    int initialize(std::string &initString);
    static int selectCallback(void *veryUsed, int argc, char **argv, char **azColName);
    int close(void);
    virtual int write(ElfFile& inModule);
    virtual
    ~SQLiteDB();
};

#endif /* SQLITEDB_H_ */
