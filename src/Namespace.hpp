#include <string>
#include <vector>

#include "Symbol.h"

class Namespace
{
   public:
    Namespace(std::string name);
    std::string getName();
    void        setName(std::string name);

   private:
    std::string          name;
    std::vector<Symbol&> symbols;
};