/*
 * Module.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef MODULE_H_
#define MODULE_H_

#include <memory>
#include <stdint.h>
#include <vector>
#include "Logger.h"
#include "Juicer.h"

class ElfFile;
class Symbol;
class Field;
class Enumeration;
class BitField;

/**
 * The module class contains a "module" with a user-defined name.
 * It is an aggregation of ElfFiles and Symbols. It also
 * manages those resources for you. You do not need to do any manual memory
 * management with new/delete. Do not allocate this class with the "new" as
 * that will make our design error-prone and result in undefined behavior.
 *
 *@note This is an "owning class". This means that it manages some resources through unique_ptr
 *for this class that will be the elfFiles and symbols object. This means the lifetime
 *of Symbol and ElfFile objects depend on the lifetime of Module; once Module is out of scope
 *then symbols, elfFiles and every unique pointer they contains will be destroyed automatically.
 */
class Module
{
public:
    Module();
	Module(std::string &name);
	Module(const Module &module);
	virtual ~Module();
    std::vector<std::unique_ptr<ElfFile>>& getElfFiles();
    std::vector<std::unique_ptr<Symbol>>&  getSymbols();

	std::string                            getName() const;
	void                                   setName(std::string &name);
	uint32_t                               getId(void) const;
	void                                   setId(uint32_t newId);
	void                                   addElfFile(ElfFile& elfFile);
	void                                   addElfFile(std::string& name, uint32_t checksum,std::string& date,bool little_endian);
	Symbol *                               addSymbol(std::unique_ptr<Symbol> symbol);
	Symbol *                               addSymbol(std::string& name, uint32_t byte_size);
	std::vector<Field*>                    getFields();
	std::vector<Enumeration*>              getEnumerations();
	std::vector<BitField*>                 getBitFields();
    bool                                   isSymbolUnique(std::string &name);
    Symbol *                               getSymbol(std::string &name);
    bool                                   isLittleEndian();

private:
    std::string                            name;
    uint32_t                               id;
    Logger                                 logger;
    std::vector<std::unique_ptr<ElfFile>>  elfFiles;
    std::vector<std::unique_ptr<Symbol>>   symbols;
};

#endif /* MODULE_H_ */
