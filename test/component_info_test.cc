/**
 * @file component_info_test.cc
 * @brief Tests component metadata conversion and validation.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#include <eversion/component_info.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

namespace {

constexpr eversion::ComponentVersion kComponentVersion{
    .major = 3,
    .minor = 4,
    .patch = 5,
    .prerelease = "rc.1",
    .build_metadata = "git.abc",
};

constexpr eversion::ComponentInfo kComponentInfo{
    .abi_version = eversion::kComponentInfoAbiVersion,
    .struct_size = sizeof(eversion::ComponentInfo),
    .id = "test.component",
    .name = "Test Component",
    .version = kComponentVersion,
};

struct ExtendedComponentInfo {
  eversion::ComponentInfo prefix;
  const char* homepage = "";
  const char* license = "";
};

TEST(ComponentInfo, ToVersionConvertsFields) {
  const eversion::Version version = eversion::ToVersion(kComponentVersion);
  EXPECT_EQ(version.major, 3U);
  EXPECT_EQ(version.minor, 4U);
  EXPECT_EQ(version.patch, 5U);
  EXPECT_EQ(version.prerelease, "rc.1");
  EXPECT_EQ(version.build_metadata, "git.abc");
}

TEST(ComponentInfo, ToVersionTreatsNullStringsAsEmptyViews) {
  constexpr eversion::ComponentVersion kNullStrings{
      .major = 1,
      .minor = 2,
      .patch = 3,
      .prerelease = nullptr,
      .build_metadata = nullptr,
  };

  const eversion::Version version = eversion::ToVersion(kNullStrings);
  EXPECT_EQ(version.major, 1U);
  EXPECT_EQ(version.minor, 2U);
  EXPECT_EQ(version.patch, 3U);
  EXPECT_TRUE(version.prerelease.empty());
  EXPECT_TRUE(version.build_metadata.empty());
}

TEST(ComponentInfo, ValidationAcceptsCurrentAndExtendedSizes) {
  EXPECT_TRUE(eversion::IsValidComponentInfo(&kComponentInfo));

  eversion::ComponentInfo required_size = kComponentInfo;
  required_size.struct_size = eversion::kComponentInfoRequiredSize;
  EXPECT_TRUE(eversion::IsValidComponentInfo(&required_size));

  eversion::ComponentInfo extended_size = kComponentInfo;
  extended_size.struct_size = sizeof(eversion::ComponentInfo) + 64U;
  EXPECT_TRUE(eversion::IsValidComponentInfo(&extended_size));
}

TEST(ComponentInfo, NewerHostAcceptsRequiredV1Prefix) {
  eversion::ComponentInfo older_producer = kComponentInfo;
  older_producer.struct_size = eversion::kComponentInfoRequiredSize;

  EXPECT_TRUE(eversion::IsValidComponentInfo(&older_producer));
  EXPECT_TRUE(eversion::IsComponentInfoFieldAvailable(
      &older_producer, offsetof(eversion::ComponentInfo, version),
      sizeof(eversion::ComponentVersion)));
  EXPECT_FALSE(eversion::IsComponentInfoFieldAvailable(
      &older_producer, eversion::kComponentInfoRequiredSize, 1U));
}

TEST(ComponentInfo, OlderHostAcceptsNewerProducerWithAppendedFields) {
  ExtendedComponentInfo newer_producer{
      .prefix = kComponentInfo,
      .homepage = "https://example.invalid/component",
      .license = "MIT",
  };
  newer_producer.prefix.struct_size = sizeof(ExtendedComponentInfo);

  EXPECT_TRUE(eversion::IsValidComponentInfo(&newer_producer.prefix));
  EXPECT_TRUE(eversion::IsComponentInfoFieldAvailable(
      &newer_producer.prefix, offsetof(ExtendedComponentInfo, homepage),
      sizeof(newer_producer.homepage)));
  EXPECT_TRUE(eversion::IsComponentInfoFieldAvailable(
      &newer_producer.prefix, offsetof(ExtendedComponentInfo, license),
      sizeof(newer_producer.license)));
}

TEST(ComponentInfo, ValidationRejectsInvalidMetadata) {
  EXPECT_FALSE(eversion::IsValidComponentInfo(nullptr));

  eversion::ComponentInfo wrong_abi = kComponentInfo;
  wrong_abi.abi_version = eversion::kComponentInfoAbiVersion + 1U;
  EXPECT_FALSE(eversion::IsValidComponentInfo(&wrong_abi));

  eversion::ComponentInfo wrong_size = kComponentInfo;
  wrong_size.struct_size = eversion::kComponentInfoRequiredSize - 1U;
  EXPECT_FALSE(eversion::IsValidComponentInfo(&wrong_size));

  eversion::ComponentInfo missing_id = kComponentInfo;
  missing_id.id = nullptr;
  EXPECT_FALSE(eversion::IsValidComponentInfo(&missing_id));

  eversion::ComponentInfo missing_build = kComponentInfo;
  missing_build.version.build_metadata = nullptr;
  EXPECT_FALSE(eversion::IsValidComponentInfo(&missing_build));
}

TEST(ComponentInfo, FieldAvailabilityUsesStructSize) {
  eversion::ComponentInfo info = kComponentInfo;
  info.struct_size = eversion::kComponentInfoRequiredSize;
  EXPECT_TRUE(eversion::IsComponentInfoFieldAvailable(
      &info, offsetof(eversion::ComponentInfo, version),
      sizeof(eversion::ComponentVersion)));
  EXPECT_FALSE(eversion::IsComponentInfoFieldAvailable(
      &info, eversion::kComponentInfoRequiredSize, 1U));
  EXPECT_FALSE(eversion::IsComponentInfoFieldAvailable(nullptr, 0U, 1U));

  info.struct_size = sizeof(eversion::ComponentInfo) + 16U;
  EXPECT_TRUE(eversion::IsComponentInfoFieldAvailable(
      &info, sizeof(eversion::ComponentInfo), 16U));
}

static_assert(std::is_same_v<decltype(eversion::ComponentInfo{}.version),
                             eversion::ComponentVersion>);
static_assert(eversion::ToVersion(kComponentVersion) ==
              eversion::Version{.major = 3,
                                .minor = 4,
                                .patch = 5,
                                .prerelease = "rc.1",
                                .build_metadata = "other.build"});
static_assert(eversion::IsValidComponentInfo(&kComponentInfo));
static_assert(!eversion::IsValidComponentInfo(nullptr));
static_assert(eversion::IsComponentInfoFieldAvailable(
    &kComponentInfo, offsetof(eversion::ComponentInfo, version),
    sizeof(eversion::ComponentVersion)));

}  // namespace
