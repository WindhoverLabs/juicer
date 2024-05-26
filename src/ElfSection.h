#ifndef ELFSECTION_H
#define ELFSECTION_H
#include <libelf.h>
#include <stdint.h>

#include <string>

class ElfSection
{
   public:
    ElfSection();

   private:
    std::string sectionName;
    uint32_t    fileOffset;
    uint32_t    sectionType;  // See elf.h for legal values of sh_type

    Elf32_Shdr  elf32_Header;
};

#endif  // ELFSECTION_H
