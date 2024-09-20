#include "Elf64Symbol.h"

Elf64Symbol::Elf64Symbol(Elf64_Sym newSymbol, uint32_t newFileOffset, uint32_t newStrTableFileOffset)
    : symbol{newSymbol}, fileOffset{newFileOffset}, strTableFileOffset{newStrTableFileOffset}
{
}

Elf64_Sym Elf64Symbol::getSymbol() const { return symbol; }

uint32_t  Elf64Symbol::getFileOffset() const { return fileOffset; }

uint32_t  Elf64Symbol::getStrTableFileOffset() const { return strTableFileOffset; }
