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

    void               setName(const std::string& name);

    const std::string& getValue() const;

    void               setValue(const std::string& value);

    virtual ~DefineMacro();

   private:
    std::string name;
    std::string value;
};

#endif /* SRC_DEFINEMACRO_H_ */
