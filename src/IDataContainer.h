/*
 * ELFDatabase.h
 *
 *  Created on: Aug 3, 2020
 *      Author: vagrant
 */

#ifndef IDATACONTAINER_H_
#define IDATACONTAINER_H_

#include <string>

#include "ElfObj.h"
#include "Juicer.h"
#include "Logger.h"


typedef enum
{
    IDC_TYPE_SQLITE = 0,
    IDC_TYPE_CCDD   = 1,
} IDataContainer_Type_t;


/**
 *@brief class IDataContainer represents an interface that allows structures
 *such as Module to have an interface that is understood by Juicer.
 *This Abstract Interface also allows to support multiple formats
 *to write DWARF and ELF data such SQLite and NASA's CCDD.
 */
class IDataContainer
{
public:
    virtual ~IDataContainer();
    virtual int write(ElfObj& inModule) = 0;
    static IDataContainer* Create(IDataContainer_Type_t containerType, const char *initSpec, ...);

protected:
    IDataContainer();
    virtual int initialize(std::string &initString) = 0;

private:
	static Logger logger;
    static std::string vstring(const char * format, ...);

};

#endif /* IDATACONTAINER_H_ */
