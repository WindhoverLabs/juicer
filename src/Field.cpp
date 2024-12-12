/*
 * Field.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "Field.h"

Field::Field(Symbol& inSymbol, Symbol& inType)
    : symbol{inSymbol},  // @suppress("Symbol is not resolved")
      name{""},
      byte_offset{std::nullopt},
      type{inType},  // @suppress("Symbol is not resolved")
      dimensionList{},
      little_endian{false},
      id{0},
      short_description{""},
      long_description{""}
{
    logger.logDebug("Field %s::%s  byte_offset=%u  type=%s  multiplicity=%s  endian=%s  created.", symbol.getName().c_str(), name.c_str(), byte_offset,
                    type.getName().c_str(), dimensionList.toString(), little_endian ? "LE" : "BE");
}

Field::Field(Symbol& inSymbol, std::string& inName, std::optional<uint32_t> inByteOffset, Symbol& inType, DimensionList& inDimensionList, bool inLittleEndian,
             uint32_t inBitSize, uint32_t inBitOffset)
    : symbol{inSymbol},  // @suppress("Symbol is not resolved")
      name{inName},      // @suppress("Symbol is not resolved")
      byte_offset{inByteOffset},
      type{inType},  // @suppress("Symbol is not resolved")
      dimensionList{inDimensionList},
      little_endian{inLittleEndian},
      bit_offset{inBitOffset},
      bit_size{inBitSize},
      id{0},
      short_description{""},
      long_description{""}

{
    logger.logDebug("Field %s::%s  byte_offset=%u  type=%s  multiplicity=%s  endian=%s  created.", symbol.getName().c_str(), name.c_str(), byte_offset,
                    type.getName().c_str(), dimensionList.toString(), little_endian ? "LE" : "BE");
}

Field::Field(Symbol& inSymbol, std::string& inName, std::optional<uint32_t> inByteOffset, Symbol& inType, bool inLittleEndian, uint32_t inBitSize,
             uint32_t inBitOffset)
    : symbol{inSymbol},  // @suppress("Symbol is not resolved")
      name{inName},      // @suppress("Symbol is not resolved")
      byte_offset{inByteOffset},
      type{inType},  // @suppress("Symbol is not resolved")
      dimensionList{},
      little_endian{inLittleEndian},
      bit_offset{inBitOffset},
      bit_size{inBitSize},
      id{0},
      short_description{""},
      long_description{""}

{
    logger.logDebug("Field %s::%s  byte_offset=%u  type=%s  multiplicity=%s  endian=%s  created.", symbol.getName().c_str(), name.c_str(), byte_offset,
                    type.getName().c_str(), dimensionList.toString(), little_endian ? "LE" : "BE");
}
Field::~Field() {}

std::optional<uint32_t> Field::getByteOffset() const { return byte_offset; }

bool                    Field::isLittleEndian() const { return little_endian; }

std::string&            Field::getName() { return name; }

void                    Field::setName(const std::string& inName)
{
    logger.logDebug("Field %s::%s  renamed  to %s.", symbol.getName().c_str(), name.c_str(), inName.c_str());

    this->name = inName;
}

Symbol& Field::getSymbol() const { return symbol; }

Symbol& Field::getType() { return type; }

bool    Field::isBitField(void)
{
    bool rc = false;

    if (bit_size > 0 && bit_offset > 0)
    {
        rc = true;
    }

    return rc;
}

uint32_t Field::getId(void) const { return id; }

void     Field::setId(uint32_t newId) { id = newId; }

void     Field::setBitOffset(uint32_t newBitOffset) { bit_offset = newBitOffset; }

uint32_t Field::getBitOffset() const { return bit_offset; }

void     Field::setBitSize(uint32_t newBitSize) { bit_size = newBitSize; }

uint32_t Field::getBitSize() const { return bit_size; }

bool     Field::isArray(void) const { return dimensionList.getDimensions().size() > 0; }

/**
 * Get the number of elements in this array, including all dimensions in the case of multidimensional arrays.
 * If this field is not an array, 0 is returned.
 */
uint32_t Field::getArraySize() const
{
    uint32_t size{0};
    if (isArray())
    {
        size = 1;
        for (auto dim : dimensionList.getDimensions())
        {
            size *= dim.getUpperBound() + 1;
        }
    }

    return size;
}

DimensionList& Field::getDimensionList() { return dimensionList; }
