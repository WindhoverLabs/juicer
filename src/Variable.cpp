/*
 * Variable.cpp
 *
 *  Created on: Apr 25, 2024
 *      Author: lgomez
 */

#include "Variable.h"

Variable::Variable(std::string newName, Symbol& newType, ElfFile& newElf) : name{newName}, type{newType}, elf{newElf}
{
    // TODO Auto-generated constructor stub
}

const std::string& Variable::getName() const { return name; }

const Symbol&      Variable::getType() const { return type; }

const ElfFile&     Variable::getElf() const { return elf; }

const std::string& Variable::getShortDescription() const { return short_description; }
const std::string& Variable::getLongDescription() const { return long_description; }

Variable::~Variable()
{
    // TODO Auto-generated destructor stub
}
