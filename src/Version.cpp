#include "Version.h"

#include "Hash.h"

#include <ostream>
#include <regex>
#include <sstream>
#include <string>

using namespace Arbiter;

size_t std::hash<ArbiterSemanticVersion>::operator() (const ArbiterSemanticVersion &version) const
{
  return hashOf(version._major)
    ^ hashOf(version._minor)
    ^ hashOf(version._patch)
    ^ hashOf(version._prereleaseVersion)
    ^ hashOf(version._buildMetadata);
}

size_t std::hash<ArbiterSelectedVersion>::operator() (const ArbiterSelectedVersion &version) const
{
  return hashOf(version._semanticVersion) ^ hashOf(version._metadata);
}

Optional<ArbiterSemanticVersion> ArbiterSemanticVersion::fromString (const std::string &versionString)
{
  // Versions and identifiers cannot have a leading zero.
  #define VERSION "(0|[1-9][0-9]*)"
  #define IDENTIFIER "(?:0|[1-9A-Za-z-][0-9A-Za-z-]*)"
  #define DOTTED_IDENTIFIER "(" IDENTIFIER "(?:\\." IDENTIFIER ")*)"

  std::regex pattern {
    VERSION "\\." VERSION "\\." VERSION
    // prerelease begins with a hyphen followed by a dot separated identifier
    "(?:"   "-" DOTTED_IDENTIFIER ")?"
    // metadata begins with a plus sign followed by a dot separated identifier
    "(?:" "\\+" DOTTED_IDENTIFIER ")?"
  };

  #undef DOTTED_IDENTIFIER
  #undef IDENTIFIER
  #undef VERSION

  std::smatch match;
  if (!std::regex_match(versionString, match, pattern)) {
    return Optional<ArbiterSemanticVersion>();
  }

  unsigned major = std::stoul(match.str(1));
  unsigned minor = std::stoul(match.str(2));
  unsigned patch = std::stoul(match.str(3));

  Optional<std::string> prereleaseVersion;
  Optional<std::string> buildMetadata;

  if (match.length(4) > 0) {
    prereleaseVersion = Optional<std::string>(match.str(4));
  }
  if (match.length(5) > 0) {
    buildMetadata = Optional<std::string>(match.str(5));
  }

  return ArbiterSemanticVersion(major, minor, patch, prereleaseVersion, buildMetadata);
}

std::unique_ptr<Arbiter::Base> ArbiterSemanticVersion::clone () const
{
  return std::make_unique<ArbiterSemanticVersion>(*this);
}

std::ostream &ArbiterSemanticVersion::describe (std::ostream &os) const
{
  os << _major << '.' << _minor << '.' << _patch;

  if (_prereleaseVersion) {
    os << '-' << _prereleaseVersion.value();
  }

  if (_buildMetadata) {
    os << '+' << _buildMetadata.value();
  }

  return os;
}

bool ArbiterSemanticVersion::operator== (const Arbiter::Base &other) const
{
  auto ptr = dynamic_cast<const ArbiterSemanticVersion *>(&other);
  if (!ptr) {
    return false;
  }

  return _major == ptr->_major && _minor == ptr->_minor && _patch == ptr->_patch && _prereleaseVersion == ptr->_prereleaseVersion && _buildMetadata == ptr->_buildMetadata;
}

bool ArbiterSemanticVersion::operator< (const ArbiterSemanticVersion &other) const noexcept
{
  if (_major < other._major) {
    return true;
  } else if (_major > other._major) {
    return false;
  }

  if (_minor < other._minor) {
    return true;
  } else if (_minor > other._minor) {
    return false;
  }

  if (_patch < other._patch) {
    return true;
  } else if (_patch > other._patch) {
    return false;
  }

  if (_prereleaseVersion) {
    if (!other._prereleaseVersion) {
      return true;
    }
    
    std::istringstream left(*_prereleaseVersion);
    std::istringstream right(*other._prereleaseVersion);

    while (true) {
      std::string leftPiece;
      std::string rightPiece;

      if (!std::getline(left, leftPiece, '.')) {
        if (std::getline(right, rightPiece, '.')) {
          // `left` is shorter, therefore lower precedence
          return true;
        } else {
          // Equivalent prerelease versions
          return false;
        }
      } else if (!std::getline(right, rightPiece, '.')) {
        // `right` is shorter, therefore `left` is higher precedence
        return false;
      }

      Optional<unsigned long> leftNum;
      Optional<unsigned long> rightNum;

      try {
        leftNum = std::stoul(leftPiece);
      } catch (std::invalid_argument &ex) {
      }

      try {
        rightNum = std::stoul(rightPiece);
      } catch (std::invalid_argument &ex) {
      }

      if (leftNum) {
        if (rightNum) {
          if (*leftNum < *rightNum) {
            return true;
          } else if (*leftNum > *rightNum) {
            return false;
          }
        } else {
          // `left` has lower precedence because it is numeric
          return true;
        }
      } else if (rightNum) {
        // `left` has higher precedence because it is non-numeric
        return false;
      } else {
        // Compare strings lexically
        if (leftPiece < rightPiece) {
          return true;
        } else if (leftPiece > rightPiece) {
          return false;
        }
      }
    }
  }

  // Build metadata does not participate in precedence.
  return false;
}

