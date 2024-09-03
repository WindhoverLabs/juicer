#ifndef ENCODING_H
#define ENCODING_H

#include <string>

/**
 * @brief The Encoding class
 * Simple wrapper for DWARF5 encoding macros.
 * Read DWARF5 specification document section 5.1.1 titled "Base Type Encodings"
 * for more details.
 */

class Encoding
{
   public:
    Encoding(std::string name);
    Encoding();
    std::string& getName();
    void         setId(int);
    int          getId() const;

   private:
    std::string name;
    int         id;
};

#endif  // ENCODING_H
