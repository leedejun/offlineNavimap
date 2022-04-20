// A helper file that may be used to inject the git commit hash and the build time into a binary.
#pragma once

#include <cstdint>

namespace omim
{
namespace build_version
{
namespace git
{
constexpr char const * const kHash = "962c9c5018417856b79d8cd624c2848ba6be9e31-dirty";
constexpr char const * const kTag = "";
constexpr uint64_t kTimestamp = 1650421125;
constexpr char const * const kProjectName = "omim";
}  // namespace git
}  // namespace build_version
}  // namespace omim
