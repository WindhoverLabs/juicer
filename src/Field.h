

/*
 * Field.h
 *
 *  Created on: Aug 1, 2020
 *      Author: lgomez
 */

#ifndef FIELD_H_
#define FIELD_H_

#include <stdint.h>
#include <vector>

#include "DimensionList.h"
#include "Symbol.h"
#include "Logger.h"

class Symbol;

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
		  DimensionList& dimensionList,
		  bool little_endian,
		  uint32_t inBitSize = 0,
		  uint32_t inBitOffset = 0);
	Field(Symbol &symbol,
		  std::string &name,
		  uint32_t byte_offset,
		  Symbol &type,
		  bool little_endian,
		  uint32_t inBitSize = 0,
		  uint32_t inBitOffset = 0);
	virtual ~Field();
	uint32_t getByteOffset() const;
	void setByteOffset(uint32_t byteOffset);
	bool isLittleEndian() const;
	void setLittleEndian(bool littleEndian);
	uint32_t getMultiplicity() const;
	uint32_t getArraySize() const;
	void setMultiplicity(uint32_t multiplicity);
	std::string& getName();
	void setName(const std::string& name);
	Symbol& getSymbol() const;
	Symbol& getType();
	uint32_t getId(void) const;
	void setId(uint32_t newId);
	uint32_t getBitOffset() const;
	void setBitOffset(uint32_t bitOffset);
	uint32_t getBitSize() const;
	void setBitSize(uint32_t bitSize);
	Field(Field& field);
	bool isBitField(void);
	DimensionList& getDimensionList();
	bool		isArray(void) const;
	std::string getDimensionListStr();

private:
	Symbol 		 					       &symbol;
	std::string  						    name;
	uint32_t     						    byte_offset;
	Symbol 		 						   &type;
	DimensionList							dimensionList;
	bool         							little_endian;
	/*bit fields members.
	 * If this field is not bit-packed, then the bit_size and bit_offset are 0.*/
	uint32_t 	 							bit_offset;
	uint32_t 	 							bit_size;
    Logger       							logger;
    uint32_t     							id;
};

#endif /* FIELD_H_ */