std::unique_ptr<Arbiter::Base> ArbiterSelectedVersion::clone () const
{
  return std::make_unique<ArbiterSelectedVersion>(*this);
}

std::ostream &ArbiterSelectedVersion::describe (std::ostream &os) const
{
  if (_semanticVersion) {
    os << *_semanticVersion;
  } else {
    os << "metadata only";
  }

  return os << " (" << _metadata << ")";
}

bool ArbiterSelectedVersion::operator== (const Arbiter::Base &other) const
{
  auto ptr = dynamic_cast<const ArbiterSelectedVersion *>(&other);
  if (!ptr) {
    return false;
  }

  return _semanticVersion == ptr->_semanticVersion && _metadata == ptr->_metadata;
}

bool ArbiterSelectedVersion::operator< (const ArbiterSelectedVersion &other) const
{
  if (_semanticVersion) {
    if (other._semanticVersion) {
      if (*_semanticVersion < *other._semanticVersion) {
        return true;
      }
    } else {
      // Versions with a semantic version component should have higher
      // precedence.
      return false;
    }
  } else if (other._semanticVersion) {
    return true;
  }

  return _metadata < other._metadata;
}

std::unique_ptr<Arbiter::Base> ArbiterSelectedVersionList::clone () const
{
  return std::make_unique<ArbiterSelectedVersionList>(*this);
}

std::ostream &ArbiterSelectedVersionList::describe (std::ostream &os) const
{
  os << "Version list:";

  for (const auto &version : _versions) {
    os << "\n" << version;
  }

  return os;
}

bool ArbiterSelectedVersionList::operator== (const Arbiter::Base &other) const
{
  auto ptr = dynamic_cast<const ArbiterSelectedVersionList *>(&other);
  if (!ptr) {
    return false;
  }

  return _versions == ptr->_versions;
}

std::ostream &operator<< (std::ostream &os, const ArbiterSelectedVersionList &versionList)
{
  os << "Version list:";

  for (const auto &version : versionList._versions) {
    os << "\n" << version;
  }

  return os;
}

ArbiterSemanticVersion *ArbiterCreateSemanticVersion (unsigned major, unsigned minor, unsigned patch, const char *prereleaseVersion, const char *buildMetadata)
{
  return new ArbiterSemanticVersion(
    major,
    minor,
    patch,
    (prereleaseVersion ? Optional<std::string>(prereleaseVersion) : Optional<std::string>()),
    (buildMetadata ? Optional<std::string>(buildMetadata) : Optional<std::string>())
  );
}

ArbiterSemanticVersion *ArbiterCreateSemanticVersionFromString (const char *string)
{
  auto version = ArbiterSemanticVersion::fromString(string);
  if (version) {
    return new ArbiterSemanticVersion(std::move(version.value()));
  } else {
    return nullptr;
  }
}

unsigned ArbiterGetMajorVersion (const ArbiterSemanticVersion *version)
{
  return version->_major;
}

unsigned ArbiterGetMinorVersion (const ArbiterSemanticVersion *version)
{
  return version->_minor;
}

unsigned ArbiterGetPatchVersion (const ArbiterSemanticVersion *version)
{
  return version->_patch;
}

const char *ArbiterGetPrereleaseVersion (const ArbiterSemanticVersion *version)
{
  if (version->_prereleaseVersion) {
    return version->_prereleaseVersion->c_str();
  } else {
    return nullptr;
  }
}

const char *ArbiterGetBuildMetadata (const ArbiterSemanticVersion *version)
{
  if (version->_buildMetadata) {
    return version->_buildMetadata->c_str();
  } else {
    return nullptr;
  }
}

int ArbiterCompareVersionOrdering (const ArbiterSemanticVersion *lhs, const ArbiterSemanticVersion *rhs)
{
  if (*lhs < *rhs) {
    return -1;
  } else if (*lhs > *rhs) {
    return 1;
  } else {
    return 0;
  }
}

ArbiterSelectedVersion *ArbiterCreateSelectedVersion (const ArbiterSemanticVersion *semanticVersion, ArbiterUserValue metadata)
{
  return new ArbiterSelectedVersion(Optional<ArbiterSemanticVersion>::fromPointer(semanticVersion), ArbiterSelectedVersion::Metadata(metadata));
}

const ArbiterSemanticVersion *ArbiterSelectedVersionSemanticVersion (const ArbiterSelectedVersion *version)
{
  return version->_semanticVersion.pointer();
}

const void *ArbiterSelectedVersionMetadata (const ArbiterSelectedVersion *version)
{
  return version->_metadata.data();
}

ArbiterSelectedVersionList *ArbiterCreateSelectedVersionList (const ArbiterSelectedVersion * const *versions, size_t count)
{
  std::vector<ArbiterSelectedVersion> vec;
  vec.reserve(count);

  for (size_t i = 0; i < count; i++) {
    vec.emplace_back(*versions[i]);
  }

  return new ArbiterSelectedVersionList(std::move(vec));
}
