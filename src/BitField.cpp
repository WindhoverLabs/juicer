/*
 * BitField.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "BitField.h"

BitField::BitField(Field& inField) :
    field{inField}, // @suppress("Symbol is not resolved")
	bit_size{0},
	bit_offset{0}
{
    logger.logDebug("BitField %s  bit_size:%u  bit_offset:%u  created.", field.getName().c_str(), bit_size, bit_offset);
}

BitField::BitField(Field& inField, uint32_t inBitSize, uint32_t inBitOffset) :
    field{inField}, // @suppress("Symbol is not resolved")
    bit_size{inBitSize},
    bit_offset{inBitOffset}
{
    logger.logDebug("BitField %s  bit_size:%u  bit_offset:%u  created.", field.getName().c_str(), bit_size, bit_offset);
}

BitField::BitField(BitField& inBitField) :
    field{inBitField.getField()}, // @suppress("Symbol is not resolved")
    bit_size{inBitField.getBitSize()},
    bit_offset{inBitField.getBitOffset()}
{
}

BitField::~BitField()
{
}

uint32_t BitField::getBitOffset() const
{
	return bit_offset;
}

void BitField::setBitOffset(uint32_t inBitOffset)
{
    logger.logDebug("BitField %s  bit_offset changed from %u to %u.", field.getName().c_str(), bit_offset, inBitOffset);

	bit_offset = inBitOffset;
}

uint32_t BitField::getBitSize() const
{
	return bit_size;
}

void BitField::setBitSize(uint32_t inBitSize)
{
    logger.logDebug("BitField %s  bit_size changed from %u to %u.", field.getName().c_str(), bit_size, inBitSize);

	bit_size = inBitSize;
}

Field& BitField::getField() const
{
	return field;
}
