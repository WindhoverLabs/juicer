/*
 * SQLiteStructure.cpp
 *
 *  Created on: Aug 3, 2020
 *      Author: vagrant
 */

#include<string>
#include "SQLiteDB.h"

SQLiteDB::SQLiteDB() :
        database(0)
{
}

SQLiteDB::~SQLiteDB()
{
}



/**
 *@brief Opens the database at initString and creates the necessary schemas to
 *store ELF and DWARF data.
 *
 *@param initString the file path at which the database is located at.
 *
 *@return Returns SQLITE_OK if the data database was opened successfully
 *and schemas were created successfully as well.
 */
int SQLiteDB::initialize(std::string &initString)
{
    int rc = SQLITE_OK;

    /* Parse the initialization string and pull out whatever parameters we need
     * to initialize.  For now, this is literally just the file name so we
     * can just use the string directly.
     */
    rc = openDatabase(initString);

    if(SQLITE_OK == rc)
    {
        createSchemas();
    }
    else
    {
        rc = SQLITEDB_ERROR;
    }

    return rc;
}



/**
 *@brief Attempts to shutdown the database.
 *
 *@return Returns SQLITEDB_OK if it was able to close the database. Otherwise,
 *it returns SQLITEDB_ERROR if an error occurs.
 *
 *@note According to the SQLite3 docs, it seems that when
 *closing(with sqlite3_close) and fails to close, it (most of the time)
 *might have to do with some kind of unfinished transaction.
 *Visit https://sqlite.org/c3ref/close.html for more details.
 */
int SQLiteDB::close(void)
{
    int rc = SQLITEDB_OK;

    rc = sqlite3_close(database);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("The database was closed successfully.");
    }
    else
    {
        logger.logError("There was an error while closing the database.");
        rc = SQLITEDB_ERROR;
    }

    return rc;
}



/**
 *@brief Establish a new connection to the database at databaseName.
 *If the file does not yet exist, this will create it.
 *
 *@return Returns SQLITE_OK if it is able to open the database successfully.
 *If opening the database fails, then SQLITESTRUCTURE_ERROR is returned.
 */
int SQLiteDB::openDatabase(std::string &fileName)
{
    int rc = sqlite3_open(fileName.c_str(), &database);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Created the database with OK status");
    }
    else
    {
        rc = SQLITEDB_ERROR;
        logger.logError("Failed to create the database with SQLITEDB_ERROR status.",
                        sqlite3_errmsg(database));
    }

    return rc;
}



/**
 *@brief Writes all the data such as Elf, Symbols and Modules entries
 *to the SQLite database.
 *
 *@param inModule The module that contains all of the DWARF and ELF data.
 *
 *@return SQLITE_OK if it was able to write the all of data to the database
 *successfully. Otherwise, this method returns SQLITEDB_ERROR.
 */
int SQLiteDB::write(Module& inModule)
{
    int rc  = SQLITEDB_OK;

    rc = writeModuleToDatabase(inModule);


    /**
     *@note I'm not 100% sure about nesting if statements
     *because it gets a bit out of control in cases like these.
     *I know this is a tool that will always be running,
     *but is there a possibility of considering exceptions here?
     *I know exceptions aren't great, but my thinking
     *is that this write function is "atomic" in a way.
     *What I mean by that is that ALL queries must be successful in order for
     *the call to be valid. Maybe there is something I'm missing, but just
     *thought I'd propose the possibility of exceptions in cases like this.
     */
    if(SQLITE_OK == rc)
    {
        logger.logDebug("Module entries were written to the modules schema with"
                        " SQLITE_OK status.");
        rc = writeElfsToDatabase(inModule);

        if(SQLITE_OK == rc)
        {
            logger.logDebug("Elf entries were written to the elfs schema with SQLITE_OK "
                            "status.");
            rc = writeSymbolsToDatabase(inModule);

            if(SQLITE_OK == rc)
            {
                logger.logDebug("Symbol entries were written to the symbols schema "
                                "with SQLITE_OK status.");

                rc = writeFieldsToDatabase(inModule);

                if(SQLITE_OK == rc)
                {
                    logger.logDebug("Field entries were written to the fields schema "
                                    "with SQLITE_OK status.");

                    rc = writeBitFieldsToDatabase(inModule);

                    if(SQLITE_OK == rc)
                    {
                        logger.logDebug("Bitfield entries were written to the bit_fields schema "
                                        "with SQLITE_OK status.");

                        rc = writeEnumerationsToDatabase(inModule);

                        if(SQLITE_OK == rc)
                        {
                            logger.logDebug("Enumeration entries were written to the enumerations schema "
                                            "with SQLITE_OK status.");
                        }
                        else
                        {
                            logger.logDebug("There was an error while writing enumeration entries to the"
                                            " database.");

                            rc = SQLITEDB_ERROR;
                        }
                    }
                    else
                    {
                        logger.logDebug("There was an error while writing bit_field entries to the"
                                        " database.");

                        rc = SQLITEDB_ERROR;
                    }
                }
                else
                {
                    logger.logDebug("There was an error while writing field entries to the"
                                    " database.");

                    rc = SQLITEDB_ERROR;
                }
            }
            else
            {
                logger.logDebug("There was an error while writing symbol entries to the"
                                " database.");

                rc = SQLITEDB_ERROR;
            }
        }
        else
        {
            logger.logDebug("There was an error while writing elf entries to the database.");

            rc = SQLITEDB_ERROR;
        }

    }
    else
    {
        logger.logDebug("There was an error while writing module entries to "
                        "the database.");

        rc = SQLITEDB_ERROR;
    }

    return rc;
}


