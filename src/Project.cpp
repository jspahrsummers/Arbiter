#include "Project.h"

#include "Dependency.h"
#include "Instantiation.h"

#include <algorithm>

namespace Arbiter {

void Project::addInstantiation (const std::shared_ptr<Instantiation> &instantiation)
{
  // TODO: Check for duplicates?

  _instantiations.emplace_back(instantiation);
}

std::shared_ptr<Instantiation> Project::instantiationForVersion (const ArbiterSelectedVersion &version) const
{
  auto it = std::find_if(_instantiations.begin(), _instantiations.end(), [&](const auto &instantiation) {
    return instantiation->_versions.find(version) != instantiation->_versions.end();
  });

  if (it == _instantiations.end()) {
    return nullptr;
  } else {
    return *it;
  }
}

std::shared_ptr<Instantiation> Project::instantiationForDependencies (const std::unordered_set<ArbiterDependency> &dependencies) const
{
  auto it = std::find_if(_instantiations.begin(), _instantiations.end(), [&](const auto &instantiation) {
    return instantiation->dependencies() == dependencies;
  });

  if (it == _instantiations.end()) {
    return nullptr;
  } else {
    return *it;
  }
}

} // namespace Arbiter

ArbiterProjectIdentifier *ArbiterCreateProjectIdentifier (ArbiterUserValue value)
{
  return new ArbiterProjectIdentifier(ArbiterProjectIdentifier::Value(value));
}

const void *ArbiterProjectIdentifierValue (const ArbiterProjectIdentifier *projectIdentifier)
{
  return projectIdentifier->_value.data();
}

std::unique_ptr<Arbiter::Base> ArbiterProjectIdentifier::clone () const
{
  return std::make_unique<ArbiterProjectIdentifier>(*this);
}

std::ostream &ArbiterProjectIdentifier::describe (std::ostream &os) const
{
  return os << "ArbiterProjectIdentifier(" << _value << ")";
}

bool ArbiterProjectIdentifier::operator== (const Arbiter::Base &other) const
{
  auto ptr = dynamic_cast<const ArbiterProjectIdentifier *>(&other);
  if (!ptr) {
    return false;
  }

  return _value == ptr->_value;
}

size_t std::hash<ArbiterProjectIdentifier>::operator() (const ArbiterProjectIdentifier &project) const
{
  return hashOf(project._value);
}
