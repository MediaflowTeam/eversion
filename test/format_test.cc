/**
 * @file format_test.cc
 * @brief Tests `std::format` and stream insertion for EVersion values.
 * @author EVersion contributors
 * @date 2026-11-18
 */

#include <eversion/format.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

constexpr eversion::Version V(std::uint32_t major, std::uint32_t minor,
                              std::uint32_t patch,
                              std::string_view prerelease = {},
                              std::string_view build_metadata = {}) {
  return eversion::Version{.major = major,
                           .minor = minor,
                           .patch = patch,
                           .prerelease = prerelease,
                           .build_metadata = build_metadata};
}

eversion::RuntimeVersion R(std::uint32_t major, std::uint32_t minor,
                           std::uint32_t patch, std::string prerelease = {},
                           std::string build_metadata = {}) {
  return eversion::RuntimeVersion{.major = major,
                                  .minor = minor,
                                  .patch = patch,
                                  .prerelease = std::move(prerelease),
                                  .build_metadata = std::move(build_metadata)};
}

TEST(Format, FormatsCanonicalText) {
  const eversion::Version stable = V(1, 2, 3);
  EXPECT_EQ(std::format("{}", stable), stable.ToString());

  const eversion::Version full = V(1, 2, 3, "rc.1", "git.deadbeef");
  EXPECT_EQ(std::format("{}", full), full.ToString());

  const eversion::RuntimeVersion runtime = R(1, 2, 3, "rc.1", "git.deadbeef");
  EXPECT_EQ(std::format("{}", runtime), runtime.ToString());
}

TEST(Format, SupportsStringAlignmentAndFill) {
  const eversion::Version version = V(1, 2, 3);
  EXPECT_EQ(std::format("{:>9}", version), "    1.2.3");
  EXPECT_EQ(std::format("{:<9}|", version), "1.2.3    |");
  EXPECT_EQ(std::format("{:*^11}", version), "***1.2.3***");
}

TEST(Format, StreamInsertionWritesCanonicalText) {
  const eversion::Version version = V(1, 2, 3, "rc.1", "git.deadbeef");
  std::ostringstream version_stream;
  version_stream << version;
  EXPECT_EQ(version_stream.str(), version.ToString());

  const eversion::RuntimeVersion runtime = R(1, 2, 3, "rc.1", "git.deadbeef");
  std::ostringstream runtime_stream;
  runtime_stream << runtime;
  EXPECT_EQ(runtime_stream.str(), runtime.ToString());
}

}  // namespace