/**
 *@brief Writes the data in inModule to the 'modules'
 *table in the database. Please note that this method also writes the
 *new row id generated by SQLite to the inModule object.
 *
 *@param inModule the Module object to be written to the database.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *elf entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeModuleToDatabase(Module& inModule)
{
    std::string writeQuery{};
    int         rc = 0;
    char*       errorMessage = NULL;

    /*@todo I'm not sure about using std::string for queries because I'm
     *not sure if it is as readable or less readable than using something like snprintf().*/
    writeQuery += "INSERT INTO modules (name) VALUES (\"";
    writeQuery += inModule.getName();
    writeQuery += "\");";

    rc = sqlite3_exec(database, writeQuery.c_str(), NULL, NULL, &errorMessage);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Module values were written to the modules schema with "
                        "SQLITE_OK status.");

        /*Write the id to this module so that other tables can use it as
         *a foreign key */

        sqlite3_int64 lastRowId = sqlite3_last_insert_rowid(database);
        inModule.setId(lastRowId);
    }
    else
    {
        logger.logDebug("There was an error while writing data to the modules table.");
        logger.logDebug("%s.", errorMessage);

        rc = SQLITEDB_ERROR;
    }

    return rc;;
}



/**
 *@brief Iterates through all of the ELF entries in
 *inModule and writes each one to the "elfs" table.
 *
 *@param inModule The module that has the Elf data.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *elf entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeElfsToDatabase(Module& inModule)
{
   int      rc  = SQLITEDB_OK;
   char* errorMessage = NULL;

   for(auto&& elf : inModule.getElfFiles())
   {
       /*
        * @todo I want to store these SQLite magical values into MACROS,
        * but I'm not sure what is the best way to do that without it being
        * messy.
        */
       std::string writeElfQuery{};

       writeElfQuery += "INSERT INTO elfs(name, module, checksum, little_endian) "
                        "VALUES(\"";
       writeElfQuery += elf->getName();
       writeElfQuery += "\",";
       writeElfQuery += std::to_string(elf->getModule().getId());
       writeElfQuery += ",";
       writeElfQuery += std::to_string(elf->getChecksum());
       writeElfQuery += ",";
       writeElfQuery += std::to_string(elf->isLittleEndian()? SQLiteDB_TRUE: SQLiteDB_FALSE);
       writeElfQuery += ");";

       logger.logDebug("Sending \"%s\" query to database.",writeElfQuery.c_str());

       rc = sqlite3_exec(database, writeElfQuery.c_str(), NULL, NULL,
                         &errorMessage);

       if(SQLITE_OK == rc)
       {
           logger.logDebug("Elf values were written to the elfs schema with "
                           "SQLITE_OK status.");
       }
       else
       {
           logger.logDebug("There was an error while writing data to the elfs table.");
           logger.logDebug("%s.", errorMessage);

           rc = SQLITEDB_ERROR;
       }
   }

   return rc;
}



