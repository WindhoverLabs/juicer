/*
 * Module.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "ElfFile.h"

#include "Symbol.h"
#include "Field.h"
#include "Enumeration.h"
#include <limits.h>

/**
 *@brief
 *
 *@param inName This is a path to the Elf File.
 */
ElfFile::ElfFile(std::string &inName) :
    name{inName}, // @suppress("Symbol is not resolved")
    id{0}
{
    normalizePath(name);
    logger.logDebug("Elf '%s' created.", getName().c_str());
}

ElfFile::ElfFile():
    name{""},
    id{0}
{
    logger.logError("Cannot call Module default constructor.");
}



ElfFile::ElfFile(const ElfFile& elf) :
    name{elf.name}, // @suppress("Symbol is not resolved")
    id{0}
{
    logger.logError("Cannot call Module copy constructor.");
}



ElfFile::~ElfFile()
{
}



std::string ElfFile::getName() const
{
	return name;
}


/**
 *@brief
 *
 *@note Absolute paths are enforced.
 */
void ElfFile::setName(std::string &inName)
{
    logger.logDebug("Module %s renamed to %s.", name.c_str(), inName.c_str());

    this->name = inName;
    normalizePath(name);
}


uint32_t ElfFile::getId(void) const
{
    return id;
}



void ElfFile::setId(uint32_t newId)
{
    id = newId;

}


void ElfFile::isLittleEndian(bool inLittleEndian)
{
    logger.logDebug("ELF %s endian changed from %s to %s.", name.c_str(), little_endian ? "LE" : "BE", inLittleEndian ? "LE" : "BE");

	little_endian = inLittleEndian;
}

bool ElfFile::isLittleEndian(void) const
{
	return little_endian;
}

void ElfFile::setDate(const std::string& inDate)
{
    logger.logDebug("ELF %s date changed from %s to %s.", name.c_str(), date.c_str(), inDate.c_str());

	this->date = inDate;
}

const std::string& ElfFile::getDate() const
{
	return date;
}

void ElfFile::setMD5(std::string inChecksum)
{
    logger.logDebug("ELF %s checksum changed from 0x%08x to 0x%08x.", name.c_str(), md5, inChecksum);

	this->md5 = inChecksum;
}

std::string ElfFile::getMD5() const
{
	return md5;
}

/**
 *@note IF there is the possibility that this method cannot find the symbol
 *with the given name, then I think we should return what is called an
 *"Optional" value in modern C++. std::optional is the data type for this in
 *the STL. However, std::optional is only available on C++17 and up.
 *We can cook up our own std::optional with something like std::tuple
 *nonetheless. Will re-evaluate. Visit https://en.cppreference.com/w/cpp/utility/optional
 *and https://en.cppreference.com/w/cpp/utility/tuple for details.
 */
Symbol* ElfFile::getSymbol(std::string &name)
{
    Symbol* returnSymbol = nullptr;

	for(auto&& symbol: symbols)
	{

		if(symbol->getName() == name)
		{
		    returnSymbol = symbol.get();
		}
	}

	return returnSymbol;
}

bool ElfFile::isSymbolUnique(std::string &name)
{
	bool    rc = false;
	Symbol* symbol = getSymbol(name);

	if(symbol == nullptr)
	{
		rc = true;
	}
	else
	{
		rc = false;

		logger.logDebug("isSymbolUnique is false.");
	}

	return rc;
}

Symbol * ElfFile::addSymbol(std::unique_ptr<Symbol> inSymbol)
{
    logger.logDebug("Adding Symbol %s to Module %s.", inSymbol->getName().c_str(), name.c_str());

    symbols.push_back(std::move(inSymbol));

    return symbols.back().get();
}

Symbol * ElfFile::addSymbol(std::string& inName,
		                uint32_t inByteSize,
						Artifact newArtifact)
{
    Symbol *symbol = getSymbol(inName);

    if(symbol == nullptr)
    {
        std::unique_ptr<Symbol> newSymbol = std::make_unique<Symbol>(*this, inName, inByteSize, newArtifact);

	    symbols.push_back(std::move(newSymbol));

        symbol = symbols.back().get();
    }

    return symbol;
}

std::vector<std::unique_ptr<Symbol>>& ElfFile::getSymbols()
{
	return symbols;
}

std::vector<Field*> ElfFile::getFields()
{
	std::vector<Field*> outFields = std::vector<Field*>();
	/**
	 *@note numberOfFields could be used for pre-allocating space for the vector.
	 */
	int                 numberOfFields = 0;

	for(auto&& symbol: symbols)
	{
	    numberOfFields += symbol->getFields().size();
	}

	for(auto&& symbol: symbols)
	{
		for(auto&& field: symbol->getFields())
		{
		    outFields.push_back(field.get());
		}
	}

	return outFields;
}

std::vector<Enumeration*> ElfFile::getEnumerations()
{
	std::vector<Enumeration*> outEnumerations = std::vector<Enumeration*>();

	for(auto&& symbol: symbols)
	{
	    for(auto&& enumeration: symbol->getEnumerations())
	    {
	        outEnumerations.push_back(enumeration.get());
	    }
	}

	return outEnumerations;
}

/**
 *Converts the path into an absolute path.
 */
void ElfFile::normalizePath(std::string& path)
{
  char resolvedPath[PATH_MAX];
  realpath(path.c_str(), resolvedPath);

  /**
   * Whether std::string::clear deallocates memory or not is, like many things in the standard,
   * implementation-defined.
   * One may read more details about this here:https://en.cppreference.com/w/cpp/string/basic_string/clear
   */
  path.clear();
  path.insert(0, resolvedPath);
}
