// A helper file that may be used to inject the git commit hash and the build time into a binary.
#pragma once

#include <cstdint>

namespace omim
{
namespace build_version
{
namespace git
{
constexpr char const * const kHash = "290a05b66227351fce5fd3b3324d2f44dbd0b276-dirty";
constexpr char const * const kTag = "";
constexpr uint64_t kTimestamp = 1617248501;
constexpr char const * const kProjectName = "omim";
}  // namespace git
}  // namespace build_version
}  // namespace omim