/**
 *@brief Iterates through all of the symbols entries in
 *inModule and writes each one to the "symbols" table.
 *
 *
 *@return Returns SQLITEDB_OK if all of the symbols entries are written to the
 *database successfully. If the method fails to write at least one of the
 *symbol entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeSymbolsToDatabase(Module& inModule)
{
    int         rc  = SQLITEDB_OK;
    char*       errorMessage = NULL;

    /**
     * @note Are we allowed for ground tools to do this for loops?
     * I know for Flight Software we need to explicitly state the "++i",
     * but should/can we do this here with loops for Juicer?
     */
    for(auto&& symbol : inModule.getSymbols())
    {
        /*
         * @todo I want to store these SQLite magical values into MACROS,
         * but I'm not sure what is the best way to do that without it being
         * messy.
         */
        std::string writeSymbolQuery{};

        writeSymbolQuery += "INSERT INTO symbols(module, name, byte_size) "
                             "VALUES(";
        writeSymbolQuery += std::to_string(symbol->getModule().getId());
        writeSymbolQuery += ",\"";
        writeSymbolQuery += symbol->getName();
        writeSymbolQuery += "\",";
        writeSymbolQuery += std::to_string(symbol->getByteSize());
        writeSymbolQuery += ")";

        rc = sqlite3_exec(database, writeSymbolQuery.c_str(), NULL, NULL,
                          &errorMessage);

        if(SQLITE_OK == rc)
        {
            logger.logDebug("Symbol values were written to the symbols schema with "
                            "SQLITE_OK status.");

            /*Write the id to this symbol so that other tables can use it as
             *a foreign key */
            sqlite3_int64 lastRowId = sqlite3_last_insert_rowid(database);
            symbol->setId(lastRowId);
        }
        else
        {
            logger.logDebug("There was an error while data to the symbols table.");
            logger.logDebug("%s.", errorMessage);

            rc = SQLITEDB_ERROR;
        }
    }

    return rc;

}



/**
 *@brief Iterates through all of the field entries in
 *inModule and writes each one to the "fields" table.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *field entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeFieldsToDatabase(Module& inModule)
{
    int         rc  = SQLITEDB_OK;
    char*       errorMessage = NULL;

    /**
     * @note Are we allowed for ground tools to do this for loops?
     * I know for Flight Software we need to explicitly state the "++i",
     * but should/can we do this here with loops for Juicer?
     */
    for(auto field : inModule.getFields())
    {
        /*
         * @todo I want to store these SQLite magical values into MACROS,
         * but I'm not sure what is the best way to do that without it being
         * messy.
         */
        std::string writeFieldQuery{};

        writeFieldQuery += "INSERT INTO fields(symbol, name, byte_offset, type, "
                            "multiplicity, little_endian) VALUES(";
        writeFieldQuery += std::to_string(field->getSymbol().getId());
        writeFieldQuery += ",";
        writeFieldQuery += "\"";
        writeFieldQuery += field->getName();
        writeFieldQuery += "\"";
        writeFieldQuery += ",";
        writeFieldQuery += std::to_string(field->getByteOffset());
        writeFieldQuery += ",";
        writeFieldQuery += std::to_string(field->getType().getId());
        writeFieldQuery += ",";
        writeFieldQuery += std::to_string(field->getMultiplicity());
        writeFieldQuery += ",";
        writeFieldQuery += std::to_string(field->isLittleEndian()?
                                          SQLiteDB_TRUE: SQLiteDB_FALSE);
        writeFieldQuery += ");";

        rc = sqlite3_exec(database, writeFieldQuery.c_str(), NULL, NULL,
                          &errorMessage);

        if(SQLITE_OK == rc)
        {
            logger.logDebug("Field values were written to the fields schema with "
                            "SQLITE_OK status.");

            /*Write the id to this field so that other tables can use it as
             *a foreign key */
            sqlite3_int64 lastRowId = sqlite3_last_insert_rowid(database);
            field->setId(lastRowId);


        }
        else
        {
            logger.logDebug("There was an error while data to the fields table.");
            logger.logDebug("%s.", errorMessage);

            rc = SQLITEDB_ERROR;
        }
    }

    return rc;

}



/**
 *@brief Iterates through all of the bit_field entries in
 *inModule and writes each one to the "bit_fields" table.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *bit_field entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeBitFieldsToDatabase(Module& inModule)
{
    int   rc = SQLITEDB_OK;
    char* errorMessage = NULL;

    /**
     * @note Are we allowed for ground tools to do this for loops?
     * I know for Flight Software we need to explicitly state the "++i",
     * but should/can we do this here with loops for Juicer?
     */
    for(auto bitField : inModule.getBitFields())
    {
        /*
         * @todo I want to store these SQLite magical values into MACROS,
         * but I'm not sure what is the best way to do that without it being
         * messy.
         */
        std::string writeBitFieldQuery{};

        writeBitFieldQuery += "INSERT INTO bit_fields(field, bit_field, bit_offset)"
                              "VALUES(";
        writeBitFieldQuery += std::to_string(bitField->getField().getId());
        writeBitFieldQuery += ",";
        writeBitFieldQuery += std::to_string(bitField->getBitSize());
        writeBitFieldQuery += ",";
        writeBitFieldQuery += std::to_string(bitField->getBitOffset());
        writeBitFieldQuery += ");";

        rc = sqlite3_exec(database, writeBitFieldQuery.c_str(), NULL, NULL,
                          &errorMessage);

        if(SQLITE_OK == rc)
        {
            logger.logDebug("BitField values were written to the bit_fields schema with "
                            "SQLITE_OK status.");
        }
        else
        {
            logger.logDebug("There was an error while data to the bit_fields table.");
            logger.logDebug("%s.", errorMessage);

            rc = SQLITEDB_ERROR;
        }
    }

    return rc;

}



