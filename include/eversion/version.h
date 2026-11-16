/**
 * @file version.h
 * @brief Defines EVersion's version view type and comparison APIs.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#ifndef EVERSION_VERSION_H_
#define EVERSION_VERSION_H_

#include <eversion/detail/semver_precedence.h>

#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

/**
 * @brief Version metadata API for EVersion.
 *
 * Provides version views, runtime-owned versions, and ABI-stable component
 * metadata for use across dynamic-library boundaries.
 */
namespace eversion {

struct Version;

/**
 * @brief Formats a version as `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`.
 *
 * @param version Version value to format.
 * @return A canonical string representation of @p version.
 */
[[nodiscard]] std::string ToString(const Version& version);

/**
 * @brief Version view used by generated headers and runtime metadata.
 *
 * `Version` is intentionally lightweight and can be used in constant
 * expressions. The prerelease and build metadata members are non-owning
 * `std::string_view` values; callers must ensure the referenced storage
 * outlives every `Version` view that points at it. Build metadata is kept for
 * display, but ignored by comparison operators.
 */
struct Version {
  /** @brief Major version component. */
  std::uint32_t major = 0;
  /** @brief Minor version component. */
  std::uint32_t minor = 0;
  /** @brief Patch version component. */
  std::uint32_t patch = 0;
  /** @brief Optional prerelease identifier without the leading hyphen. */
  std::string_view prerelease;
  /** @brief Optional build metadata without the leading plus sign. */
  std::string_view build_metadata;

  /**
   * @brief Returns whether this version represents a stable release.
   *
   * EVersion treats `0.x.y` versions and prerelease versions as unstable.
   */
  [[nodiscard]] constexpr bool IsStable() const noexcept {
    return major > 0 && prerelease.empty();
  }

  /**
   * @brief Returns whether the version has prerelease metadata.
   */
  [[nodiscard]] constexpr bool HasPrerelease() const noexcept {
    return !prerelease.empty();
  }

  /**
   * @brief Returns whether the version has build metadata.
   */
  [[nodiscard]] constexpr bool HasBuildMetadata() const noexcept {
    return !build_metadata.empty();
  }

  /**
   * @brief Formats this version as a canonical string.
   */
  [[nodiscard]] std::string ToString() const;

  /**
   * @brief Compares two versions by release precedence.
   *
   * Build metadata is ignored for precedence, so distinct versions that differ
   * only by build metadata are weakly equivalent.
   */
  friend constexpr std::weak_ordering operator<=>(
      const Version& left, const Version& right) noexcept;
  /**
   * @brief Returns whether two versions have equal precedence.
   *
   * Build metadata is ignored for equality.
   */
  friend constexpr bool operator==(const Version& left,
                                   const Version& right) noexcept;
};

[[nodiscard]] constexpr std::weak_ordering operator<=>(
    const Version& left, const Version& right) noexcept {
  const std::strong_ordering core_order =
      std::tie(left.major, left.minor, left.patch) <=>
      std::tie(right.major, right.minor, right.patch);
  if (core_order != std::strong_ordering::equal) {
    return core_order;
  }
  return detail::ComparePrerelease(left.prerelease, right.prerelease);
}

[[nodiscard]] constexpr bool operator==(const Version& left,
                                        const Version& right) noexcept {
  return (left <=> right) == std::weak_ordering::equivalent;
}

inline std::string ToString(const Version& version) {
  std::string text = std::to_string(version.major) + '.' +
                     std::to_string(version.minor) + '.' +
                     std::to_string(version.patch);
  if (version.HasPrerelease()) {
    text += '-';
    text += version.prerelease;
  }
  if (version.HasBuildMetadata()) {
    text += '+';
    text += version.build_metadata;
  }
  return text;
}

inline std::string Version::ToString() const {
  return eversion::ToString(*this);
}

}  // namespace eversion

#endif  // EVERSION_VERSION_H_
