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
    if (child == nullptr)
    {
        return;
    }

    // NOTE: It might be better to use a map instead of a vector to store children, maybe...

    // Check child is not already in the list using fully qualified name
    for (auto& c : children)
    {
        std::string c_fqn     = c->getFullyQualifiedName();
        std::string child_fqn = child->getFullyQualifiedName();
        if (c_fqn == child_fqn)
        {
            // log error
            // Logger::getInstance().logError("Namespace::addChild: Child already exists in the list");
            return;
        }
    }

    child->setParent(this);
    children.push_back(child);
}

std::string Namespace::getFullyQualifiedName()
{
    // Ensure we don't set the member parent to nullptr
    Namespace* tmpParent = parent;
    fullyQualifiedName   = name;
    while (tmpParent != nullptr)
    {
        fullyQualifiedName = tmpParent->getName() + separator + fullyQualifiedName;
        tmpParent          = tmpParent->getParent();
    }
    return fullyQualifiedName;
}

std::vector<Namespace*>& Namespace::getChildren() { return children; }
