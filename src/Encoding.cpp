#include "Encoding.h"

Encoding::Encoding(std::string name) : name{name} {}

Encoding::Encoding() {}

std::string& Encoding::getName() { return name; }

void         Encoding::setId(int newId) { id = newId; }

int          Encoding::getId() const { return id; }
