/*
 * Symbol.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "Symbol.h"
#include "Field.h"
#include "Enumeration.h"

Symbol::Symbol(Module& inModule) :
    module{inModule}, // @suppress("Symbol is not resolved")
	name{""},
	byte_size{0}
{
    logger.logDebug("Symbol %s::%s (%u bytes) created.", module.getName().c_str(), name.c_str(), byte_size);
}



Symbol::Symbol(Module& inModule, std::string &inName, uint32_t inByteSize) :
    module{inModule}, // @suppress("Symbol is not resolved")
    name{inName}, // @suppress("Symbol is not resolved")
    byte_size{inByteSize}
{
    logger.logDebug("Symbol %s::%s (%u bytes) created.", module.getName().c_str(), name.c_str(), byte_size);
}



Symbol::~Symbol()
{
}

void Symbol::addField( Field& inField)
{
	logger.logDebug("Adding Field %s to Symbol %s.", inField.getName().c_str(), name.c_str());

	addField(inField.getName(), inField.getByteOffset(), inField.getType(), inField.getMultiplicity(), inField.isLittleEndian());
}




/**
 *@note There is another function in the <memory.h> API,
 *which is used by the smart pointers such as
 *unique_ptr, which is also called addField. Maybe we should place our
 *elf data structures inside a namespace called ElfData?
 */
void Symbol::addField(std::string& inName,
		              uint32_t inByteOffset,
					  Symbol &inType,
					  uint32_t inMultiplicity,
					  bool inLittleEndian)
{

    Field *field = getField(inName);

    if(field == nullptr)
    {
        fields.push_back(std::make_unique<Field>(*this, inName, inByteOffset, inType, inMultiplicity, inLittleEndian));
    }
}

void Symbol::addEnumeration( Enumeration& inEnumeration)
{
	logger.logDebug("Adding Enumeration %s:%i to Symbol %s.", inEnumeration.getName().c_str(), inEnumeration.getValue(), name.c_str());

	addEnumeration(inEnumeration.getName(), inEnumeration.getValue());
}

void Symbol::addEnumeration(std::string& inName, int32_t inValue)
{
	enumerations.push_back(std::make_unique<Enumeration>(*this, inName, inValue));
}



Symbol::Symbol(const Symbol &inSymbol) :
    module{inSymbol.module}, // @suppress("Symbol is not resolved")
    name{inSymbol.name}, // @suppress("Symbol is not resolved")
    byte_size{inSymbol.byte_size},
    id{0}
{
    std::cout << "Copy constructor\n";
}



const std::string& Symbol::getName(void) const
{
	return name;
}



void Symbol::setName(std::string &inName)
{
    logger.logDebug("Symbol %s::%s renamed to %s::%s.", module.getName().c_str(), name.c_str(), module.getName().c_str(), inName.c_str());

	this->name = inName;

}



uint32_t Symbol::getByteSize() const
{
	return byte_size;
}



void Symbol::setByteSize(uint32_t inByteSize)
{
    logger.logDebug("Symbol %s::%s byte size changed from %d to %d.", module.getName().c_str(), name.c_str(), byte_size, inByteSize);

    this->byte_size = inByteSize;
}



void Symbol::setId(uint32_t newId)
{
    id = newId;
}



uint32_t Symbol::getId(void) const
{
    return id;
}




const Module& Symbol::getModule() const
{
	return module;
}

std::vector<std::unique_ptr<Enumeration>>& Symbol::getEnumerations()
{
	return enumerations;
}

std::vector<std::unique_ptr<Field>>& Symbol::getFields()
{
	return fields;
}



/**
 *@brief Iterates through all of the possible fields and finds the field with
 *the name fieldName.
 *
 *@return If the field is found, then a pointer to the found field is returned.
 *Otherwise, this method returns nullptr. Please note that Symbol
 *is an owning class and manages all of its resources, including this pointer.
 *The call doesn't have to nor should attempt to manage the returned pointer.
 *
 *@note Given the possibility that this method cannot find the field
 *with the given name, then I think we should return what is called an
 *"Optional" value in modern C++. std::optional is the data type for this in
 *the STL. However, std::optional is only available on C++17 and up.
 *We can cook up our own std::optional with something like std::tuple
 *nonetheless. Will re-evaluate. Visit https://en.cppreference.com/w/cpp/utility/optional
 *and https://en.cppreference.com/w/cpp/utility/tuple for details.
 */
Field* Symbol::getField(std::string &fieldName) const
{
    Field* outField = nullptr;

	for(auto&& field: fields)
	{

		if(field->getName() == fieldName)
		{
			return field.get();
		}
	}

	return outField;
}

bool Symbol::isFieldUnique(std::string &name)
{
	bool rc = false;
	Field* field = getField(name);

	if(nullptr == field)
	{
		rc = true;
	}
	else
	{
		rc = false;
	}

	return rc;
}

bool Symbol::hasFields(void)
{
	bool rc = false;

	if(fields.size() > 0)
	{
		rc = true;
	}

	return rc;
}

bool Symbol::isEnumerated(void)
{
	bool rc = false;

	if(enumerations.size() > 0)
	{
		rc = true;
	}

	return rc;
}