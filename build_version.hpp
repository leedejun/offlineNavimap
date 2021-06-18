// A helper file that may be used to inject the git commit hash and the build time into a binary.
#pragma once

#include <cstdint>

namespace omim
{
namespace build_version
{
namespace git
{
constexpr char const * const kHash = "3b09faccc0f0884ad6cd34e5b4e88f551446ae82-dirty";
constexpr char const * const kTag = "";
constexpr uint64_t kTimestamp = 1617355646;
constexpr char const * const kProjectName = "omim";
}  // namespace git
}  // namespace build_version
}  // namespace omim
