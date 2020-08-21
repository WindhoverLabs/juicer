/*
 * Module.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "Module.h"
#include "ElfFile.h"
#include "Symbol.h"
#include "Field.h"
#include "BitField.h"
#include "Enumeration.h"



Module::Module(std::string &inName) :
    name{inName}, // @suppress("Symbol is not resolved")
    id{0}
{
    logger.logDebug("Module '%s' created.", getName().c_str());
}

Module::Module():
    name{""},
    id{0}
{
    logger.logError("Cannot call Module default constructor.");
}



Module::Module(const Module& module) :
    name{module.name}, // @suppress("Symbol is not resolved")
    id{0}
{
    logger.logError("Cannot call Module copy constructor.");
}



Module::~Module()
{
}



std::string Module::getName() const
{
	return name;
}



void Module::setName(std::string &inName)
{
    logger.logDebug("Module %s renamed to %s.", name.c_str(), inName.c_str());

	this->name = inName;
}


uint32_t Module::getId(void) const
{
    return id;
}



void Module::setId(uint32_t newId)
{
    id = newId;

}

void Module::addElfFile(ElfFile &inElfFile)
{
	logger.logDebug("Adding ELF %s to Module %s.", inElfFile.getName().c_str(), name.c_str());

}

void Module::addElfFile(std::string& name,
		                uint32_t checksum,
						std::string& date,
		                bool little_endian)
{
	ElfFile newElfFile{*this, name, checksum, date, little_endian};
	elfFiles.push_back(std::make_unique<ElfFile>(*this, name, checksum, date, little_endian));

	addElfFile(newElfFile);
}

bool Module::isLittleEndian(void)
{
    bool rc = false;

    /* Just read the endianness from the first ELF file.  They should all be the same. */
    if(elfFiles.size() > 0)
    {
        rc = elfFiles.front()->isLittleEndian();
    }

    return rc;
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

Symbol* Module::getSymbol(std::string &name)
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

bool Module::isSymbolUnique(std::string &name)
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

Symbol * Module::addSymbol(std::unique_ptr<Symbol> inSymbol)
{
    logger.logDebug("Adding Symbol %s to Module %s.", inSymbol->getName().c_str(), name.c_str());

    symbols.push_back(std::move(inSymbol));

    return symbols.back().get();
}

Symbol * Module::addSymbol(std::string& inName,
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

std::vector<std::unique_ptr<ElfFile>>& Module::getElfFiles()
{
	return elfFiles;
}

std::vector<std::unique_ptr<Symbol>>& Module::getSymbols()
{
	return symbols;
}

std::vector<Field*> Module::getFields()
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

std::vector<Enumeration*> Module::getEnumerations()
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

std::vector<BitField*> Module::getBitFields()
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
