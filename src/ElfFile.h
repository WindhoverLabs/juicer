/*
 * Module.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef ElfFile_H
#define ElfFile_H

#include <libelf.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "DefineMacro.h"
#include "Elf32Symbol.h"
#include "Encoding.h"
#include "Field.h"
#include "Juicer.h"
#include "Logger.h"
#include "Variable.h"
#include "dwarf.h"

class Symbol;
class Field;
class Enumeration;
class Variable;

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
    std::vector<std::unique_ptr<Symbol>>              &getSymbols();

    std::string                                        getName() const;
    void                                               setName(std::string &name);
    uint32_t                                           getId(void) const;
    void                                               setId(uint32_t newId);
    Symbol                                            *addSymbol(std::unique_ptr<Symbol> symbol);
    Symbol                                            *addSymbol(std::string &name, uint32_t byte_size, Artifact newArtifact);
    Symbol                                            *addSymbol(std::string &inName, uint32_t inByteSize, Artifact newArtifact, Symbol *targetSymbol);
    std::vector<Field *>                               getFields();
    std::vector<Enumeration *>                         getEnumerations();
    bool                                               isSymbolUnique(std::string &name);
    Symbol                                            *getSymbol(std::string &name);
    const std::string                                 &getDate() const;
    void                                               setDate(const std::string &date);
    bool                                               isLittleEndian() const;
    void                                               isLittleEndian(bool littleEndian);
    void                                               setMD5(std::string newID);
    std::string                                        getMD5() const;
    void                                               addDefineMacro(std::string name, std::string value);
    void                                               addDefineMacro(DefineMacro newMacro);

    const std::vector<DefineMacro>                    &getDefineMacros() const;

    const std::map<std::string, std::vector<uint8_t>> &getInitializedSymbolData() const;

    void                                               setInitializedSymbolData(const std::map<std::string, std::vector<uint8_t>> &initializedSymbolData);
    void                                               addVariable(Variable newVariable);
    const std::vector<Variable>                       &getVariables() const;
    void                                               addElf32SectionHeader(Elf32_Shdr newVariable);
    std::vector<Elf32_Shdr>                            getElf32Headers() const;

    void                                               addElf32SymbolTableSymbol(Elf32Symbol newSymbol);
    std::vector<Elf32Symbol>                           getElf32SymbolTable() const;

    std::vector<Encoding>                              getDWARFEncodings();

    Encoding                                          &getDWARFEncoding(int encoding);

   private:
    std::string                                 md5;
    /**
     *@note I'm not sure about date being a std::string. I wonder if it'll
     * become problematic with other formats other than SQLite...dates and
     * times can be very painful to deal with, especially if we just pass them
     * around as a std::string. I don't think C++14 has built-in support
     * for dealing with times and dates. C++20 does. We could probably just
     * use C-style ctime headers to handle this.
     */
    std::string                                 date;
    bool                                        little_endian;
    std::string                                 name;
    uint32_t                                    id;
    Logger                                      logger;
    std::vector<std::unique_ptr<Symbol>>        symbols;

    void                                        normalizePath(std::string &);
    std::vector<DefineMacro>                    defineMacros{};
    std::vector<Variable>                       variables{};

    /**
     *  Data that is already initialized at compile time that is will be loaded by loader/linker into memory.
     */
    std::map<std::string, std::vector<uint8_t>> initializedSymbolData{};

    std::vector<Elf32_Shdr>                     elf32Headers{};
    std::vector<Elf64_Shdr>                     elf64Headers{};

    std::vector<Elf32Symbol>                    elf32SymbolTable{};
    std::vector<Elf32_Sym>                      elf32StringsTable{};

    /**
     * @note This list of encodings can be found in dwarf.h or
     *  in DWARF5 specification document section 5.1.1 titled "Base Type Encodings"
     */
    /**
     * @brief encodingMap maps the DWARF macros to strings.
     */
    std::map<int, Encoding>                     encodingsMap = {
        {DW_ATE_address, Encoding{"DW_ATE_address"}},
        {DW_ATE_boolean, Encoding{"DW_ATE_boolean"}},
        {DW_ATE_complex_float, Encoding{"DW_ATE_complex_float"}},
        {DW_ATE_float, Encoding{"DW_ATE_float"}},
        {DW_ATE_signed, Encoding{"DW_ATE_signed"}},
        {DW_ATE_signed_char, Encoding{"DW_ATE_signed_char"}},
        {DW_ATE_unsigned, Encoding{"DW_ATE_unsigned"}},
        {DW_ATE_unsigned_char, Encoding{"DW_ATE_unsigned_char"}},
        {DW_ATE_imaginary_float, Encoding{"DW_ATE_imaginary_float"}},
        {DW_ATE_packed_decimal, Encoding{"DW_ATE_packed_decimal"}},
        {DW_ATE_numeric_string, Encoding{"DW_ATE_numeric_string"}},
        {DW_ATE_edited, Encoding{"DW_ATE_edited"}},
        {DW_ATE_signed_fixed, Encoding{"DW_ATE_signed_fixed"}},
        {DW_ATE_unsigned_fixed, Encoding{"DW_ATE_unsigned_fixed"}},
        {DW_ATE_decimal_float, Encoding{"DW_ATE_decimal_float"}},
        {DW_ATE_UTF, Encoding{"DW_ATE_UTF"}},
        {DW_ATE_UCS, Encoding{"DW_ATE_UCS"}},
        {DW_ATE_ASCII, Encoding{"DW_ATE_ASCII"}},

    };

    Encoding &getDWARFEncoding();
};

#endif /* ElfFile_H_ */
