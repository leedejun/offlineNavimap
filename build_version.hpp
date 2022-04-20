// A helper file that may be used to inject the git commit hash and the build time into a binary.
#pragma once

#include <cstdint>

namespace omim
{
namespace build_version
{
namespace git
{
constexpr char const * const kHash = "a2120c8a5c4435f66102491f3f7b56b1b1133d0a-dirty";
constexpr char const * const kTag = "";
constexpr uint64_t kTimestamp = 1650368336;
constexpr char const * const kProjectName = "omim";
}  // namespace git
}  // namespace build_version
}  // namespace omim
