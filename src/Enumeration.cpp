/*
 * Enumeration.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "Enumeration.h"

Enumeration::Enumeration(Symbol& inSymbol)
    : symbol{inSymbol},  // @suppress("Symbol is not resolved")
      name{""},
      value{0},
      short_description{""},
      long_description{""}
{
    logger.logDebug("ENUM %s:%s  value:%u  created.", symbol.getName().c_str(), name.c_str(), value);
}

Enumeration::Enumeration(Symbol& inSymbol, std::string& inName, int64_t inValue)
    : symbol{inSymbol},  // @suppress("Symbol is not resolved")
      name{inName},      // @suppress("Symbol is not resolved")
      value{inValue},
      short_description{""},
      long_description{""}
{
    logger.logDebug("ENUM %s:%s  value:%u  created.", symbol.getName().c_str(), name.c_str(), value);
}

Enumeration::Enumeration(Enumeration& inEnumeration)
    : symbol{inEnumeration.getSymbol()},  // @suppress("Symbol is not resolved")
      name{inEnumeration.getName()},      // @suppress("Symbol is not resolved")
      value{inEnumeration.getValue()}
{
}

Enumeration::~Enumeration() {}

std::string& Enumeration::getName() { return name; }

void         Enumeration::setName(std::string& inName)
{
    logger.logDebug("ENUM %s:%s renamed to %s.", symbol.getName().c_str(), name.c_str(), inName.c_str());

    this->name = inName;
}

Symbol& Enumeration::getSymbol() { return symbol; }

int64_t Enumeration::getValue() { return value; }

void    Enumeration::setValue(int64_t inValue)
{
    logger.logDebug("ENUM %s:%s value changed from %i to %i.", symbol.getName().c_str(), name.c_str(), value, inValue);

    this->value = inValue;
}
