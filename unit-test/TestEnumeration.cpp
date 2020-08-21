/*
 * TestEnumeration.cpp
 *
 *  Created on: Aug 12, 2020
 *      Author: vagrant
 */
#include "catch.hpp"
#include "Enumeration.h"

/**
 * @brief Unit tests for Enumeration class.
 */

TEST_CASE("Test Enumeration name correctness", "[Enumeration]")
{
    std::string symbolName{"string"};
    std::string moduleName{"ABC"};

    Module symbolModule{moduleName};

    Symbol enumSymbol{symbolModule};
    enumSymbol.setName(symbolName);
    std::string colorName{"Color"};

    Enumeration colorEnumeration{enumSymbol};

    colorEnumeration.setName(colorName);

    REQUIRE(colorEnumeration.getName() == colorName);
}

TEST_CASE("Test Enumeration value correctness", "[Enumeration]")
{
    std::string symbolName{"string"};
    std::string moduleName{"ABC"};

    Module symbolModule{moduleName};

    Symbol enumSymbol{symbolModule};
    enumSymbol.setName(symbolName);
    uint64_t value{714};

    Enumeration colorEnumeration{enumSymbol};

    colorEnumeration.setValue(value);

    REQUIRE(colorEnumeration.getValue() == value);
}

TEST_CASE("Test Constructor Enumeration(Symbol &symbol) "
                    " correctness ", "[Enumeration]")
{
    std::string symbolName{"string"};
    std::string moduleName{"ABC"};

    Module symbolModule{moduleName};

    Symbol enumSymbol{symbolModule};
    enumSymbol.setName(symbolName);

    Enumeration colorEnumeration{enumSymbol};

    REQUIRE(colorEnumeration.getSymbol().getName() == symbolName);
}

TEST_CASE("Test Constructor Enumeration(Symbol &symbol, std::string &name, uint64_t value) "
                    " correctness ", "[Enumeration]")
{
    std::string symbolName{"string"};
    std::string enumName{"Color"};
    uint64_t    value{714};
    std::string moduleName{"ABC"};

    Module      symbolModule{moduleName};

    Symbol enumSymbol{symbolModule};
    enumSymbol.setName(symbolName);

    Enumeration colorEnumeration{enumSymbol, enumName, value};

    REQUIRE(colorEnumeration.getSymbol().getName() == symbolName);
    REQUIRE(colorEnumeration.getName() == enumName);
    REQUIRE(colorEnumeration.getValue() == value);
}
