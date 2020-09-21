/*
 * Module.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "ElfObj.h"
#include "ElfFile.h"
#include "Symbol.h"
#include "Field.h"
#include "BitField.h"
#include "Enumeration.h"


ElfObj::ElfObj(std::string &inName) :
    name{inName}, // @suppress("Symbol is not resolved")
    id{0}
{
    logger.logDebug("Module '%s' created.", getName().c_str());
}

ElfObj::ElfObj():
    name{""},
    id{0}
{
    logger.logError("Cannot call Module default constructor.");
}



ElfObj::ElfObj(const ElfObj& module) :
    name{module.name}, // @suppress("Symbol is not resolved")
    id{0}
{
    logger.logError("Cannot call Module copy constructor.");
}



ElfObj::~ElfObj()
{
}



std::string ElfObj::getName() const
{
	return name;
}



void ElfObj::setName(std::string &inName)
{
    logger.logDebug("Module %s renamed to %s.", name.c_str(), inName.c_str());

	this->name = inName;
}


uint32_t ElfObj::getId(void) const
{
    return id;
}



void ElfObj::setId(uint32_t newId)
{
    id = newId;

}

//bool ElfObj::isLittleEndian(void)
//{
//    bool rc = false;
//
//    /* Just read the endianness from the first ELF file.  They should all be the same. */
//    if(elfFiles.size() > 0)
//    {
//        rc = elfFiles.front()->isLittleEndian();
//    }
//
//    return rc;
//}

void ElfObj::isLittleEndian(bool inLittleEndian)
{
    logger.logDebug("ELF %s endian changed from %s to %s.", name.c_str(), little_endian ? "LE" : "BE", inLittleEndian ? "LE" : "BE");

	little_endian = inLittleEndian;
}

bool ElfObj::isLittleEndian(void) const
{
	return little_endian;
}

void ElfObj::setDate(const std::string& inDate)
{
    logger.logDebug("ELF %s date changed from %s to %s.", name.c_str(), date.c_str(), inDate.c_str());

	this->date = inDate;
}

const std::string& ElfObj::getDate() const
{
	return date;
}

void ElfObj::setChecksum(uint32_t inChecksum)
{
    logger.logDebug("ELF %s checksum changed from 0x%08x to 0x%08x.", name.c_str(), checksum, inChecksum);

	this->checksum = inChecksum;
}

uint32_t ElfObj::getChecksum() const
{
	return checksum;
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
Symbol* ElfObj::getSymbol(std::string &name)
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

bool ElfObj::isSymbolUnique(std::string &name)
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

Symbol * ElfObj::addSymbol(std::unique_ptr<Symbol> inSymbol)
{
    logger.logDebug("Adding Symbol %s to Module %s.", inSymbol->getName().c_str(), name.c_str());

    symbols.push_back(std::move(inSymbol));

    return symbols.back().get();
}

Symbol * ElfObj::addSymbol(std::string& inName,
		                uint32_t inByteSize)
{
    Symbol *symbol = getSymbol(inName);

    if(symbol == nullptr)
    {
        std::unique_ptr<Symbol> newSymbol = std::make_unique<Symbol>(*this, inName, inByteSize);

	    symbols.push_back(std::move(newSymbol));

        symbol = symbols.back().get();
    }

    return symbol;
}

std::vector<std::unique_ptr<Symbol>>& ElfObj::getSymbols()
{
	return symbols;
}

std::vector<Field*> ElfObj::getFields()
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

std::vector<Enumeration*> ElfObj::getEnumerations()
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

std::vector<BitField*> ElfObj::getBitFields()
{
	std::vector<BitField*> outBitFields;
	std::vector< Field*> fields = getFields();

	for(auto&& field: getFields())
	{
	    for(auto&& bitField: field->getBitFields())
	    {
	        outBitFields.push_back(bitField.get());
	    }
	}

	return outBitFields;
}
