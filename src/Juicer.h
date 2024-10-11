/****************************************************************************
 *
 *   Copyright (c) 2017 Windhover Labs, L.L.C. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name Windhover Labs nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

#ifndef JUICER_H_
#define JUICER_H_
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>  /* For open() */
#include <sys/types.h> /* For open() */
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "DimensionList.h"
#include "ElfFile.h"
#include "Enumeration.h"
#include "Field.h"
#include "Logger.h"
#include "Symbol.h"
#include "dwarf.h"
#include "libdwarf.h"

class Field;

/*
 * Macros for error values of Juicer methods and functions.
 */
#define JUICER_OK     0
#define JUICER_ERROR  -1
#define DWARF_VERSION 4

typedef enum
{
    JUICER_OUTPUT_MODE_UNKNOWN = 0,
    JUICER_OUTPUT_MODE_SQLITE  = 1,
    JUICER_OUTPUT_MODE_CCDD    = 2
} JuicerOutputMode_t;

typedef enum
{
    JUICER_ENDIAN_UNKNOWN = 0,
    JUICER_ENDIAN_BIG     = 1,
    JUICER_ENDIAN_LITTLE  = 2
} JuicerEndianness_t;

class IDataContainer;
class ElfFile;
class Symbol;

/**
 *@brief The Juicer class aims to squeeze Elf files from their DWARF
 *and ELF sections. This class should(in the future) be able to get data
 *such as the OS ABI, data structures sizes, endianness, architecture and much more
 *from any ELF file that is given to it.
 *@note please note that this still is a work-in-progress.
 */
class Juicer
{
   public:
    Juicer();
    int parse(std::string& elfFilePath);
    virtual ~Juicer();
    JuicerEndianness_t getEndianness();
    void               setIDC(IDataContainer* idc);

    bool               isExtras() const { return extras; }

    void               setExtras(bool extras) { this->extras = extras; }

    void               setGroupNumber(unsigned int groupNumber) { this->groupNumber = groupNumber; };

    unsigned int       getDwarfVersion();

   private:
    Dwarf_Debug              dbg = 0;
    int                      res = DW_DLV_ERROR;
    Dwarf_Handler            errhand;
    Dwarf_Ptr                errarg = 0;
    int                      readCUList(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Error& error);
    int                      getDieAndSiblings(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die in_die, int in_level);
    Symbol*                  process_DW_TAG_typedef(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die in_die);
    Symbol*                  process_DW_TAG_base_type(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die in_die);
    void                     process_DW_TAG_structure_type(ElfFile& elf, Symbol& symbol, Dwarf_Debug dbg, Dwarf_Die inDie);
    Symbol*                  process_DW_TAG_pointer_type(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die inDie);
    Symbol*                  process_DW_TAG_variable_type(ElfFile& elf, Dwarf_Debug dbg, Dwarf_Die inDie);
    void                     process_DW_TAG_enumeration_type(ElfFile& elf, Symbol& symbol, Dwarf_Debug dbg, Dwarf_Die inDie);
    int                      process_DW_TAG_array_type(ElfFile& elf, Symbol& symbol, Dwarf_Debug dbg, Dwarf_Die inDie);
    void                     process_DW_TAG_union_type(ElfFile& elf, Symbol& symbol, Dwarf_Debug dbg, Dwarf_Die inDie);
    char*                    getFirstAncestorName(Dwarf_Die inDie);
    int                      printDieData(Dwarf_Debug dbg, Dwarf_Die print_me, uint32_t level);
    char*                    dwarfStringToChar(char* dwarfString);
    void                     addBitFields(Dwarf_Die dataMemberDie, Field& dataMemberField);
    void                     addPaddingToStruct(Symbol& symbol);
    void                     addPaddingEndToStruct(Symbol& symbol);
    bool                     isDWARFVersionSupported(Dwarf_Die);
    int                      elfFile = 0;
    Logger                   logger;
    IDataContainer*          idc = 0;
    bool                     isIDCSet(void);
    Symbol*                  getBaseTypeSymbol(ElfFile& elf, Dwarf_Die inDie, DimensionList& multiplicity);
    void                     DisplayDie(Dwarf_Die inDie, uint32_t level);

    std::vector<Dwarf_Die>   getChildrenVector(Dwarf_Debug dbg, Dwarf_Die die);
    int                      getNumberOfSiblingsForDie(Dwarf_Debug dbg, Dwarf_Die die);

    uint32_t                 calcArraySizeForDimension(Dwarf_Debug dbg, Dwarf_Die die);

    DimensionList            getDimList(Dwarf_Debug dbg, Dwarf_Die die);

    std::vector<std::string> dbgSourceFiles{};

    std::string              generateMD5SumForFile(std::string filePath);
    std::string&             getdbgSourceFile(ElfFile& elf, int pathIndex);
    DefineMacro              getDefineMacro(Dwarf_Half macro_operator, Dwarf_Macro_Context mac_context, int i, Dwarf_Unsigned line_number, Dwarf_Unsigned index,
                                            Dwarf_Unsigned offset, const char* macro_string, Dwarf_Half& forms_count, Dwarf_Error& error, Dwarf_Die cu_die, ElfFile& elf);
    DefineMacro              getDefineMacroFromString(std::string macro_string);
    std::map<std::string, std::vector<uint8_t>> getObjDataFromElf(ElfFile* elfFileObj);

    bool                                        extras;

    unsigned int                                groupNumber{0};
    Dwarf_Half                                  dwarfVersion = 0;
};

#endif /* JUICER_H_ */
