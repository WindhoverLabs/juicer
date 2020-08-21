/*
 * ElfFile.cpp
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#include "ElfFile.h"


ElfFile::ElfFile():module{}{}

ElfFile::ElfFile(Module& inModule) :
    module{inModule}, // @suppress("Symbol is not resolved")
    name{""},
    checksum{0},
    date{""},
    little_endian{false}
{
    logger.logDebug("ELF %s@%s  checksum:0x%08x  date:%s  endian:%s  created.", name.c_str(), module.getName().c_str(), checksum, date.c_str(), little_endian ? "LE" : "BE");
}

/**
 *@todo Matt, not sure we talked about this but we could just delete this
 *constructor and the error of using a copy constructor will be caught by the
 *compiler before even running juicer.
 */
ElfFile::ElfFile(Module& inModule, std::string &inName, uint32_t inChecksum,
		std::string &inDate, bool inLittleEndian) :
		    module{inModule}, // @suppress("Symbol is not resolved")
			name{inName}, // @suppress("Symbol is not resolved")
		    checksum{inChecksum},
		    date{inDate}, // @suppress("Symbol is not resolved")
		    little_endian{inLittleEndian}
{
    logger.logDebug("ELF %s@%s  checksum:0x%08x  date:%s  endian:%s  created.", name.c_str(), module.getName().c_str(), checksum, date.c_str(), little_endian ? "LE" : "BE");
}



ElfFile::~ElfFile()
{
}



uint32_t ElfFile::getChecksum() const
{
	return checksum;
}



void ElfFile::setChecksum(uint32_t inChecksum)
{
    logger.logDebug("ELF %s checksum changed from 0x%08x to 0x%08x.", name.c_str(), checksum, inChecksum);

	this->checksum = inChecksum;
}



const std::string& ElfFile::getDate() const
{
	return date;
}



void ElfFile::setDate(const std::string& inDate)
{
    logger.logDebug("ELF %s date changed from %s to %s.", name.c_str(), date.c_str(), inDate.c_str());

	this->date = inDate;
}



bool ElfFile::isLittleEndian() const
{
	return little_endian;
}

/**
 *@brief Sets the endianness of this ElfFile object.
 */
void ElfFile::isLittleEndian(bool inLittleEndian)
{
    logger.logDebug("ELF %s endian changed from %s to %s.", name.c_str(), little_endian ? "LE" : "BE", inLittleEndian ? "LE" : "BE");

	little_endian = inLittleEndian;
}



const Module& ElfFile::getModule() const
{
	return module;
}



const std::string& ElfFile::getName() const
{
	return name;
}



void ElfFile::setName(const std::string& inName)
{
    logger.logDebug("ELF renamed %s to %s.", name.c_str(), inName.c_str());

	this->name = inName;
}



ElfFile::ElfFile(const ElfFile& inElfFile) :
	module{inElfFile.getModule()}, // @suppress("Symbol is not resolved")
	name{inElfFile.getName()}, // @suppress("Symbol is not resolved")
	checksum{inElfFile.getChecksum()},
	date{inElfFile.getDate()}, // @suppress("Symbol is not resolved")
	little_endian{inElfFile.isLittleEndian()}
{
    logger.logError("Cannot call ElfFile copy constructor.");
}
