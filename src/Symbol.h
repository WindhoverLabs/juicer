/*
 * Symbol.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <stdint.h>
#include <vector>
#include <memory>

#include "ElfFile.h"
#include "Logger.h"
#include "Enumeration.h"
#include "Field.h"

class Field;
class Enumeration;
class ElfFile;

class Symbol
{
public:
	Symbol(ElfFile &elf);
	Symbol(ElfFile &elf, std::string &name, uint32_t byte_size);
	virtual ~Symbol();
	const std::string& getName(void) const;
	void setName(std::string &name);
	uint32_t getByteSize() const ;
	void setByteSize(uint32_t byteSize);
	ElfFile& getElf();
	void setId(uint32_t newId);
	uint32_t getId(void) const;
	Symbol(const Symbol &symbol);
	void addField(Field &inField);
	void addField(std::string& inName,
			uint32_t inByteOffset,
			Symbol &inType,
			uint32_t inMultiplicity,
			bool inLittleEndian,
			uint32_t inBitSize = 0,
			uint32_t inBitOffset = 0);
	void addEnumeration(Enumeration &inEnumeration);
	void addEnumeration(std::string& name, int32_t value);
	std::vector<std::unique_ptr<Enumeration>>& getEnumerations();
	std::vector<std::unique_ptr<Field>>& getFields();
	bool hasBitFields();
    bool isFieldUnique(std::string &name);
    Field* getField(std::string &name) const;
    bool hasFields(void);
    bool isEnumerated(void);

protected:

private:
	ElfFile       &elf;
	std::string  name;
	uint32_t     byte_size;
    Logger       logger;
    uint32_t     id;
    std::vector<std::unique_ptr<Field>> fields;
    std::vector<std::unique_ptr<Enumeration>> enumerations;

};

#endif /* SYMBOL_H_ */
