/*
 * Field.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef FIELD_H_
#define FIELD_H_

#include <stdint.h>
#include <vector>
#include "Symbol.h"
#include "Logger.h"


class BitField;

/**
 *@note This is an "owning class". This means that it manages some resources through unique_ptr
 *for this class that will be the bit_fields object. This means the lifetime
 *of BitField objects depend on the lifetime of Field; once Field is out of scope
 *then bit_fields and every unique pointer it contains will be destroyed automatically.
 */
class Field
{
public:
	Field(Symbol &symbol, Symbol &type);
	Field(Symbol &symbol,
		  std::string &name,
		  uint32_t byte_offset,
		  Symbol &type,
		  uint32_t multiplicity,
		  bool little_endian);
	virtual ~Field();
	uint32_t getByteOffset() const;
	void setByteOffset(uint32_t byteOffset);
	bool isLittleEndian() const;
	void setLittleEndian(bool littleEndian);
	uint32_t getMultiplicity() const;
	void setMultiplicity(uint32_t multiplicity);
	std::string& getName();
	void setName(const std::string& name);
	Symbol& getSymbol() const;
	Symbol& getType();
	uint32_t getId(void) const;
	void setId(uint32_t newId);
	Field(Field& field);
	std::vector<std::unique_ptr<BitField>>& getBitFields();
	void addBitField(BitField &inBitField);
	void addBitField(uint32_t inBitSize, int32_t inBitOffset);
	bool isBitField(void);

protected:

private:
	Symbol &symbol;
	std::string  name;
	uint32_t     byte_offset;
	Symbol &type;
	uint32_t     multiplicity;
	bool         little_endian;
    Logger       logger;
    uint32_t     id;
    std::vector<std::unique_ptr<BitField>> bit_fields;
};

#endif /* FIELD_H_ */
