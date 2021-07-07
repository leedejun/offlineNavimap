// A helper file that may be used to inject the git commit hash and the build time into a binary.
#pragma once

#include <cstdint>

namespace omim
{
namespace build_version
{
namespace git
{
constexpr char const * const kHash = "b8cfe0dc736478275f911b03f4f8c2547e14b08f-dirty";
constexpr char const * const kTag = "";
constexpr uint64_t kTimestamp = 1624083767;
constexpr char const * const kProjectName = "omim";
}  // namespace git
}  // namespace build_version
}  // namespace omim
