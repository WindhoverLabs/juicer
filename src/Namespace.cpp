#include "Namespace.hpp"

Namespace::Namespace(std::string name) { this->name = name; }

void        Namespace::setName(std::string name) { this->name = name; }

std::string Namespace::getName() { return name; }
