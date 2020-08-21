/*
 * ELFDatabase.cpp
 *
 *  Created on: Aug 3, 2020
 *      Author: vagrant
 */

#include "IDataContainer.h"
#include "SQLiteDB.h"


Logger IDataContainer::logger;


IDataContainer::~IDataContainer ()
{
}



IDataContainer::IDataContainer ()
{
}



IDataContainer* IDataContainer::Create(IDataContainer_Type_t containerType, const char *initSpec, ...)
{
    IDataContainer *container = 0;
    std::string initString;
    va_list args, args_copy;

    va_start(args, initSpec);
    va_copy(args_copy, args);

    int len = vsnprintf(nullptr, 0, initSpec, args);
    if (len < 0)
    {
        va_end(args_copy);
        va_end(args);
    	logger.logError("vsnprintf error in IDataContainer::Create. '%i'", len);
    }
    else
    {
		if (len > 0)
		{
			initString.resize(len);
			// note: &result[0] is *guaranteed* only in C++11 and later
			// to point to a buffer of contiguous memory with room for a
			// null-terminator, but this "works" in earlier versions
			// in *most* common implementations as well...
			vsnprintf(&initString[0], len+1, initSpec, args_copy); // or result.data() in C++17 and later...
		}

		va_end(args_copy);
		va_end(args);

		logger.logDebug("IDC initialization string = '%s'", initString.c_str());

		switch(containerType)
		{
			case IDC_TYPE_SQLITE:
			{
				int rc;

				logger.logDebug("Creating SQLiteDB IDC.");

				container = new SQLiteDB();

				rc = container->initialize(initString);
				if(rc < 0)
				{
					logger.logError("Failed to create SQLiteDB data container. '%i'", rc);
					delete container;
					container = 0;
				}

				logger.logDebug("Created SQLiteDB IDC.");

				break;
			}

			case IDC_TYPE_CCDD:
			{
				logger.logError("CCDD data container not yet supported.");

				break;
			}

			default:
			{
				logger.logError("Invalid IDataContainer type '%u'", (unsigned int)containerType);

				break;
			}
		}
    }

	return container;
}



std::string IDataContainer::vstring(const char *format, ...)
{
    std::string result;
    va_list args, args_copy;

    va_start(args, format);
    va_copy(args_copy, args);

    int len = vsnprintf(nullptr, 0, format, args);
    if (len < 0)
    {
        va_end(args_copy);
        va_end(args);
        throw std::runtime_error("vsnprintf error");
    }

    if (len > 0)
    {
        result.resize(len);
        // note: &result[0] is *guaranteed* only in C++11 and later
        // to point to a buffer of contiguous memory with room for a
        // null-terminator, but this "works" in earlier versions
        // in *most* common implementations as well...
        vsnprintf(&result[0], len+1, format, args_copy); // or result.data() in C++17 and later...
    }

    va_end(args_copy);
    va_end(args);

    return result;
}
