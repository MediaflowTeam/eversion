/**
 * @file version_declare_test.cc
 * @brief Tests generated target version constants.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#include <eversion/version.h>
#include <eversion/version_info.h>
#include <eversion_demo/version_info.h>
#include <gtest/gtest.h>

namespace {

TEST(VersionDeclare, EVersionDogfoodsGeneratedVersionHeader) {
  EXPECT_EQ(eversion::version_info::kVersionMajor, 0U);
  EXPECT_EQ(eversion::version_info::kVersionMinor, 1U);
  EXPECT_EQ(eversion::version_info::kVersionPatch, 0U);
  EXPECT_EQ(eversion::version_info::kVersionString, "0.1.0");
}

TEST(VersionDeclare, GeneratedConstantsMatch) {
  EXPECT_EQ(eversion_demo::version_info::kVersionMajor, 1U);
  EXPECT_EQ(eversion_demo::version_info::kVersionMinor, 2U);
  EXPECT_EQ(eversion_demo::version_info::kVersionPatch, 3U);
  EXPECT_EQ(eversion_demo::version_info::kVersionPrerelease, "rc.1");
  EXPECT_EQ(eversion_demo::version_info::kVersionBuildMetadata, "git.deadbeef");
  EXPECT_EQ(eversion_demo::version_info::kVersionString,
            "1.2.3-rc.1+git.deadbeef");
}

TEST(VersionDeclare, GeneratedVersionParticipatesInComparison) {
  EXPECT_EQ(eversion_demo::version_info::kVersion,
            (eversion::Version{1, 2, 3, "rc.1", "other.build"}));
}

static_assert(eversion::version_info::kVersionString == "0.1.0");
static_assert(eversion_demo::version_info::kVersionMajor == 1U);
static_assert(eversion_demo::version_info::kVersionString ==
              "1.2.3-rc.1+git.deadbeef");
static_assert(eversion_demo::version_info::kVersion <
              eversion::Version{.major = 1,
                                .minor = 2,
                                .patch = 3,
                                .prerelease = {},
                                .build_metadata = {}});

}  // namespace
