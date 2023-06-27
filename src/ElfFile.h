/*
 * Module.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef ElfFile_H
#define ElfFile_H

#include <memory>
#include <stdint.h>
#include <vector>
#include "Logger.h"
#include "Juicer.h"
#include "Field.h"

class Symbol;
class Field;
class Enumeration;

/**
 * The elf class contains an "module" with a user-defined name.
 * It is an aggregation of Symbols, fields, bit fields and enumerations. It also
 * manages those resources for you. You do not need to do any manual memory
 * management with new/delete. Do not allocate this class with the "new" as
 * that will make our design error-prone and result in undefined behavior.
 *
 *@note This is an "owning class". This means that it manages some resources through unique_ptr
 *for this class that will be the elfFiles and symbols object. This means the lifetime
 *of Symbol and ElfFile objects depend on the lifetime of Module; once Module is out of scope
 *then symbols, elfFiles and every unique pointer they contains will be destroyed automatically.
 */
class ElfFile
{
public:
    ElfFile();
	ElfFile(std::string &name);
	ElfFile(const ElfFile &elf);
	virtual ~ElfFile();
    std::vector<std::unique_ptr<Symbol>>&  getSymbols();

	std::string                            getName() const;
	void                                   setName(std::string &name);
	uint32_t                               getId(void) const;
	void                                   setId(uint32_t newId);
	Symbol *                               addSymbol(std::unique_ptr<Symbol> symbol);
	Symbol *                               addSymbol(std::string& name, uint32_t byte_size, Artifact newArtifact);
	std::vector<Field*>                    getFields();
	std::vector<Enumeration*>              getEnumerations();
    bool                                   isSymbolUnique(std::string &name);
    Symbol *                               getSymbol(std::string &name);
	const std::string& getDate() const;
	void setDate(const std::string& date);
	bool isLittleEndian() const;
	void isLittleEndian(bool littleEndian);
	void setMD5(std::string newID);
	std::string getMD5() const;

private:
	std::string md5;
	/**
	 *@note I'm not sure about date being a std::string. I wonder if it'll
	 * become problematic with other formats other than SQLite...dates and
	 * times can be very painful to deal with, especially if we just pass them
	 * around as a std::string. I don't think C++14 has built-in support
	 * for dealing with times and dates. C++20 does. We could probably just
	 * use C-style ctime headers to handle this.
	 */
	std::string date;
	bool        little_endian;
    std::string                            name;
    uint32_t                               id;
    Logger                                 logger;
    std::vector<std::unique_ptr<Symbol>>   symbols;

    void normalizePath(std::string&);
};

#endif /* ElfFile_H_ */
