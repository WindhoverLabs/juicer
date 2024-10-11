/*
 * Symbol.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "Artifact.h"
#include "DimensionList.h"
#include "ElfFile.h"
#include "Encoding.h"
#include "Enumeration.h"
#include "Field.h"
#include "Logger.h"

class Field;
class Enumeration;
class ElfFile;

/**
 *@class Symbol represents a "symbol" in the dwarf.
 *These include intrinsic types and struct types.
 */
class Symbol
{
   public:
    Symbol(ElfFile &elf);
    Symbol(ElfFile &elf, std::string &name, uint32_t byte_size, Artifact);
    Symbol(ElfFile &elf, std::string &name, uint32_t byte_size, Artifact, Symbol &targetSymbol);
    virtual ~Symbol();
    const std::string &getName(void) const;
    void               setName(std::string &name);
    uint32_t           getByteSize() const;
    void               setByteSize(uint32_t byteSize);
    ElfFile           &getElf();
    void               setId(uint32_t newId);
    uint32_t           getId(void) const;
    Symbol(const Symbol &symbol);
    void addField(Field &inField);
    void addField(std::string &inName, std::optional<uint32_t> inByteOffset, Symbol &inType, DimensionList &dimensionList, bool inLittleEndian,
                  uint32_t inBitSize = 0, uint32_t inBitOffset = 0);
    void addField(std::string &inName, uint32_t inByteOffset, Symbol &inType, bool inLittleEndian, uint32_t inBitSize = 0, uint32_t inBitOffset = 0);
    void addEnumeration(Enumeration &inEnumeration);
    void addEnumeration(std::string &name, int32_t value);
    std::vector<std::unique_ptr<Enumeration>> &getEnumerations();
    std::vector<std::unique_ptr<Field>>       &getFields();
    bool                                       hasBitFields();
    bool                                       isFieldUnique(std::string &name);
    Field                                     *getField(std::string &name) const;
    Artifact                                  &getArtifact();

    const std::string                         &getShortDescription() const { return short_description; }

    const std::string                         &getLongDescription() const { return long_description; }

    void                                       setTargetSymbol(Symbol *newTargetSymbol);

    Symbol                                    *getTargetSymbol();

    bool                                       hasTargetSymbol();

    void                                       setEncoding(int newEncoding);

    bool                                       hasEncoding();

    int                                        getEncoding();

   private:
    ElfFile                                  &elf;
    std::string                               name;
    uint32_t                                  byte_size;
    Logger                                    logger;
    uint32_t                                  id;
    std::vector<std::unique_ptr<Field>>       fields;
    std::vector<std::unique_ptr<Enumeration>> enumerations;
    Artifact                                  artifact;
    Symbol                                   *targetSymbol{nullptr};  // This is useful for typedef'd names

    std::string                               short_description;
    std::string                               long_description;

    int                                       encoding{-1};
};

#endif /* SYMBOL_H_ */
