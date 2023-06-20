/*
 * SQLiteStructure.cpp
 *
 *  Created on: Aug 3, 2020
 *      Author: vagrant
 */

#include<string>
#include <stdio.h>
#include "SQLiteDB.h"

SQLiteDB::SQLiteDB() :
        database(0)
{
}

SQLiteDB::~SQLiteDB()
{
}

/**
 *@brief This call back function assumes that the first column is that row's id,
 *for now. It is meant to be passed to sqlite3_exec when executing "SELECT" sql statements.
 *This function will be called for every row that is fetched from the database.
 *
 *@param veryUsed a std::map<std::string, std::vector<std::string> >* that is used to store
 *table's data. The map has the format of "map[id] = row" where row is a vector containing
 *that row's data, excluding that row's id. The ids are used as key for the map.
 *@param argc The number of columns in this row.
 *@param argv An array containing every column of this row.
 *
 */
int SQLiteDB::selectCallback(void *veryUsed, int argc, char **argv, char **azColName)
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

/**
 *@brief A SQLite callback that is meant to be used for "SELECT COUNT(*)..." queries.
 *This is especially useful for when wanting to know if a table has any rows at all
 *that meet a particular condition.
 *
 *@param count The number of rows will be stored on here.
 *@param arc The number of rows. Assuming that the caller used this callback for a
 *"SELECT COUNT(*)..." type of query, this will always be 1.
 *@param argv An array of strings representing every column of this row, which assuming
 *this is a "SELECT COUNT(*)..." type of query, will always be 1.
 *@param azColName
 *
 *@return This function always returns 0.
 */
int SQLiteDB::doesRowExistCallback(void *count, int argc, char **argv, char **azColName) {
	int32_t *c = (int32_t*)count;
    *c = atoi(argv[0]);
    return 0;
}

/**
 *@brief Checks if the symbol called name exists on the symbols table.
 *
 *@param name The name of the symbol to check for.
 *
 *@return If the symbol with the name of name exists, this function returns true. Otherwise,
 *this function returns false.
 *@note Please note that this function assumes that ALL symbols in our table
 *are universally unique across all Elf files.
 */
bool SQLiteDB::doesSymbolExist(std::string name)
{
	int32_t row_count = 0;

    int 	rc = SQLITE_OK;

    char* 	errorMessage = nullptr;

	std::string countRowsQuery{"SELECT COUNT(*) FROM symbols"};
	countRowsQuery += " WHERE name=\"";
	countRowsQuery += name + "\";";

    rc = sqlite3_exec(database, countRowsQuery.c_str(), SQLiteDB::doesRowExistCallback, &row_count,
            &errorMessage);

    if(SQLITE_OK != rc)
    {
    	logger.logWarning("Looks like there was a problem sending query \"%s\" "
    			"to the database.", countRowsQuery.c_str());
    	logger.logError("%s", errorMessage);
    	row_count = 0;
    }

    return row_count==0? false: true;
}


/**
 *@brief Checks if the symbol called name exists on the symbols table.
 *
 *@param name The name of the symbol to check for.
 *
 *@return If the symbol with the name of name exists, this function returns true. Otherwise,
 *this function returns false.
 *@note Please note that this function assumes that ALL symbols in our table
 *are universally unique across all Elf files.
 */
