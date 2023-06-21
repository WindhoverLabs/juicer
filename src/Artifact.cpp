/*
 * Artifact.cpp
 *
 *  Created on: Jun 16, 2023
 *      Author: lgomez
 */

#include "Artifact.h"

Artifact::Artifact(ElfFile& elf, std::string path):
				   elf{elf}, filePath{path.c_str()}
{
	// TODO Auto-generated constructor stub

}


Artifact::Artifact(ElfFile& elf):
				   elf{elf}
{

}

Artifact::Artifact(const Artifact &artifact):
		 elf{artifact.elf}, filePath{artifact.filePath}, crc{artifact.getCRC()}
{

}

Artifact::~Artifact() {
	// TODO Auto-generated destructor stub
}

void Artifact::setId(uint32_t newID)
{
	id = newID;
}
uint32_t Artifact::getId()
{
	return id;
}


std::string Artifact::getFilePath() const
{
	return filePath;
}
ElfFile& Artifact::getElf()
{
	return elf;
}

void Artifact::setCRC(uint32_t newCRC)
{
	crc = newCRC;
}
uint32_t Artifact::getCRC() const
{
	return crc;
}

