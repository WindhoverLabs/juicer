/*
 * Artifact.h
 *
 *  Created on: Jun 16, 2023
 *      Author: lgomez
 */

#ifndef SRC_ARTIFACT_H_
#define SRC_ARTIFACT_H_

#include <string>

class ElfFile;


class Artifact {
	private:
	std::string filePath{};
	ElfFile       &elf;
	uint32_t     id;
	std::string md5{"0"};

public:
	Artifact(ElfFile &elf, std::string path);
	Artifact(ElfFile &elf);

	Artifact(const Artifact &artifact);

	void setId(uint32_t newID);
	uint32_t getId();

	void setMD5(std::string newID);
	std::string getMD5() const;


	std::string  getFilePath() const;
	ElfFile       &getElf();

	~Artifact();
};

#endif /* SRC_ARTIFACT_H_ */