bool SQLiteDB::doesArtifactExist(std::string filePath)
{
	int32_t row_count = 0;

    int 	rc = SQLITE_OK;

    char* 	errorMessage = nullptr;

	std::string countRowsQuery{"SELECT COUNT(*) FROM artifacts"};
	countRowsQuery += " WHERE path=\"";
	countRowsQuery += filePath + "\";";

    rc = sqlite3_exec(database, countRowsQuery.c_str(), SQLiteDB::doesRowExistCallback, &row_count,
            &errorMessage);

    if(SQLITE_OK != rc)
    {
    	logger.logWarning("Looks like there was a problem sending query \"%s\" "
    			"to the database.", countRowsQuery.c_str());
    	logger.logError("%s", errorMessage);
    	row_count = 0;
    }

    return row_count==0? false: true;
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
    	rc = createSchemas();
        if(SQLITE_OK == rc)
        {
        	logger.logInfo("The schemas were created successfully.");
        }
        else
        {
        	logger.logInfo("There was an error while creating the schemas.");
        }
    }
    else
    {
        rc = SQLITEDB_ERROR;
        logger.logError("There was an error while opening the database.");
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
 *@brief Writes all the data such as Elf, Symbols and elfs entries
 *to the SQLite database.
 *
 *@param inElf The elf that contains all of the DWARF and ELF data.
 *
 *@return SQLITE_OK if it was able to write the all of data to the database
 *successfully. Please note that attempting to write an entry that already exists is NOT an error
 *in juicer's context; if a user attempts to write a record that already exists, this function returns SQLITE_OK.
 *Otherwise, this method returns SQLITEDB_ERROR.
 */
int SQLiteDB::write(ElfFile& inElf)
{
    int rc  = SQLITEDB_OK;

    rc = writeElfToDatabase(inElf);

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
    if(SQLITEDB_ERROR != rc)
    {
        logger.logDebug("Elf entries were written to the elfs schema with SQLITE_OK "
                        "status.");

        if(SQLITEDB_ERROR != rc)
        {

        	rc = writeElfToDatabase(inElf);

        	if(SQLITEDB_ERROR != rc)
        	{
        		 rc = writeArtifactsToDatabase(inElf);
        	}
        	else
        	{
                logger.logDebug("There was an error while writing field entries to the"
                                " database.");
                rc = SQLITEDB_ERROR;
        	}

            rc = writeSymbolsToDatabase(inElf);

            if(SQLITEDB_ERROR != rc)
            {
                logger.logDebug("Symbol entries were written to the symbols schema "
                                "with SQLITE_OK status.");

                rc = writeFieldsToDatabase(inElf);

                writeDimensionsListToDatabase(inElf);

                if(SQLITEDB_ERROR != rc)
                {
                    logger.logDebug("Field entries were written to the fields schema "
                                    "with SQLITE_OK status.");

                        rc = writeEnumerationsToDatabase(inElf);

                        if(SQLITEDB_ERROR != rc)
                        {
                            logger.logInfo("Enumeration entries were written to the fields schema "
                                           "with SQLITE_OK status.");
                        	rc = SQLITE_OK;
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
                    logger.logDebug("There was an error while writing field entries to the"
                                    " database.");

                    rc = SQLITEDB_ERROR;
                }


            }
        }

    }
    else
    {
        logger.logDebug("There was an error while writing elf entries to the database.");

        rc = SQLITEDB_ERROR;
    }

    return rc;
}



/**
 *@brief Iterates through all of the ELF entries in
 *inElf and writes each one to the "elfs" table.
 *
 *@param inElf The elf that has the Elf data.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *elf entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeElfToDatabase(ElfFile& inElf)
{
   int      rc  = SQLITEDB_OK;
   char* errorMessage = NULL;

   /*
	* @todo I want to store these SQLite magical values into MACROS,
	* but I'm not sure what is the best way to do that without it being
	* messy.
	*/
   std::string writeElfQuery{};

   writeElfQuery += "INSERT INTO elfs(name, checksum, little_endian) "
					"VALUES(\"";
   writeElfQuery += inElf.getName();
   writeElfQuery += "\",";
   writeElfQuery += std::to_string(inElf.getChecksum());
   writeElfQuery += ",";
   writeElfQuery += std::to_string(inElf.isLittleEndian()? SQLiteDB_TRUE: SQLiteDB_FALSE);
   writeElfQuery += ");";

   logger.logDebug("Sending \"%s\" query to database.",writeElfQuery.c_str());

   rc = sqlite3_exec(database, writeElfQuery.c_str(), NULL, NULL,
					 &errorMessage);

   if(SQLITE_OK == rc)
   {
	   logger.logDebug("Elf values were written to the elfs schema with "
					   "SQLITE_OK status.");
       /*Write the id to this elf so that other tables can use it as
        *a foreign key */

       sqlite3_int64 lastRowId = sqlite3_last_insert_rowid(database);
       inElf.setId(lastRowId);
   }
   else
   {

	   if(sqlite3_extended_errcode(database) == SQLITE_CONSTRAINT_UNIQUE)
	   {
		   logger.logDebug("%s.", errorMessage);
		   rc  = SQLITE_OK;
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
 *@brief Iterates through all of the Artifact entries in
 *inElf and writes each one to the "artifacts" table.
 *
 *@param inElf The elf that has the Elf data.
 *
 *@return Returns SQLITEDB_OK if all of the artifact entries are written to the
 *database successfully. If the method fails to write at least one of the
 *elf entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeArtifactsToDatabase(ElfFile& inElf)
{
   int      rc  = SQLITEDB_OK;
   char* errorMessage = NULL;

   /*
	* @todo I want to store these SQLite magical values into MACROS,
	* but I'm not sure what is the best way to do that without it being
	* messy.
	*/
   std::string writArtifactQuery{};

   for(auto&& s: inElf.getSymbols())
   {
	   Artifact& ar = s->getArtifact();

		bool artifacExists = doesArtifactExist(ar.getFilePath());

		/**
		 *First check if the symbol already exists in the database.
		 *If it does we don't need to write to the database. In that case, all we need is
		 *to get the id which will be used by other tables such as enumerations and fields as
		 * a foreign key.
		 */
		if(artifacExists)
		{

			std::map<std::string, std::vector<std::string>> artifactMap{};

			std::string getArtifactIdQuery{"SELECT * FROM artifacts where path="};
			getArtifactIdQuery += "\"";
			getArtifactIdQuery += ar.getFilePath();

			getArtifactIdQuery += "\";";
			rc = sqlite3_exec(database, getArtifactIdQuery.c_str(), SQLiteDB::selectCallback, &artifactMap,
					&errorMessage);

			if(SQLITE_OK == rc)
			{
				/**
				 * We know there is only one element in our map, since symbol names are unique.
				 */
				for(auto pair: artifactMap)
				{
					ar.setId(std::stoi(pair.first));
				}
			}
		}

	   writArtifactQuery += "INSERT INTO artifacts(elf, path) "
						"VALUES(";
	   writArtifactQuery += std::to_string(inElf.getId());
	   writArtifactQuery += ",";
	   writArtifactQuery += "\"";
	   writArtifactQuery += s->getArtifact().getFilePath();
	   writArtifactQuery +=	 "\"";
	   writArtifactQuery += ");";

	   logger.logDebug("Sending \"%s\" query to database.",writArtifactQuery.c_str());

	   rc = sqlite3_exec(database, writArtifactQuery.c_str(), NULL, NULL,
						 &errorMessage);
	   if(SQLITE_OK == rc)
	   {
		   logger.logDebug("Elf values were written to the artifacts schema with "
						   "SQLITE_OK status.");
	       /*Write the id to this elf so that other tables can use it as
	        *a foreign key */

	       sqlite3_int64 lastRowId = sqlite3_last_insert_rowid(database);
	       if(s->getName().compare("CFE_ES_TaskInfo_t") == 0)
	       {
	    	   std::cout << "lastRowId" << lastRowId << std::endl;
	       }
	       ar.setId(lastRowId);
	   }
	   else
	   {

		   if(sqlite3_extended_errcode(database) == SQLITE_CONSTRAINT_UNIQUE)
		   {
			   logger.logDebug("%s.", errorMessage);
			   rc  = SQLITE_OK;
		   }
		   else
		   {
			   logger.logDebug("There was an error while writing data to the artifacts table.");
			   logger.logDebug("%s.", errorMessage);
			   rc = SQLITEDB_ERROR;
		   }
	   }

	   writArtifactQuery.clear();
   }


   return rc;
}



/**
 *@brief Iterates through all of the symbols entries in
 *inElf and writes each one to the "symbols" table.
 *
 *
 *@return Returns SQLITEDB_OK if all of the symbols entries are written to the
 *database successfully. If the method fails to write at least one of the
 *symbol entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeSymbolsToDatabase(ElfFile& inElf)
{
    int         rc  = SQLITEDB_OK;
    char*       errorMessage = nullptr;

    /**
     * @note Are we allowed for ground tools to do this for loops?
     * I know for Flight Software we need to explicitly state the "++i",
     * but should/can we do this here with loops for Juicer?
     */

    for(auto&& symbol : inElf.getSymbols())
    {
		bool symbolExists = doesSymbolExist(symbol->getName());

		/**
		 *First check if the symbol already exists in the database.
		 *If it does we don't need to write to the database. In that case, all we need is
		 *to get the id which will be used by other tables such as enumerations and fields as
		 * a foreign key.
		 */
		if(symbolExists)
		{

			std::map<std::string, std::vector<std::string>> symbolsMap{};

			std::string getSymbolIdQuery{"SELECT * FROM symbols where name="};
			getSymbolIdQuery += "\"";
			getSymbolIdQuery += symbol->getName();

			getSymbolIdQuery += "\";";
			rc = sqlite3_exec(database, getSymbolIdQuery.c_str(), SQLiteDB::selectCallback, &symbolsMap,
					&errorMessage);

			if(SQLITE_OK == rc)
			{
				/**
				 * We know there is only one element in our map, since symbol names are unique.
				 */
				for(auto pair: symbolsMap)
				{
					 symbol->setId(std::stoi(pair.first));
				}
			}
		}

		   else
		   {
			   /*
			* @todo I want to store these SQLite magical values into MACROS,
			 * but I'm not sure what is the best way to do that without it being
			 * messy.
			 */
			   std::string writeSymbolQuery{};

			   writeSymbolQuery += "INSERT INTO symbols(elf, name, byte_size, artifact) "
									 "VALUES(";
			   writeSymbolQuery += std::to_string(symbol->getElf().getId());
			   writeSymbolQuery += ",\"";
			   writeSymbolQuery += symbol->getName();

			   writeSymbolQuery += "\",";
			   writeSymbolQuery += std::to_string(symbol->getByteSize());
			   writeSymbolQuery += ",";
			   writeSymbolQuery += std::to_string(symbol->getArtifact().getId());
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
				   logger.logError("Looks like something went wrong with query "
						   "\"%s\":\"%s\"", writeSymbolQuery ,errorMessage);

			   }

		   }

    }

    return rc;

}


/**
 *@brief Iterates through all of the field entries in
 *inElf and writes each one to the "fields" table.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *field entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeFieldsToDatabase(ElfFile& inElf)
{
    int         rc  = SQLITEDB_OK;
    char*       errorMessage = NULL;

    /**
     * @note Are we allowed for ground tools to do this for loops?
     * I know for Flight Software we need to explicitly state the "++i",
     * but should/can we do this here with loops for Juicer?
     */
    for(auto field : inElf.getFields())
    {
        /*
         * @todo I want to store these SQLite magical values into MACROS,
         * but I'm not sure what is the best way to do that without it being
         * messy.
         */
        std::string writeFieldQuery{};

        writeFieldQuery += "INSERT INTO fields(symbol, name, byte_offset, type, "
                            "little_endian, bit_size, bit_offset) VALUES(";
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
        writeFieldQuery += std::to_string(field->isLittleEndian()?
                                          SQLiteDB_TRUE: SQLiteDB_FALSE);

        writeFieldQuery += ",";
        writeFieldQuery += std::to_string(field->getBitSize());
        writeFieldQuery += ",";
        writeFieldQuery += std::to_string(field->getBitOffset());

        writeFieldQuery += ");";

        rc = sqlite3_exec(database, writeFieldQuery.c_str(), NULL, NULL,
                          &errorMessage);

        if(SQLITE_OK == rc)
        {
            /*Write the id to this field so that other tables can use it as
             *a foreign key */
            sqlite3_int64 lastRowId = sqlite3_last_insert_rowid(database);
            field->setId(lastRowId);
        }
        else
        {
            if(sqlite3_extended_errcode(database) == SQLITE_CONSTRAINT_UNIQUE)
            {
            	rc  = SQLITE_OK;
                logger.logWarning("SQLITE_CONSTRAINT_UNIQUE violated. "
                				"Query:\"%s\""
                				"Error message:%s.", writeFieldQuery.c_str(), errorMessage);
            }
            else
            {
            	rc = SQLITEDB_ERROR;
                logger.logError("There was an error while writing data to the fields table. "
                				"Query:\"%s\""
                				"Error message:%s.", writeFieldQuery.c_str(), errorMessage);
            }
        }
    }

    return rc;

}

/**
 *@brief Iterates through all of the field entries in
 *inElf and writes each one to the "fields" table.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *field entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeDimensionsListToDatabase(ElfFile& inElf)
{
	//TODO:Add dimensions to ElfFile object.
    int         rc  = SQLITEDB_OK;
    char*       errorMessage = NULL;

    /**
     * @note Are we allowed for ground tools to do this for loops?
     * I know for Flight Software we need to explicitly state the "++i",
     * but should/can we do this here with loops for Juicer?
     */
    for(auto field : inElf.getFields())
    {
    	if (field->isArray())
    	{

    		uint32_t dimOrder = 0;
        	for(auto dim: field->getDimensionList().getDimensions())
        	{
                /*
                 * @todo I want to store these SQLite magical values into MACROS,
                 * but I'm not sure what is the best way to do that without it being
                 * messy.
                 */
                std::string writeDimsQuery{};

                writeDimsQuery += "INSERT INTO dimension_lists(field_id, dim_order, upper_bound"
                                    ") VALUES(";
                writeDimsQuery += std::to_string(field->getId());
                writeDimsQuery += ",";
                writeDimsQuery += std::to_string(dimOrder);
                writeDimsQuery += ",";
                writeDimsQuery += std::to_string(dim.getUpperBound());

                writeDimsQuery += ");";

                rc = sqlite3_exec(database, writeDimsQuery.c_str(), NULL, NULL,
                                  &errorMessage);

                if(SQLITE_OK == rc)
                {
                    logger.logDebug("DimensionList values were written to the symbols schema with "
                                     "SQLITE_OK status.");
                }
                else
                {
                    if(sqlite3_extended_errcode(database) == SQLITE_CONSTRAINT_UNIQUE)
                    {
                        logger.logWarning("SQLITE_CONSTRAINT_UNIQUE violated. "
                        				"Query:\"%s\""
                        				"Error message:%s.", writeDimsQuery.c_str(), errorMessage);
                    	rc  = SQLITE_OK;
                    }
                    else
                    {
                        logger.logError("There was an error while writing data to the dimension table. "
                                        "Query:\"%s\""
                                        "Error message:%s.", writeDimsQuery.c_str(), errorMessage);
                        rc = SQLITEDB_ERROR;
                    }
                }
            	dimOrder++;
        	}

    	}
    }

    return rc;

}


