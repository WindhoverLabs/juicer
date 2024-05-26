/*
 * DefineMacro.cpp
 *
 *  Created on: Apr 13, 2024
 *      Author: lgomez
 */

#include "DefineMacro.h"

DefineMacro::DefineMacro(std::string inName, std::string inValue) : name{inName}, value{inValue}  // @suppress("Symbol is not resolved")
{
    // TODO Auto-generated constructor stub
}

const std::string& DefineMacro::getName() const { return name; }

void               DefineMacro::setName(const std::string& name) { this->name = name; }

const std::string& DefineMacro::getValue() const { return value; }

void               DefineMacro::setValue(const std::string& value) { this->value = value; }

DefineMacro::~DefineMacro()
{
    // TODO Auto-generated destructor stub
}
