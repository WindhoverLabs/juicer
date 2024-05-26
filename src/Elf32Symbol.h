#ifndef ELF32SYMBOL_H
#define ELF32SYMBOL_H

#include <libelf.h>

/**
 * @brief The Elf32Symbol class
 * Extension of Elf32_Sym struct. Makes it easier to map symbol sections
 * to file offsets.
 */

class Elf32Symbol
{
   private:
    Elf32_Sym symbol;
    uint32_t  fileOffset;
    uint32_t  strTableFileOffset;

   public:
    Elf32_Sym getSymbol() const;
    uint32_t  getStrTableFileOffset() const;
    uint32_t  getFileOffset() const;
    Elf32Symbol(Elf32_Sym newSymbol, uint32_t newFileOffset, uint32_t newStrTableFileOffset);
};

#endif  // ELF32SYMBOL_H