/**
 *@brief Iterates through all of the enumeration entries in
 *inModule and writes each one to the "enumerations" table.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *enumeration entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeEnumerationsToDatabase(Module& inModule)
{
    int   rc = SQLITEDB_OK;
    char* errorMessage = NULL;

    /**
     * @note Are we allowed for ground tools to do this for loops?
     * I know for Flight Software we need to explicitly state the "++i",
     * but should/can we do this here with loops for Juicer?
     */
    for(auto enumeration : inModule.getEnumerations())
    {
        /*
         * @todo I want to store these SQLite magical values into MACROS,
         * but I'm not sure what is the best way to do that without it being
         * messy.
         */
        std::string writeEnumerationQuery{};

        writeEnumerationQuery += "INSERT INTO enumerations(symbol, value, name)"
                                 "VALUES(";
        writeEnumerationQuery += std::to_string(enumeration->getSymbol().getId());
        writeEnumerationQuery += ",";
        writeEnumerationQuery += std::to_string(enumeration->getValue());
        writeEnumerationQuery += ",\"";
        writeEnumerationQuery += enumeration->getName();
        writeEnumerationQuery += "\");";

        rc = sqlite3_exec(database, writeEnumerationQuery.c_str(), NULL, NULL,
                          &errorMessage);

        if(SQLITE_OK == rc)
        {
            logger.logDebug("Enumeration values were written to the enumerations schema with "
                            "SQLITE_OK status.");
        }
        else
        {
            logger.logDebug("There was an error while data to the enumerations table.");
            logger.logDebug("%s.", errorMessage);

            rc = SQLITEDB_ERROR;
        }
    }

    return rc;

}




/**
 *@brief This method creates all of the schemas that will be needed to store
 *the DWARF and ELF data.
 *
 *@note This method assumes the sqlite database handle has been initialized
 *previously with a call to initialize().
 *
 *@return Returns SQLITE_OK if all of the schemas were created successfully.
 *If an error occurs when creating any of the schemas, returns SQLITEDB_ERROR.
 */
int SQLiteDB::createSchemas(void)
{
    int rc = SQLITE_OK;

    rc = createModuleSchema();

    if(SQLITE_OK == rc)
    {
        logger.logDebug("createModuleSchema() created the modules schema "
                        "successfully.");

        rc = createElfSchema();
        if(SQLITE_OK == rc)
        {
            logger.logDebug("createElfSchema() created the elfs schema "
                            "successfully.");

            rc = createSymbolSchema();
            if(SQLITE_OK == rc)
            {
                logger.logDebug("createSymbolSchema() created the symbols schema "
                                "successfully.");

                rc = createFiledSchema();
                if(SQLITE_OK == rc)
                {
                    logger.logDebug("createFiledSchema() created the fields schema "
                                    "successfully.");

                    rc = createBitFiledSchema();
                    if(SQLITE_OK == rc)
                    {
                        logger.logDebug("createBitFiledSchema() created the bit_fields schema "
                                        "successfully.");

                        rc = createEnumerationSchema();
                        if(SQLITE_OK == rc)
                        {
                            logger.logDebug("createEnumerationSchema() created the bit_fields schema "
                                            "successfully.");
                        }
                        else
                        {
                            logger.logDebug("createEnumerationSchema() failed.");
                            rc = SQLITEDB_ERROR;
                        }
                    }
                    else
                    {
                        logger.logDebug("createBitFiledSchema() failed.");
                        rc = SQLITEDB_ERROR;
                    }
                }
                else
                {
                    logger.logDebug("createFiledSchema() failed.");
                    rc = SQLITEDB_ERROR;
                }
            }
            else
            {
                logger.logDebug("createSymbolSchema() failed.");
                rc = SQLITEDB_ERROR;
            }
        }
        else
        {
            logger.logDebug("createElfSchema() failed.");
            rc = SQLITEDB_ERROR;
        }
    }
    else
    {
        logger.logDebug("createModuleSchema() failed.");
        rc = SQLITEDB_ERROR;
    }

    return rc;
}



