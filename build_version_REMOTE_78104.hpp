// A helper file that may be used to inject the git commit hash and the build time into a binary.
#pragma once

#include <cstdint>

namespace omim
{
namespace build_version
{
namespace git
{
constexpr char const * const kHash = "3d9d5a5e468f5550351d61419f48a3ca7fc7ada7-dirty";
constexpr char const * const kTag = "";
constexpr uint64_t kTimestamp = 1617262582;
constexpr char const * const kProjectName = "omim";
}  // namespace git
}  // namespace build_version
}  // namespace omim
