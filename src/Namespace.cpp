#include "Namespace.hpp"

Namespace::Namespace() {}
Namespace::Namespace(std::string name)
{
    this->name         = name;
    fullyQualifiedName = name;
}

void Namespace::setName(std::string name)
{
    this->name         = name;
    fullyQualifiedName = name;
}

std::string&            Namespace::getName() { return name; }

Namespace*              Namespace::getChild() { return child; }

void                    Namespace::setChild(Namespace* child) { this->child = child; }

void                    Namespace::setParent(Namespace* parent) { this->parent = parent; }

Namespace*              Namespace::getParent() { return parent; }

std::optional<uint32_t> Namespace::getId() { return id; }
void                    Namespace::setId(int id) { this->id = id; }

void                    Namespace::addChild(Namespace* child)
{
    child->setParent(this);
    children.push_back(child);
    // fullyQualifiedName += separator + child->getName();
}

std::string Namespace::getFullyQualifiedName()
{
    while (parent != nullptr)
    {
        fullyQualifiedName = parent->getName() + separator + fullyQualifiedName;
        parent             = parent->getParent();
    }
    return fullyQualifiedName;
}

std::vector<Namespace*>& Namespace::getChildren() { return children; }