/**
 *@brief Creates the modules schema.
 *If the schema already exists, then this method does nothing.
 *This method assumes the sqlite handle database has been initialized
 *previously with a call to initialize().
 *
 *@return Returns SQLITE_OK created the modules schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createModuleSchema(void)
{
    std::string createModuleTableQuery{CREATE_MODULE_TABLE};
    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createModuleTableQuery.c_str(), NULL, NULL,
                      NULL);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Created table %s with OK status",
                        createModuleTableQuery.c_str());
    }
    else
    {
        logger.logError("Failed to create the modules table. '%s'",
                         sqlite3_errmsg(database));
        rc = SQLITEDB_ERROR;
    }

    return rc;
}

/**
 *@brief Creates the elfs schema.
 *If the schema already exists, then this method does nothing.
 *This method assumes the sqlite handle database has been initialized
 *previously with a call to initialize().
 *
 *@return Returns SQLITE_OK created the modules schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createElfSchema(void)
{
    std::string createELfTableQuery {CREATE_ELF_TABLE};
    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createELfTableQuery.c_str(), NULL, NULL,
                      NULL);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Created table %s with OK status",
                        createELfTableQuery.c_str());
    }
    else
    {
        logger.logError("Failed to create the elfs table. '%s'",
                        sqlite3_errmsg(database));
        rc = SQLITEDB_ERROR;
    }

    return rc;
}

/**
 *@brief Creates the symbols schema.
 *If the schema already exists, then this method does nothing.
 *This method assumes the sqlite handle database has been initialized
 *previously with a call to initialize().
 *
 *@return Returns SQLITE_OK created the modules schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createSymbolSchema(void)
{
    std::string createSumbolTableQuery{CREATE_SYMBOL_TABLE};
    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createSumbolTableQuery.c_str(), NULL, NULL,
                      NULL);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Created table %s with OK status",
                        createSumbolTableQuery.c_str());
    }
    else
    {
        logger.logError("Failed to create the symbols table. '%s'",
                        sqlite3_errmsg(database));
        rc = SQLITEDB_ERROR;
    }

    return rc;
}

/**
 *@brief Creates the fields schema.
 *If the schema already exists, then this method does nothing.
 *This method assumes the sqlite handle database has been initialized
 *previously with a call to initialize().
 *
 *@return Returns SQLITE_OK created the modules schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createFiledSchema(void)
{
    std::string createBitFieldTableQuery{CREATE_FIELD_TABLE};
    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createBitFieldTableQuery.c_str(), NULL,
                      NULL,NULL);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Created table %s with OK status",
                        createBitFieldTableQuery.c_str());
    }
    else
    {
        logger.logError("Failed to create the fields table. '%s'",
                        sqlite3_errmsg(database));
        rc = SQLITEDB_ERROR;
    }

    return rc;
}

/**
 *@brief Creates the bit_fields schema. If
 *the schema already exists, then this method does nothing. This method assumes
 *the sqlite handle database has been initialized previously with a call
 *to initialize().
 *
 *@return Returns SQLITE_OK created the modules schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createBitFiledSchema(void)
{
    std::string createBitFieldTableQuery{CREATE_BITFIELD_TABLE};
    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createBitFieldTableQuery.c_str(), NULL,
                        NULL,NULL);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Created table %s with OK status",
                        createBitFieldTableQuery.c_str());
    }
    else
    {
        logger.logError("Failed to create the bit_fields table. '%s'",
                        sqlite3_errmsg(database));
        rc = SQLITEDB_ERROR;
    }
    return rc;
}

/**
 *@brief Creates the enumerations schema.
 *If the schema already exists, then this method does nothing.
 *This method assumes the sqlite handle database has been initialized
 *previously with a call to initialize().
 *
 *@return Returns SQLITE_OK created the modules schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createEnumerationSchema(void)
{
    std::string createEnumerationTableQuery{CREATE_ENUMERATION_TABLE};

    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createEnumerationTableQuery.c_str(), NULL,
                      NULL, NULL);

    if(SQLITE_OK==rc)
    {
        logger.logDebug("Created table %s with OK status",
                createEnumerationTableQuery.c_str());
    }
    else
    {
        logger.logError("Failed to create the fields table. '%s'",
                sqlite3_errmsg(database));
        rc = SQLITEDB_ERROR;
    }

    return rc;
}