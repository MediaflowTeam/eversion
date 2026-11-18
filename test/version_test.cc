/**
 * @file version_test.cc
 * @brief Tests the EVersion value type and precedence comparison.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#include <eversion/detail/semver_precedence.h>
#include <eversion/runtime_version.h>
#include <gtest/gtest.h>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

constexpr eversion::Version V(std::uint32_t major, std::uint32_t minor,
                              std::uint32_t patch,
                              std::string_view prerelease = {},
                              std::string_view build_metadata = {}) noexcept {
  return eversion::Version{.major = major,
                           .minor = minor,
                           .patch = patch,
                           .prerelease = prerelease,
                           .build_metadata = build_metadata};
}

eversion::RuntimeVersion R(std::uint32_t major, std::uint32_t minor,
                           std::uint32_t patch, std::string prerelease = {},
                           std::string build_metadata = {}) noexcept {
  return eversion::RuntimeVersion{.major = major,
                                  .minor = minor,
                                  .patch = patch,
                                  .prerelease = std::move(prerelease),
                                  .build_metadata = std::move(build_metadata)};
}

template <typename T>
concept CanCallAsVersion = requires { std::declval<T>().AsVersion(); };

template <typename T>
concept CanCallView = requires { std::declval<T>().View(); };

template <typename T>
concept HasFreeToString = requires(const T& value) { ToString(value); };

static_assert(!HasFreeToString<eversion::Version>);
static_assert(!HasFreeToString<eversion::RuntimeVersion>);

static_assert(!CanCallAsVersion<eversion::RuntimeVersion&>);
static_assert(!CanCallAsVersion<const eversion::RuntimeVersion&>);
static_assert(!CanCallAsVersion<eversion::RuntimeVersion>);
static_assert(!CanCallAsVersion<const eversion::RuntimeVersion>);

static_assert(!CanCallView<eversion::RuntimeVersion&>);
static_assert(!CanCallView<const eversion::RuntimeVersion&>);

static_assert(std::is_aggregate_v<eversion::Version>);
static_assert(std::is_aggregate_v<eversion::RuntimeVersion>);
static_assert(std::is_same_v<decltype(eversion::ToRuntimeVersion(V(1, 0, 0))),
                             eversion::RuntimeVersion>);
static_assert(
    !std::is_convertible_v<eversion::RuntimeVersion, eversion::Version>);
static_assert(
    !std::is_convertible_v<eversion::Version, eversion::RuntimeVersion>);
static_assert(
    std::is_same_v<decltype(std::declval<const eversion::RuntimeVersion&>() <=>
                            std::declval<const eversion::RuntimeVersion&>()),
                   std::weak_ordering>);
static_assert(
    std::is_same_v<decltype(std::declval<const eversion::RuntimeVersion&>() <=>
                            std::declval<const eversion::Version&>()),
                   std::weak_ordering>);
static_assert(
    std::is_same_v<decltype(std::declval<const eversion::Version&>() <=>
                            std::declval<const eversion::RuntimeVersion&>()),
                   std::weak_ordering>);

TEST(Version, EqualityIgnoresBuildMetadata) {
  EXPECT_EQ(V(1, 2, 3), V(1, 2, 3));
  EXPECT_NE(V(1, 2, 3), V(1, 2, 4));
  EXPECT_EQ(V(1, 2, 3, {}, "git.aaa"), V(1, 2, 3, {}, "git.bbb"));
}

TEST(Version, SpaceshipUsesWeakOrderingForMetadataEquivalence) {
  static_assert(
      std::is_same_v<decltype(V(1, 2, 3) <=> V(1, 2, 3)), std::weak_ordering>);

  EXPECT_EQ(V(1, 2, 3, {}, "git.aaa") <=> V(1, 2, 3, {}, "git.bbb"),
            std::weak_ordering::equivalent);
  EXPECT_EQ(V(1, 2, 3, {}, "git.aaa").ToString(), "1.2.3+git.aaa");
  EXPECT_EQ(V(1, 2, 3, {}, "git.bbb").ToString(), "1.2.3+git.bbb");
}

TEST(Version, CoreComponentsCompareLexicographically) {
  EXPECT_LT(V(1, 5, 5), V(2, 5, 5));
  EXPECT_GT(V(2, 5, 5), V(1, 5, 5));
  EXPECT_LT(V(1, 2, 5), V(1, 3, 5));
  EXPECT_GT(V(1, 3, 5), V(1, 2, 5));
  EXPECT_LT(V(1, 2, 3), V(1, 2, 4));
  EXPECT_GT(V(1, 2, 4), V(1, 2, 3));

  EXPECT_LT(V(1, 9, 9), V(2, 0, 0));
  EXPECT_LT(V(1, 1, 9), V(1, 2, 0));

  EXPECT_LT(V(1, 2, 3, "alpha"), V(1, 2, 3));
  EXPECT_GT(V(1, 2, 3), V(1, 2, 3, "alpha"));
}

TEST(Version, PrecedenceOrdering) {
  EXPECT_LT(V(1, 0, 0, "alpha"), V(1, 0, 0));
  EXPECT_LT(V(1, 0, 0, "alpha"), V(1, 0, 0, "alpha.1"));
  EXPECT_LT(V(1, 0, 0, "alpha.1"), V(1, 0, 0, "alpha.beta"));
  EXPECT_LT(V(1, 0, 0, "beta.2"), V(1, 0, 0, "beta.11"));
}

TEST(Version, Stability) {
  EXPECT_FALSE(V(0, 9, 0).IsStable());
  EXPECT_FALSE(V(1, 0, 0, "rc.1").IsStable());
  EXPECT_TRUE(V(1, 0, 0).IsStable());
}

TEST(Version, ToStringIncludesPrereleaseAndBuildMetadata) {
  EXPECT_EQ(V(1, 2, 3, "rc.1", "git.deadbeef").ToString(),
            "1.2.3-rc.1+git.deadbeef");
}

TEST(Version, ToStringFormatsEachOptionalPart) {
  EXPECT_EQ(V(1, 2, 3).ToString(), "1.2.3");
  EXPECT_EQ(V(1, 2, 3, "rc.1").ToString(), "1.2.3-rc.1");
  EXPECT_EQ(V(0, 0, 4294967295U).ToString(), "0.0.4294967295");
}

TEST(Version, RuntimeVersionOwnsStrings) {
  eversion::RuntimeVersion owned{
      .major = 1,
      .minor = 2,
      .patch = 3,
      .prerelease = "rc.1",
      .build_metadata = "git.deadbeef",
  };

  EXPECT_TRUE(owned.HasPrerelease());
  EXPECT_TRUE(owned.HasBuildMetadata());
  EXPECT_FALSE(owned.IsStable());
  EXPECT_EQ(owned.ToString(), "1.2.3-rc.1+git.deadbeef");
}

TEST(Version, ToRuntimeVersionCopiesEveryField) {
  const eversion::RuntimeVersion owned =
      eversion::ToRuntimeVersion(V(1, 2, 3, "rc.1", "git.deadbeef"));
  EXPECT_EQ(owned.major, 1U);
  EXPECT_EQ(owned.minor, 2U);
  EXPECT_EQ(owned.patch, 3U);
  EXPECT_EQ(owned.prerelease, "rc.1");
  EXPECT_EQ(owned.build_metadata, "git.deadbeef");
  EXPECT_EQ(owned.ToString(), "1.2.3-rc.1+git.deadbeef");

  const eversion::RuntimeVersion stable =
      eversion::ToRuntimeVersion(V(1, 0, 0));
  EXPECT_FALSE(stable.HasPrerelease());
  EXPECT_FALSE(stable.HasBuildMetadata());
}

TEST(Version, ToRuntimeVersionOwnsCopiesOfTheViewedStrings) {
  std::string source_prerelease = "rc.1";
  std::string source_build_metadata = "git.deadbeef";
  const eversion::Version view =
      V(1, 2, 3, source_prerelease, source_build_metadata);

  const eversion::RuntimeVersion owned = eversion::ToRuntimeVersion(view);
  EXPECT_NE(owned.prerelease.data(), source_prerelease.data());
  EXPECT_NE(owned.build_metadata.data(), source_build_metadata.data());

  source_prerelease.assign("changed");
  source_build_metadata.assign("changed");
  EXPECT_EQ(owned.prerelease, "rc.1");
  EXPECT_EQ(owned.build_metadata, "git.deadbeef");
}

TEST(Version, ToRuntimeVersionComparesEqualToItsSource) {
  const eversion::Version full = V(1, 2, 3, "rc.1", "git.deadbeef");
  const eversion::RuntimeVersion full_owned = eversion::ToRuntimeVersion(full);
  EXPECT_EQ(full_owned, full);
  EXPECT_EQ(full_owned.ToString(), full.ToString());

  const eversion::Version stable = V(0, 0, 1);
  const eversion::RuntimeVersion stable_owned =
      eversion::ToRuntimeVersion(stable);
  EXPECT_EQ(stable_owned, stable);
  EXPECT_EQ(stable_owned.ToString(), stable.ToString());

  const eversion::Version alpha = V(2, 0, 0, "alpha.1");
  const eversion::RuntimeVersion alpha_owned =
      eversion::ToRuntimeVersion(alpha);
  EXPECT_EQ(alpha_owned, alpha);
  EXPECT_EQ(alpha_owned.ToString(), alpha.ToString());
}

TEST(Version, RuntimeVersionComparesByPrecedence) {
  EXPECT_LT(R(1, 5, 5), R(2, 5, 5));
  EXPECT_GT(R(2, 5, 5), R(1, 5, 5));
  EXPECT_LT(R(1, 2, 5), R(1, 3, 5));
  EXPECT_GT(R(1, 3, 5), R(1, 2, 5));
  EXPECT_LT(R(1, 2, 3), R(1, 2, 4));
  EXPECT_GT(R(1, 2, 4), R(1, 2, 3));

  EXPECT_LT(R(1, 9, 9), R(2, 0, 0));
  EXPECT_LT(R(1, 1, 9), R(1, 2, 0));

  EXPECT_LT(R(1, 0, 0, "alpha"), R(1, 0, 0));
  EXPECT_LT(R(1, 0, 0, "beta.2"), R(1, 0, 0, "beta.11"));

  EXPECT_EQ(R(1, 2, 3, {}, "git.aaa"), R(1, 2, 3, {}, "git.bbb"));
  const std::weak_ordering build_metadata_order =
      R(1, 2, 3, {}, "git.aaa") <=> R(1, 2, 3, {}, "git.bbb");
  EXPECT_EQ(build_metadata_order, std::weak_ordering::equivalent);
  EXPECT_NE(R(1, 2, 3), R(1, 2, 4));
}

TEST(Version, RuntimeVersionComparesWithVersion) {
  EXPECT_LT(R(1, 2, 3), V(1, 2, 4));
  EXPECT_GT(V(1, 2, 4), R(1, 2, 3));

  EXPECT_LT(V(1, 0, 0, "rc.1"), R(1, 0, 0));
  EXPECT_GT(R(1, 0, 0), V(1, 0, 0, "rc.1"));

  EXPECT_EQ(R(1, 2, 3, {}, "git.aaa"), V(1, 2, 3, {}, "git.bbb"));
  EXPECT_EQ(V(1, 2, 3, {}, "git.bbb"), R(1, 2, 3, {}, "git.aaa"));
  EXPECT_NE(R(1, 2, 3), V(1, 2, 3, "rc.1"));
}

TEST(Version, TryParseVersionAcceptsSupportedForms) {
  const eversion::RuntimeVersion full_version =
      eversion::TryParseVersion("1.2.3-rc.001+git-deadbeef")
          .value_or(eversion::RuntimeVersion{});
  EXPECT_EQ(full_version.major, 1U);
  EXPECT_EQ(full_version.minor, 2U);
  EXPECT_EQ(full_version.patch, 3U);
  EXPECT_EQ(full_version.prerelease, "rc.001");
  EXPECT_EQ(full_version.build_metadata, "git-deadbeef");
  EXPECT_EQ(full_version.ToString(), "1.2.3-rc.001+git-deadbeef");

  const eversion::RuntimeVersion stable_version =
      eversion::TryParseVersion("4294967295.0.7")
          .value_or(eversion::RuntimeVersion{});
  EXPECT_EQ(stable_version.major, std::numeric_limits<std::uint32_t>::max());
  EXPECT_EQ(stable_version.minor, 0U);
  EXPECT_EQ(stable_version.patch, 7U);
  EXPECT_TRUE(stable_version.IsStable());

  const eversion::RuntimeVersion build_only_version =
      eversion::TryParseVersion("1.2.3+build.42")
          .value_or(eversion::RuntimeVersion{});
  EXPECT_EQ(build_only_version.prerelease, "");
  EXPECT_EQ(build_only_version.build_metadata, "build.42");

  const eversion::RuntimeVersion hyphenated_metadata_version =
      eversion::TryParseVersion("1.2.3-alpha-1+build-42")
          .value_or(eversion::RuntimeVersion{});
  EXPECT_EQ(hyphenated_metadata_version.prerelease, "alpha-1");
  EXPECT_EQ(hyphenated_metadata_version.build_metadata, "build-42");
}

TEST(Version, LeadingZerosAreReadAsNumbers) {
  const eversion::RuntimeVersion core_leading_zeros =
      eversion::TryParseVersion("01.002.0003")
          .value_or(eversion::RuntimeVersion{});
  EXPECT_EQ(core_leading_zeros.major, 1U);
  EXPECT_EQ(core_leading_zeros.minor, 2U);
  EXPECT_EQ(core_leading_zeros.patch, 3U);
  EXPECT_EQ(core_leading_zeros.ToString(), "1.2.3");

  const eversion::RuntimeVersion prerelease_leading_zeros =
      eversion::TryParseVersion("1.2.3-rc.007")
          .value_or(eversion::RuntimeVersion{});
  EXPECT_EQ(prerelease_leading_zeros.prerelease, "rc.007");
  EXPECT_EQ(prerelease_leading_zeros.ToString(), "1.2.3-rc.007");

  EXPECT_EQ(prerelease_leading_zeros, V(1, 2, 3, "rc.7"));
}

TEST(Version, TryParseVersionRejectsInvalidForms) {
  EXPECT_FALSE(eversion::TryParseVersion("").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3.4").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3-").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3+").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3-rc..1").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3-.alpha").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3-alpha.").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3+build..1").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3+.build").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3+build.").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3-rc_1").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("1.2.3+a+b").has_value());
  EXPECT_FALSE(eversion::TryParseVersion("4294967296.0.0").has_value());
}

TEST(Version, PrereleaseNumericBoundaries) {
  EXPECT_EQ(V(1, 0, 0, "007"), V(1, 0, 0, "7"));
  EXPECT_LT(V(1, 0, 0, "9"), V(1, 0, 0, "123456789012345678901234567890"));
  EXPECT_LT(V(1, 0, 0, "1"), V(1, 0, 0, "alpha"));
  EXPECT_LT(V(1, 0, 0, "1"), V(1, 0, 0));
}

TEST(Version, EqualPrecedenceProducesEqualHashes) {
  const eversion::Version build_metadata_a = V(1, 2, 3, {}, "git.aaa");
  const eversion::Version build_metadata_b = V(1, 2, 3, {}, "git.bbb");
  EXPECT_EQ(build_metadata_a, build_metadata_b);
  EXPECT_EQ(std::hash<eversion::Version>{}(build_metadata_a),
            std::hash<eversion::Version>{}(build_metadata_b));

  const eversion::Version rc_zeros = V(1, 0, 0, "rc.007");
  const eversion::Version rc_seven = V(1, 0, 0, "rc.7");
  EXPECT_EQ(rc_zeros, rc_seven);
  EXPECT_EQ(std::hash<eversion::Version>{}(rc_zeros),
            std::hash<eversion::Version>{}(rc_seven));

  const eversion::Version alpha_zeros = V(1, 0, 0, "alpha.01");
  const eversion::Version alpha_one = V(1, 0, 0, "alpha.1");
  EXPECT_EQ(alpha_zeros, alpha_one);
  EXPECT_EQ(std::hash<eversion::Version>{}(alpha_zeros),
            std::hash<eversion::Version>{}(alpha_one));
}

TEST(Version, RuntimeVersionHashMatchesVersionHash) {
  const eversion::RuntimeVersion runtime = R(1, 0, 0, "rc.007", "x");
  const eversion::Version view = V(1, 0, 0, "rc.7");
  const std::size_t runtime_hash =
      std::hash<eversion::RuntimeVersion>{}(runtime);
  const std::size_t view_hash = std::hash<eversion::Version>{}(view);
  EXPECT_EQ(runtime_hash, view_hash);
}

TEST(Version, UnorderedContainersUsePrecedenceEquality) {
  std::unordered_set<eversion::Version> versions;
  versions.insert(V(1, 2, 3, {}, "git.aaa"));
  EXPECT_NE(versions.find(V(1, 2, 3, {}, "git.bbb")), versions.end());
  versions.insert(V(1, 2, 3, {}, "git.bbb"));
  EXPECT_EQ(versions.size(), 1U);

  std::unordered_map<eversion::RuntimeVersion, int> runtime_versions;
  runtime_versions.emplace(R(1, 2, 3, {}, "git.aaa"), 1);
  EXPECT_NE(runtime_versions.find(R(1, 2, 3, {}, "git.bbb")),
            runtime_versions.end());
  runtime_versions.emplace(R(1, 2, 3, {}, "git.bbb"), 2);
  EXPECT_EQ(runtime_versions.size(), 1U);
}

TEST(Version, DistinctPrecedencesHashDifferently) {
  const std::hash<eversion::Version> hasher;
  std::set<std::size_t> hashes;
  hashes.insert(hasher(V(1, 2, 3)));
  hashes.insert(hasher(V(1, 2, 4)));
  hashes.insert(hasher(V(1, 3, 3)));
  hashes.insert(hasher(V(2, 2, 3)));
  hashes.insert(hasher(V(1, 2, 3, "rc.1")));
  hashes.insert(hasher(V(1, 2, 3, "rc.2")));
  EXPECT_EQ(hashes.size(), 6U);
}

TEST(Version, IsIdenticalToComparesEveryField) {
  const eversion::Version build_metadata_a = V(1, 2, 3, {}, "git.aaa");
  const eversion::Version build_metadata_b = V(1, 2, 3, {}, "git.bbb");
  EXPECT_FALSE(build_metadata_a.IsIdenticalTo(build_metadata_b));

  const eversion::Version rc_zeros = V(1, 0, 0, "rc.007");
  const eversion::Version rc_seven = V(1, 0, 0, "rc.7");
  EXPECT_FALSE(rc_zeros.IsIdenticalTo(rc_seven));

  const eversion::Version alpha_zeros = V(1, 0, 0, "alpha.01");
  const eversion::Version alpha_one = V(1, 0, 0, "alpha.1");
  EXPECT_FALSE(alpha_zeros.IsIdenticalTo(alpha_one));

  const eversion::Version first = V(1, 2, 3, "rc.1", "git.aaa");
  const eversion::Version second = V(1, 2, 3, "rc.1", "git.aaa");
  EXPECT_TRUE(first.IsIdenticalTo(second));
  EXPECT_EQ(first.ToString(), second.ToString());
}

TEST(Version, RuntimeVersionIsIdenticalToComparesEveryField) {
  const eversion::RuntimeVersion first = R(1, 2, 3, "rc.1", "git.aaa");
  const eversion::RuntimeVersion second = R(1, 2, 3, "rc.1", "git.aaa");
  const eversion::RuntimeVersion other_build = R(1, 2, 3, "rc.1", "git.bbb");
  EXPECT_TRUE(first.IsIdenticalTo(second));
  EXPECT_FALSE(first.IsIdenticalTo(other_build));

  const eversion::RuntimeVersion rc_zeros = R(1, 0, 0, "rc.007");
  const eversion::RuntimeVersion rc_seven = R(1, 0, 0, "rc.7");
  EXPECT_TRUE(rc_zeros.IsIdenticalTo(V(1, 0, 0, "rc.007")));
  EXPECT_FALSE(rc_zeros.IsIdenticalTo(V(1, 0, 0, "rc.7")));
  EXPECT_FALSE(rc_zeros.IsIdenticalTo(rc_seven));

  const eversion::Version full = V(1, 2, 3, "rc.1", "git.aaa");
  EXPECT_TRUE(first.IsIdenticalTo(full));
  EXPECT_FALSE(other_build.IsIdenticalTo(full));
}

static_assert(V(1, 2, 3) == V(1, 2, 3));
static_assert(V(1, 0, 0, "alpha") < V(1, 0, 0));
static_assert(V(1, 2, 3, {}, "x") == V(1, 2, 3, {}, "y"));
static_assert((V(1, 2, 3, {}, "x") <=> V(1, 2, 3, {}, "y")) ==
              std::weak_ordering::equivalent);
static_assert(V(1, 0, 0).IsStable());
static_assert(V(1, 5, 5) < V(2, 5, 5));
static_assert(V(1, 2, 5) < V(1, 3, 5));
static_assert(V(1, 2, 3) < V(1, 2, 4));
static_assert(V(1, 9, 9) < V(2, 0, 0));
static_assert(V(1, 1, 9) < V(1, 2, 0));
static_assert(V(1, 2, 3, "alpha") < V(1, 2, 3));
static_assert(!V(1, 0, 0, "rc.007").IsIdenticalTo(V(1, 0, 0, "rc.7")));
static_assert(
    V(1, 2, 3, "rc.1", "git.aaa").IsIdenticalTo(V(1, 2, 3, "rc.1", "git.aaa")));
static_assert(noexcept(std::hash<eversion::Version>{}(V(1, 0, 0))));
static_assert(noexcept(std::hash<eversion::RuntimeVersion>{}(R(1, 0, 0))));
static_assert(V(1, 2, 3).ToString() == "1.2.3");
static_assert(V(0, 0, 4294967295U).ToString() == "0.0.4294967295");
static_assert(
    V(1, 2, 3, "rc.1-a-long-prerelease", "git.deadbeefcafe").ToString() ==
    "1.2.3-rc.1-a-long-prerelease+git.deadbeefcafe");
static_assert(eversion::RuntimeVersion{
                  .major = 1,
                  .minor = 2,
                  .patch = 3,
                  .prerelease = "rc.1-a-long-prerelease",
                  .build_metadata = "git.deadbeefcafe",
              }
                  .ToString() ==
              "1.2.3-rc.1-a-long-prerelease+git.deadbeefcafe");
static_assert(noexcept(eversion::detail::CompareIdentifier("alpha", "beta")));

}  // namespace
