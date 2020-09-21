/*
 * TestSymbol.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: vagrant
 */

#include <ElfObj.h>
#include "catch.hpp"
#include "Field.h"
#include "Enumeration.h"
/**
 *@todo This testing module is not done yet.
 */

TEST_CASE( "Test the correctness of name", "[Symbol]" )
{
    std::string newModuleName{"ABC"};
    std::string newSymbolName{"string"};
    ElfObj      myModule{newModuleName};
    Symbol newSymbol{myModule};

    newSymbol.setName(newSymbolName);

    REQUIRE(newSymbol.getName() == newSymbolName);
}

TEST_CASE( "Test the correctness of byte_size ", "[Symbol]" )
{
    std::string newModuleName{"ABC"};;
    uint32_t    byteSize{8};
    ElfObj      myModule{newModuleName};
    Symbol      newSymbol{myModule};

    newSymbol.setByteSize(byteSize);

    REQUIRE(newSymbol.getByteSize() == byteSize);
}

TEST_CASE( "Test the correctness of id ", "[Symbol]" )
{
    std::string newModuleName{"ABC"};
    std::string newSymbolName{"string"};
    uint32_t    id{123};
    ElfObj      myModule{newModuleName};
    Symbol newSymbol{myModule};

    newSymbol.setId(id);

    REQUIRE(newSymbol.getId() == id);
}

TEST_CASE( "Test addField(Field &inField) method", "[Symbol]" )
{
    std::string newModuleName{"ABC"};
    std::string newSymbolName{"string"};
    const std::string fieldName{"intField"};
    ElfObj      myModule{newModuleName};

    Symbol newSymbol{myModule};

    Symbol newType{myModule};

    Field   newField{newSymbol, newType};
    newField.setName(fieldName);

    newSymbol.addField(newField);

    REQUIRE(newSymbol.getFields().back()->getName() == fieldName);
}

TEST_CASE( "Test addField(std::string& inName, uint32_t inByteOffset, "
                                            "Symbol &inType, uint32_t inMultiplicity, "
                                            "bool inLittleEndian) method ", "[Symbol]" )
{
    std::string fieldName{"intField"};
    std::string            newModuleName{"ABC"};
    std::string            newSymbolName{"string"};
    std::string            newTypeName{"Shape"};
    ElfObj                 myModule{newModuleName};
    bool                     littleEndian = true;
    uint32_t               byteOffset{32};
    uint32_t               multiplicity{2};

    Symbol newSymbol{myModule};
    Symbol newType{myModule};

    newType.setName(newTypeName);

    newSymbol.addField(fieldName, byteOffset, newType, multiplicity, littleEndian);

    REQUIRE(newSymbol.getFields().back()->getName() == fieldName);
    REQUIRE(newSymbol.getFields().back()->getByteOffset() == byteOffset);
    REQUIRE(newSymbol.getFields().back()->getType().getName() == newTypeName);
    REQUIRE(newSymbol.getFields().back()->getMultiplicity() == multiplicity);
    REQUIRE(newSymbol.getFields().back()->isLittleEndian() == littleEndian);
}

TEST_CASE( "Test addEnumeration(Enumeration &inEnumeration); method",
                     "[Symbol]" )
{
    std::string     newModuleName{"ABC"};
    std::string     newSymbolName{"string"};
    std::string     enumName{"Color"};
    ElfObj          myModule{newModuleName};
    Symbol          newSymbol{myModule};
    Enumeration newEnum(newSymbol);

    newEnum.setName(enumName);

    newSymbol.addEnumeration(newEnum);

    REQUIRE(newSymbol.getEnumerations().back()->getName() == enumName);
}

