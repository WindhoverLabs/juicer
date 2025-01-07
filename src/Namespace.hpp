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
    std::string&            getName();
    void                    setName(std::string name);
    Namespace*              getChild();
    void                    setChild(Namespace* child);
    void                    setParent(Namespace* parent);
    Namespace*              getParent();
    std::optional<uint32_t> getId();
    void                    setId(int id);

   private:
    std::string             name;
    // std::list is worth considering here
    Namespace*              parent{nullptr};
    Namespace*              child{nullptr};
    std::optional<uint32_t> id{std::nullopt};
    // std::vector<Symbol*>     symbols;
};

#endif  // NAMESPACE_HPP