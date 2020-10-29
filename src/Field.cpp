/*
 * Field.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "Field.h"

Field::Field(Symbol& inSymbol, Symbol& inType) :
    	    symbol{inSymbol}, // @suppress("Symbol is not resolved")
		    name{""},
		    byte_offset{0},
		    type{inType}, // @suppress("Symbol is not resolved")
		    multiplicity{0},
		    little_endian{false},
		    id{0}
{
    logger.logDebug("Field %s::%s  byte_offset=%u  type=%s  multiplicity=%d  endian=%s  created.", symbol.getName().c_str(), name.c_str(), byte_offset, type.getName().c_str(), multiplicity, little_endian ? "LE" : "BE");
}

Field::Field(Symbol& inSymbol, std::string &inName, uint32_t inByteOffset,
		Symbol& inType, uint32_t inMultiplicity, bool inLittleEndian, uint32_t inBitSize,
		uint32_t inBitOffset) :
    	    symbol{inSymbol}, // @suppress("Symbol is not resolved")
    		name{inName}, // @suppress("Symbol is not resolved")
    		byte_offset{inByteOffset},
    		type{inType}, // @suppress("Symbol is not resolved")
    		multiplicity{inMultiplicity},
    		little_endian{inLittleEndian},
    		bit_offset{inBitOffset},
			bit_size{inBitSize},
    		id{0}


{

    logger.logDebug("Field %s::%s  byte_offset=%u  type=%s  multiplicity=%d  endian=%s  created.", symbol.getName().c_str(), name.c_str(), byte_offset, type.getName().c_str(), multiplicity, little_endian ? "LE" : "BE");
}

Field::~Field()
{
}

Field::Field(Field& field) :
    		symbol{field.getSymbol()}, // @suppress("Symbol is not resolved")
    		name{field.getName()}, // @suppress("Symbol is not resolved")
    		byte_offset{field.getByteOffset()},
    		type{field.getType()}, // @suppress("Symbol is not resolved")
    		multiplicity{field.getMultiplicity()},
    		little_endian{field.isLittleEndian()}
{
}

uint32_t Field::getByteOffset() const
{
	return byte_offset;
}

void Field::setByteOffset(uint32_t inByteOffset)
{
    logger.logDebug("Field %s::%s  byte_offset changed from %u to %u.", symbol.getName().c_str(), name.c_str(), byte_offset, inByteOffset);

	byte_offset = inByteOffset;
}

bool Field::isLittleEndian() const
{
	return little_endian;
}

void Field::setLittleEndian(bool inLittleEndian)
{
    logger.logDebug("Field %s::%s  endian changed from %s to %s.", symbol.getName().c_str(), name.c_str(), little_endian ? "LE" : "BE", inLittleEndian ? "LE" : "BE");

	little_endian = inLittleEndian;
}

uint32_t Field::getMultiplicity() const
{
	return multiplicity;
}

void Field::setMultiplicity(uint32_t inMultiplicity)
{
    logger.logDebug("Field %s::%s  multiplicity changed from %u to %u.", symbol.getName().c_str(), name.c_str(), multiplicity, inMultiplicity);

	this->multiplicity = inMultiplicity;
}

std::string& Field::getName()
{
	return name;
}

void Field::setName(const std::string& inName)
{
    logger.logDebug("Field %s::%s  renamed  to %s.", symbol.getName().c_str(), name.c_str(), inName.c_str());

	this->name = inName;
}

Symbol& Field::getSymbol() const
{
	return symbol;
}

Symbol& Field::getType()
{
	return type;
}

bool Field::isBitField(void)
{
	bool rc = false;

	if(bit_size>0 && bit_offset>0)
	{
		rc = true;
	}

	return rc;
}

uint32_t Field::getId(void) const
{
    return id;
}

void Field::setId(uint32_t newId)
{
    id = newId;
}

void Field::setBitOffset(uint32_t newBitOffset)
{
	bit_offset = newBitOffset;
}

uint32_t Field::getBitOffset() const
{
	return bit_offset;
}

void Field::setBitSize(uint32_t newBitSize)
{
	bit_size = newBitSize;
}

uint32_t Field::getBitSize() const
{
	return bit_size;
}
