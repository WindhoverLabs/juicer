/*
 * Artifact.h
 *
 *  Created on: Jun 16, 2023
 *      Author: lgomez
 */

#ifndef SRC_ARTIFACT_H_
#define SRC_ARTIFACT_H_

#include <string>

class Artifact {
	private:
	std::string filePath;

public:
	Artifact();
	virtual ~Artifact();
};

#endif /* SRC_ARTIFACT_H_ */
