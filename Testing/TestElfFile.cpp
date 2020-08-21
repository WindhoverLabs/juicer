/*
 * TestElfFile.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: vagrant
 */

#include "catch.hpp"
#include "ElfFile.h"

/**
 * @brief Unit tests for ElfFile class.
 *
 *@note Not 100% sure about the structure of these tests,
 *especially the constructors testing. Will re-evaluate.
 */

TEST_CASE("Test module correctness", "[ElfFile]")
{
    std::string newModuleName{"ABC"};
    Module      myModule{newModuleName};
    ElfFile        elffy{myModule};

    /**
     * @note Not sure if just testing the module's name is sufficient to test
     * the correctness of this module, within the context of ElfFile.
     */
    REQUIRE(elffy.getModule().getName() == newModuleName);
}

TEST_CASE("Test checksum correctness", "[ElfFile]")
{
    ElfFile elffy{};
    uint32_t checksum{1033};
    elffy.setChecksum(checksum);

    REQUIRE(elffy.getChecksum() == checksum);
}

TEST_CASE("Test name correctness", "[ElfFile]")
{
    ElfFile elffy{};
    std::string elffyName{"elffy.o"};
    elffy.setName(elffyName);

    REQUIRE(elffy.getName() == elffyName);
}

TEST_CASE("Test date correctness", "[ElfFile]")
{
    ElfFile elffy{};
    std::string elffyDate{"1 January 1970"};
    elffy.setDate(elffyDate);

    REQUIRE(elffy.getDate() == elffyDate);
}

TEST_CASE("Test little_endian correctness", "[ElfFile]")
{
    ElfFile elffy{};
    bool littleEndian = false;
    elffy.isLittleEndian(littleEndian);

    REQUIRE(elffy.isLittleEndian() == littleEndian);
}

/**
 *
 *@note Not sure if writing the constructor's signature into the name
 * is a good way of naming this test. The reason I do this is that I want
 * to make it clear that this test is testing a very specific constructor.
 * Will re-evaluate.
 */
TEST_CASE( "Test constructor   ElfFile(Module &module,\
                                                   std::string &name,\
                                                   uint32_t checksum,\
                                                   std::string &date,\
                                                   bool little_endian);\
                                                     correctness", "[ElfFile] " ) {
    std::string newModuleName{"ABC"};
    Module      myModule{newModuleName};
    std::string elfName = "Elffy.o";
   uint32_t     checksum = 93;
   bool           littleEndian = true;
   std::string  date{"1 January 1970"};

    ElfFile        elffy{myModule, elfName, checksum, date, littleEndian };

    /**
     * @note Not sure if just testing the module's name is sufficient to test
     * the correctness of this module, within the context of ElfFile.
     */
    REQUIRE(elffy.getModule().getName() == newModuleName);
    REQUIRE(elffy.getName() == elfName);
    REQUIRE(elffy.getDate() == date);
    REQUIRE(elffy.isLittleEndian() == littleEndian);
}

/**
 *
 *@note Not sure if writing the constructor's signature into the name
 * is a good way of naming this test. The reason I do this is that I want
 * to make it clear that this test is testing a very specific constructor.
 * Will re-evaluate.
 */
TEST_CASE( "Test constructor  ElfFile(Module &module) correctness", "[ElfFile] " ) {
    std::string newModuleName{"ABC"};
    Module      myModule{newModuleName};

    ElfFile        elffy{myModule };

    /**
     * @note Not sure if just testing the module's name is sufficient to test
     * the correctness of this module, within the context of ElfFile.
     */
    REQUIRE(elffy.getModule().getName() == newModuleName);
}