/**
 *@brief Iterates through all of the enumeration entries in
 *inElf and writes each one to the "enumerations" table.
 *
 *@return Returns SQLITEDB_OK if all of the elf entries are written to the
 *database successfully. If the method fails to write at least one of the
 *enumeration entries to the database, then SQLITEDB_ERROR is returned.
 */
int SQLiteDB::writeEnumerationsToDatabase(ElfFile& inElf)
{
    int   rc = SQLITEDB_OK;
    char* errorMessage = NULL;

    /**
     * @note Are we allowed for ground tools to do this for loops?
     * I know for Flight Software we need to explicitly state the "++i",
     * but should/can we do this here with loops for Juicer?
     */
    for(auto enumeration : inElf.getEnumerations())
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
            logger.logDebug("There was an error while writing data to the enumerations table. %s.", errorMessage);

            if(sqlite3_extended_errcode(database) == SQLITE_CONSTRAINT_UNIQUE)
            {
            	rc  = SQLITE_OK;
            }
            else
            {
            	rc = SQLITEDB_ERROR;
            }
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

                rc = createFieldsSchema();

                if(SQLITE_OK == rc)
                {
                    rc = createDimensionsSchema();

    				if(SQLITE_OK == rc)
    				{
    					logger.logDebug("createDimensionsSchema() created the dimensions schema "
    									"successfully.");

    					rc = createEnumerationSchema();
    					if(SQLITE_OK == rc)
    					{
    						logger.logDebug("createEnumerationSchema() created the enumerations schema "
    										"successfully.");

        					rc = createArtifactsSchema();
        					if(SQLITE_OK == rc)
        					{
        						logger.logDebug("createArtifactsSchema() created the artifacts schema "
        										"successfully.");

        					}
        					else
        					{
        						logger.logDebug("createArtifactsSchema() failed.");
        						rc = SQLITEDB_ERROR;
        					}

    					}
    					else
    					{
    						logger.logDebug("createDimensionsSchema() failed.");
    						rc = SQLITEDB_ERROR;
    					}
    				}
    				else
    				{
                        logger.logDebug("createDimensionsSchema() failed.");
                        rc = SQLITEDB_ERROR;
    				}
                }
                else
                {
                    logger.logDebug("createFieldsSchema() failed.");
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



    return rc;
}


/**
 *@brief Creates the elfs schema.
 *If the schema already exists, then this method does nothing.
 *This method assumes the sqlite handle database has been initialized
 *previously with a call to initialize().
 *
 *@return Returns SQLITE_OK created the elfs schema successfully.
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
        logger.logDebug("Created table \"elfs\" with OK status");
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
 *@return Returns SQLITE_OK created the elfs schema successfully.
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
        logger.logDebug("Created table \"symbols\" with OK status");
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
 *@return Returns SQLITE_OK created the elfs schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createFieldsSchema(void)
{
    std::string createFieldTableQuery{CREATE_FIELD_TABLE};
    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createFieldTableQuery.c_str(), NULL,
                      NULL,NULL);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Created table \"fields\" with OK status");
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
 *@brief Creates the fields schema.
 *If the schema already exists, then this method does nothing.
 *This method assumes the sqlite handle database has been initialized
 *previously with a call to initialize().
 *
 *@return Returns SQLITE_OK created the elfs schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createDimensionsSchema(void)
{
    std::string createDimensionsTableQuery{CREATE_DIMENSION_TABLE};
    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createDimensionsTableQuery.c_str(), NULL,
                      NULL,NULL);

    if(SQLITE_OK == rc)
    {
        logger.logDebug("Created table \"dimensions\" with OK status");
    }
    else
    {
        logger.logError("Failed to create the dimensions table. '%s'",
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
 *@return Returns SQLITE_OK created the elfs schema successfully.
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
        logger.logDebug("Created table \"enumerations\" with OK status");
    }
    else
    {
        logger.logError("Failed to create the enumerations table. '%s'",
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
 *@return Returns SQLITE_OK created the elfs schema successfully.
 *If an error occurs, SQLITEDB_ERROR returns.
 */
int SQLiteDB::createArtifactsSchema(void)
{
    std::string createArtifactsTableQuery{CREATE_ARTIFACTS_TABLE};

    int         rc = SQLITE_OK;

    /*@todo The last argument for sqlite3_exec is an error handler that is not
     * necessary to pass in, but I really think we should for better error
     * logging.*/
    rc = sqlite3_exec(database, createArtifactsTableQuery.c_str(), NULL,
                      NULL, NULL);

    if(SQLITE_OK==rc)
    {
        logger.logDebug("Created table \"artifacts\" with OK status");
    }
    else
    {
        logger.logError("Failed to create the artifacts table. '%s'",
                sqlite3_errmsg(database));
        rc = SQLITEDB_ERROR;
    }

    return rc;
}
