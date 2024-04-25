/*
 * Variable.h
 *
 *  Created on: Apr 25, 2024
 *      Author: lgomez
 */

#ifndef SRC_VARIABLE_H_
#define SRC_VARIABLE_H_

#include <string>

#include "Symbol.h"

class Variable
{
   public:
    Variable(std::string newName, Symbol& newType, ElfFile& newElf);
    virtual ~Variable();

    const std::string& getName() const;
    const Symbol&      getType() const;

    const ElfFile&     getElf() const;

    const std::string& getShortDescription() const;

    const std::string& getLongDescription() const;

   private:
    std::string name;
    Symbol&     type;
    ElfFile&    elf;

    std::string short_description{"TODO"};
    std::string long_description{"TODO"};
};

#endif /* SRC_VARIABLE_H_ */
