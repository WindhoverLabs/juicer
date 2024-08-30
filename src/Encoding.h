#ifndef ENCODING_H
#define ENCODING_H

#include <string>

class Encoding
{
   public:
    Encoding(std::string name);
    Encoding();

   private:
    std::string name;
    int         id;
};

#endif  // ENCODING_H
