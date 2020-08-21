/*
 * BitField.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef BITFIELD_H_
#define BITFIELD_H_

#include <stdint.h>
#include "Field.h"
#include "Logger.h"

class BitField
{
public:
	BitField(Field &field);
	BitField(Field &field, uint32_t bit_size, uint32_t bit_offset);
	virtual ~BitField();
	BitField(BitField &bitfield);
	uint32_t getBitOffset() const;
	void setBitOffset(uint32_t bitOffset);
	uint32_t getBitSize() const;
	void setBitSize(uint32_t bitSize);
	Field& getField() const;
	void setField(const Field& field);

protected:

private:
	Field    &field;
	uint32_t bit_size;
	uint32_t bit_offset;
	Logger   logger;
};

#endif /* BITFIELD_H_ */
