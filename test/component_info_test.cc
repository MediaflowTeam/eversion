/**
 * @file component_info_test.cc
 * @brief Tests component metadata conversion and validation.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#include <eversion/component_info.h>
#include <eversion/runtime_version.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

namespace {

template <typename T>
concept HasToVersion = requires(const T& value) { ToVersion(value); };

static_assert(!HasToVersion<eversion::ComponentVersion>);

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

void ExpectMalformedInEachVersionField(const char* text) {
  const eversion::ComponentVersion malformed_prerelease{
      .prerelease = text,
      .build_metadata = "build.42",
  };
  EXPECT_FALSE(eversion::IsWellFormedComponentVersion(malformed_prerelease));

  const eversion::ComponentVersion malformed_build_metadata{
      .prerelease = "rc.1",
      .build_metadata = text,
  };
  EXPECT_FALSE(
      eversion::IsWellFormedComponentVersion(malformed_build_metadata));
}

struct ExtendedComponentInfo {
  eversion::ComponentInfo prefix;
  const char* homepage = "";
  const char* license = "";
};

TEST(ComponentInfo, ToRuntimeVersionCopiesFields) {
  const eversion::RuntimeVersion version =
      eversion::ToRuntimeVersion(kComponentVersion);
  EXPECT_EQ(version.major, 3U);
  EXPECT_EQ(version.minor, 4U);
  EXPECT_EQ(version.patch, 5U);
  EXPECT_EQ(version.prerelease, "rc.1");
  EXPECT_EQ(version.build_metadata, "git.abc");
  EXPECT_NE(version.prerelease.data(), kComponentVersion.prerelease);
  EXPECT_EQ(version, (eversion::Version{.major = 3,
                                        .minor = 4,
                                        .patch = 5,
                                        .prerelease = "rc.1",
                                        .build_metadata = "git.abc"}));
}

TEST(ComponentInfo, ToRuntimeVersionTreatsNullStringsAsEmpty) {
  constexpr eversion::ComponentVersion kNullStrings{
      .major = 1,
      .minor = 2,
      .patch = 3,
      .prerelease = nullptr,
      .build_metadata = nullptr,
  };

  const eversion::RuntimeVersion version =
      eversion::ToRuntimeVersion(kNullStrings);
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

TEST(ComponentInfo, WellFormedComponentVersionAcceptsSemVerIdentifiers) {
  constexpr eversion::ComponentVersion kEmptyStrings{
      .prerelease = "",
      .build_metadata = "",
  };
  EXPECT_TRUE(eversion::IsWellFormedComponentVersion(kEmptyStrings));

  constexpr eversion::ComponentVersion kPrereleaseAndBuild{
      .prerelease = "rc.1",
      .build_metadata = "build.42",
  };
  EXPECT_TRUE(eversion::IsWellFormedComponentVersion(kPrereleaseAndBuild));

  constexpr eversion::ComponentVersion kNumericAndHyphen{
      .prerelease = "007",
      .build_metadata = "a-b.c",
  };
  EXPECT_TRUE(eversion::IsWellFormedComponentVersion(kNumericAndHyphen));

  constexpr eversion::ComponentVersion kZero{
      .prerelease = "0",
      .build_metadata = "0",
  };
  EXPECT_TRUE(eversion::IsWellFormedComponentVersion(kZero));
}

TEST(ComponentInfo, WellFormedComponentVersionRejectsMalformedIdentifiers) {
  ExpectMalformedInEachVersionField("bad id!! ");
  ExpectMalformedInEachVersionField("a..b");
  ExpectMalformedInEachVersionField(".a");
  ExpectMalformedInEachVersionField("a.");
  ExpectMalformedInEachVersionField("rc.1 ");
  ExpectMalformedInEachVersionField("+x");
  ExpectMalformedInEachVersionField("rc_1");
}

TEST(ComponentInfo, WellFormedComponentVersionRejectsNullStrings) {
  const eversion::ComponentVersion null_prerelease{
      .prerelease = nullptr,
      .build_metadata = "build.42",
  };
  EXPECT_FALSE(eversion::IsWellFormedComponentVersion(null_prerelease));

  const eversion::ComponentVersion null_build_metadata{
      .prerelease = "rc.1",
      .build_metadata = nullptr,
  };
  EXPECT_FALSE(eversion::IsWellFormedComponentVersion(null_build_metadata));
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
static_assert(eversion::IsValidComponentInfo(&kComponentInfo));
static_assert(!eversion::IsValidComponentInfo(nullptr));
static_assert(eversion::IsComponentInfoFieldAvailable(
    &kComponentInfo, offsetof(eversion::ComponentInfo, version),
    sizeof(eversion::ComponentVersion)));

constexpr eversion::ComponentVersion kMalformedComponentVersion{
    .prerelease = "bad id!! ",
    .build_metadata = "build.42",
};
static_assert(eversion::IsWellFormedComponentVersion(kComponentVersion));
static_assert(
    !eversion::IsWellFormedComponentVersion(kMalformedComponentVersion));

static_assert(std::is_standard_layout_v<eversion::ComponentVersion>,
              "ComponentVersion must be standard layout for the fixed ABI v1 "
              "layout; see README 'Runtime Metadata ABI'");
static_assert(std::is_standard_layout_v<eversion::ComponentInfo>,
              "ComponentInfo must be standard layout for the fixed ABI v1 "
              "layout; see README 'Runtime Metadata ABI'");

constexpr bool kIsLp64 = sizeof(void*) == 8U && sizeof(std::size_t) == 8U;

static_assert(
    !kIsLp64 || offsetof(eversion::ComponentVersion, major) == 0U,
    "ComponentVersion::major is at offset 0 in the fixed ABI v1 layout; see "
    "README 'Runtime Metadata ABI'");
static_assert(
    !kIsLp64 || offsetof(eversion::ComponentVersion, minor) == 4U,
    "ComponentVersion::minor is at offset 4 in the fixed ABI v1 layout; see "
    "README 'Runtime Metadata ABI'");
static_assert(
    !kIsLp64 || offsetof(eversion::ComponentVersion, patch) == 8U,
    "ComponentVersion::patch is at offset 8 in the fixed ABI v1 layout; see "
    "README 'Runtime Metadata ABI'");
static_assert(
    !kIsLp64 || offsetof(eversion::ComponentVersion, prerelease) == 16U,
    "ComponentVersion::prerelease is at offset 16 in the fixed ABI v1 layout; "
    "see README 'Runtime Metadata ABI'");
static_assert(
    !kIsLp64 || offsetof(eversion::ComponentVersion, build_metadata) == 24U,
    "ComponentVersion::build_metadata is at offset 24 in the fixed ABI v1 "
    "layout; see README 'Runtime Metadata ABI'");
static_assert(!kIsLp64 || sizeof(eversion::ComponentVersion) == 32U,
              "ComponentVersion is 32 bytes in the fixed ABI v1 layout; see "
              "README 'Runtime Metadata ABI'");

static_assert(!kIsLp64 || offsetof(eversion::ComponentInfo, abi_version) == 0U,
              "ComponentInfo::abi_version is at offset 0 in the fixed ABI v1 "
              "layout; see README 'Runtime Metadata ABI'");
static_assert(!kIsLp64 || offsetof(eversion::ComponentInfo, struct_size) == 8U,
              "ComponentInfo::struct_size is at offset 8 in the fixed ABI v1 "
              "layout; see README 'Runtime Metadata ABI'");
static_assert(!kIsLp64 || offsetof(eversion::ComponentInfo, id) == 16U,
              "ComponentInfo::id is at offset 16 in the fixed ABI v1 layout; "
              "see README 'Runtime Metadata ABI'");
static_assert(!kIsLp64 || offsetof(eversion::ComponentInfo, name) == 24U,
              "ComponentInfo::name is at offset 24 in the fixed ABI v1 layout; "
              "see README 'Runtime Metadata ABI'");
static_assert(!kIsLp64 || offsetof(eversion::ComponentInfo, version) == 32U,
              "ComponentInfo::version is at offset 32 in the fixed ABI v1 "
              "layout; see README 'Runtime Metadata ABI'");
static_assert(!kIsLp64 || sizeof(eversion::ComponentInfo) == 64U,
              "ComponentInfo is 64 bytes in the fixed ABI v1 layout; see "
              "README 'Runtime Metadata ABI'");
static_assert(
    !kIsLp64 || eversion::kComponentInfoRequiredSize == 64U,
    "kComponentInfoRequiredSize is 64 in the fixed ABI v1 layout; see README "
    "'Runtime Metadata ABI'");

}  // namespace
