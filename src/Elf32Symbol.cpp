#include "Elf32Symbol.h"

Elf32Symbol::Elf32Symbol(Elf32_Sym newSymbol, uint32_t newFileOffset, uint32_t newStrTableFileOffset)
    : symbol{newSymbol}, fileOffset{newFileOffset}, strTableFileOffset{newStrTableFileOffset}
{
}

Elf32_Sym Elf32Symbol::getSymbol() const { return symbol; }

uint32_t  Elf32Symbol::getFileOffset() const { return fileOffset; }

uint32_t  Elf32Symbol::getStrTableFileOffset() const { return strTableFileOffset; }
