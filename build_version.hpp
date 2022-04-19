// A helper file that may be used to inject the git commit hash and the build time into a binary.
#pragma once

#include <cstdint>

namespace omim
{
namespace build_version
{
namespace git
{
constexpr char const * const kHash = "dd8e9777d5360e00874bc7e83df9ea4b3a1fa584-dirty";
constexpr char const * const kTag = "";
constexpr uint64_t kTimestamp = 1623837837;
constexpr char const * const kProjectName = "omim";
}  // namespace git
}  // namespace build_version
}  // namespace omim