TEST_CASE( "Test addEnumeration(std::string& inName, int32_t inValue); method",
                     "[Symbol]" )
{
    std::string     newModuleName{"ABC"};
    std::string     newSymbolName{"string"};
    std::string     enumName{"Color"};
    uint64_t        enumValue{589};
    ElfObj          myModule{newModuleName};
    Symbol          newSymbol{myModule};
    Enumeration newEnum(newSymbol);

    newEnum.setName(enumName);

    newSymbol.addEnumeration(enumName, enumValue);

    REQUIRE(newSymbol.getEnumerations().back()->getName() == enumName);
    REQUIRE(newSymbol.getEnumerations().back()->getValue() == enumValue);
}

TEST_CASE( "Test getEnumerations() method",
                     "[Symbol]" )
{
    std::string     newModuleName{"ABC"};
    std::string     newSymbolName{"string"};
    std::string     enumName{"Color"};
    uint64_t enumValue{589};
    ElfObj          myModule{newModuleName};
    Symbol          newSymbol{myModule};
    Enumeration newEnum(newSymbol);

    newEnum.setName(enumName);

    newSymbol.addEnumeration(enumName, enumValue);

    REQUIRE(1 == newSymbol.getEnumerations().size());
    REQUIRE(newSymbol.getEnumerations().back()->getName() == enumName);
    REQUIRE(newSymbol.getEnumerations().back()->getValue() == enumValue);
}

TEST_CASE( "Test getFields() method", "[Symbol]" )
{
    std::string newModuleName{"ABC"};
    std::string newSymbolName{"string"};
    const std::string fieldName{"intField"};
    ElfObj      myModule{newModuleName};

    Symbol newSymbol{myModule};
    Symbol newType{myModule};

    Field   newField{newSymbol, newType};
    newField.setName(fieldName);
    newSymbol.addField(newField);

    REQUIRE(1 ==newSymbol.getFields().size());
    REQUIRE(newSymbol.getFields().back()->getName() == fieldName);
}

TEST_CASE( "Test isFieldUnique(std::string &name) method with unique fields", "[Symbol]" )
{
    std::string newModuleName{"ABC"};
    std::string newSymbolName{"string"};
    std::string intFieldName{"intField"};
    std::string floatFieldName{"floatField"};
    ElfObj      myModule{newModuleName};

    Symbol newSymbol{myModule};
    Symbol newType{myModule};

    Field   newIntField{newSymbol, newType};

    newIntField.setName(intFieldName);

    newSymbol.addField(newIntField);

    REQUIRE(newSymbol.isFieldUnique(floatFieldName));
}

/**
 * Test constructors
 */
TEST_CASE( "Test the correctness of constructor Symbol(Module &module) ", "[Symbol]" )
{
    std::string newModuleName{"ABC"};
    ElfObj      myModule{newModuleName};
    Symbol newSymbol{myModule};

    REQUIRE(newSymbol.getElf().getName() == newModuleName);
}

TEST_CASE( "Test the correctness of constructor Symbol(Module &module,"
        "std::string &name,"
        "uint32_t byte_size) ",
        "[Symbol]" )
{
    std::string newModuleName{"ABC"};
    ElfObj      myModule{newModuleName};
    std::string symbolName{"string"};
    uint32_t    byteSize{8};

    Symbol newSymbol{myModule, symbolName, byteSize};

    REQUIRE(newSymbol.getElf().getName() == newModuleName);
    REQUIRE(newSymbol.getName() == symbolName);
    REQUIRE(newSymbol.getByteSize() == byteSize);
}

TEST_CASE( "Test the correctness of constructor Symbol(const Symbol &symbol)", "[Symbol]" )
{
    std::string newModuleName{"ABC"};
    ElfObj      myModule{newModuleName};
    std::string symbolName{"string"};
    uint32_t    byteSize{8};

    Symbol copySymbol{myModule, symbolName, byteSize};

    Symbol constSymbol{copySymbol}; // @suppress("Invalid arguments")

    REQUIRE(constSymbol.getElf().getName() == newModuleName);
    REQUIRE(constSymbol.getName() == symbolName);
    REQUIRE(constSymbol.getByteSize() == byteSize);
}
