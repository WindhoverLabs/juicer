/*
 * Enumeration.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef ENUMERATION_H_
#define ENUMERATION_H_

#include <stdint.h>
#include "Symbol.h"
#include "Logger.h"

class Symbol;

class Enumeration
{
public:
	Enumeration(Symbol &symbol);
    Enumeration(Symbol &symbol, std::string &name, int64_t value);
	virtual ~Enumeration();
	Enumeration(Enumeration &enumeration);
	std::string& getName();
	void setName(std::string& name);
	Symbol& getSymbol();
    int64_t getValue();
    void setValue(int64_t value);

private:
	Symbol&     symbol;
	std::string name;
	/**
	 *@note I think, since these structures are dependent on the dwarf data
	 *types such as Dwarf_Unsigned, we should use those data types directly
	 *to avoid casting and ambiguity when passing data around.
	 */
    int64_t     value;
	Logger      logger;
};

#endif /* ENUMERATION_H_ */
