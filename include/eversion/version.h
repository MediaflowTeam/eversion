/**
 * @file version.h
 * @brief Defines EVersion's version view type and comparison APIs.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#ifndef EVERSION_VERSION_H_
#define EVERSION_VERSION_H_

#include <eversion/detail/decimal_string.h>
#include <eversion/detail/semver_precedence.h>
#include <eversion/detail/version_hash.h>

#include <compare>
#include <cstddef>
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

/**
 * @brief Version view used by generated headers and runtime metadata.
 *
 * `Version` is intentionally lightweight and can be used in constant
 * expressions. The prerelease and build metadata members are non-owning
 * `std::string_view` values; callers must ensure the referenced storage
 * outlives every `Version` view that points at it. Build metadata is kept for
 * display, but ignored by comparison operators. `std::hash` is also based on
 * precedence: it ignores build metadata and agrees with `operator==`.
 * `std::format` and `operator<<` are available from `<eversion/format.h>`.
 *
 * The string members are not validated: any text can be stored in them. The
 * places that produce these values are responsible for their validity.
 * `eversion_declare()` checks identifiers at configure time,
 * `TryParseVersion()` rejects invalid text, and a `ComponentVersion` that comes
 * from a plugin can be checked with `IsWellFormedComponentVersion()`. Invalid
 * text does not cause undefined behavior; it only takes part in comparisons in
 * byte order.
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
   * @brief Formats this version as `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`.
   *
   * The result can be evaluated in a constant expression, for example
   * `static_assert(version.ToString() == "1.2.3")`. Like every
   * `constexpr std::string`, the result can only be used within the same
   * constant evaluation and cannot be stored in a `constexpr` variable.
   *
   * @return A canonical string representation of this version.
   */
  [[nodiscard]] constexpr std::string ToString() const {
    std::string text = detail::ToDecimalString(major) + '.' +
                       detail::ToDecimalString(minor) + '.' +
                       detail::ToDecimalString(patch);
    if (HasPrerelease()) {
      text += '-';
      text += prerelease;
    }
    if (HasBuildMetadata()) {
      text += '+';
      text += build_metadata;
    }
    return text;
  }

  /**
   * @brief Compares this version with @p other by release precedence.
   *
   * Build metadata is ignored for precedence, so distinct versions that differ
   * only by build metadata are weakly equivalent.
   *
   * @param other Version to compare with.
   */
  [[nodiscard]] constexpr std::weak_ordering operator<=>(
      const Version& other) const noexcept {
    const std::strong_ordering core_order =
        std::tie(major, minor, patch) <=>
        std::tie(other.major, other.minor, other.patch);
    if (core_order != std::strong_ordering::equal) {
      return core_order;
    }
    return detail::ComparePrerelease(prerelease, other.prerelease);
  }

  /**
   * @brief Returns whether this version and @p other have equal precedence.
   *
   * Build metadata is ignored for equality.
   *
   * @param other Version to compare with.
   */
  [[nodiscard]] constexpr bool operator==(const Version& other) const noexcept {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  /**
   * @brief Returns whether this version and @p other are identical field by
   * field.
   *
   * Unlike `operator==`, which compares precedence and ignores build metadata,
   * this compares every field. The prerelease and build metadata strings are
   * compared byte for byte, so `rc.007` is not identical to `rc.7`, and `+a` is
   * not identical to `+b`. Two versions are identical exactly when
   * `ToString()` produces the same text.
   *
   * @param other Version to compare with.
   */
  [[nodiscard]] constexpr bool IsIdenticalTo(
      const Version& other) const noexcept {
    return major == other.major && minor == other.minor &&
           patch == other.patch && prerelease == other.prerelease &&
           build_metadata == other.build_metadata;
  }
};

}  // namespace eversion

/** @cond STD_SPECIALIZATIONS */
template <>
struct std::hash<eversion::Version> {
  /**
   * @brief Returns a hash consistent with `eversion::Version::operator==`.
   *
   * Build metadata is ignored, so versions that differ only by build metadata
   * hash equally.
   */
  std::size_t operator()(const eversion::Version& version) const noexcept {
    return eversion::detail::HashVersionPrecedence(
        version.major, version.minor, version.patch, version.prerelease);
  }
};
/** @endcond */

#endif  // EVERSION_VERSION_H_
