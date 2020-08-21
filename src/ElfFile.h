/*
 * ElfFile.h
 *
 *  Created on: Aug 1, 2020
 *      Author: vagrant
 */

#ifndef ELFFILE_H_
#define ELFFILE_H_

#include <stdint.h>
#include "Module.h"
#include "Logger.h"


class ElfFile
{
public:
    ElfFile();
	ElfFile(Module &module);
	ElfFile(Module &module,
			std::string &name,
			uint32_t checksum,
			std::string &date,
			bool little_endian);
	virtual ~ElfFile();
	uint32_t getChecksum() const;
	void setChecksum(uint32_t checksum);
	const std::string& getDate() const;
	void setDate(const std::string& date);
	bool isLittleEndian() const;
	void isLittleEndian(bool littleEndian);
	const Module& getModule() const;
	const std::string& getName() const;
	void setName(const std::string& name);
	ElfFile(const ElfFile &elfFile);

protected:

private:
	const Module      &module;
	std::string name;
	uint32_t    checksum;
	/**
	 *@note I'm not sure about date being a std::string. I wonder if it'll
	 * become problematic with other formats other than SQLite...dates and
	 * times can be very painful to deal with, especially if we just pass them
	 * around as a std::string. I don't think C++14 has built-in support
	 * for dealing with times and dates. C++20 does. We could probably just
	 * use C-style ctime headers to handle this.
	 */
	std::string date;
	bool        little_endian;
	Logger      logger;
};



#endif /* ELFFILE_H_ */
