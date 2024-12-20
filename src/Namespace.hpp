#ifndef NAMESPACE_HPP
#define NAMESPACE_HPP

#include <string>
#include <vector>

#include "Symbol.h"

class Namespace
{
   public:
    Namespace();
    Namespace(std::string name);
    std::string getName();
    void        setName(std::string name);
    Namespace*  getChild();
    void        setChild(Namespace* child);
    void        setParent(Namespace* parent);
    Namespace*  getParent();

   private:
    std::string name;
    // std::list is owrth considering here
    Namespace*  parent;
    Namespace*  child;
    uint32_t    id;
    // std::vector<Symbol*>     symbols;
};

#endif  // NAMESPACE_HPP