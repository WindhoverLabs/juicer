#include "Namespace.hpp"

Namespace::Namespace() {}
Namespace::Namespace(std::string name) { this->name = name; }

void        Namespace::setName(std::string name) { this->name = name; }

std::string Namespace::getName() { return name; }

Namespace*  Namespace::getChild() { return child; }

void        Namespace::setChild(Namespace* child) { this->child = child; }

void        Namespace::setParent(Namespace* parent) { this->parent = parent; }

Namespace*  Namespace::getParent() { return parent; }
