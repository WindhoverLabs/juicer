/*
 * DefineMacro.h
 *
 *  Created on: Apr 13, 2024
 *      Author: lgomez
 */

#ifndef SRC_DEFINEMACRO_H_
#define SRC_DEFINEMACRO_H_

#include <string>

class DefineMacro
{
   public:
    DefineMacro(std::string inName, std::string inValue);

    const std::string& getName() const;

    const std::string& getValue() const;

    virtual ~DefineMacro();

   private:
    std::string name;
    std::string value;
};

#endif /* SRC_DEFINEMACRO_H_ */
