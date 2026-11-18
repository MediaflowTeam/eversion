/**
 * @file runtime_version.h
 * @brief Defines owning runtime version values and parsing APIs.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#ifndef EVERSION_RUNTIME_VERSION_H_
#define EVERSION_RUNTIME_VERSION_H_

#include <eversion/detail/runtime_version_parsing.h>
#include <eversion/version.h>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace eversion {

/**
 * @brief Owning version value for parsed or otherwise runtime-owned metadata.
 *
 * `RuntimeVersion` owns its strings and does not hand out `Version` views, so
 * a stored value cannot dangle. Compare it directly with another
 * `RuntimeVersion` or with a `Version`; use `ToRuntimeVersion()` to copy a
 * `Version` into one.
 */
struct RuntimeVersion {
  /** @brief Major version component. */
  std::uint32_t major = 0;
  /** @brief Minor version component. */
  std::uint32_t minor = 0;
  /** @brief Patch version component. */
  std::uint32_t patch = 0;
  /** @brief Optional prerelease identifier without the leading hyphen. */
  std::string prerelease;
  /** @brief Optional build metadata without the leading plus sign. */
  std::string build_metadata;

  /** @brief Returns whether this version represents a stable release. */
  [[nodiscard]] bool IsStable() const noexcept { return View().IsStable(); }

  /** @brief Returns whether the version has prerelease metadata. */
  [[nodiscard]] bool HasPrerelease() const noexcept {
    return !prerelease.empty();
  }

  /** @brief Returns whether the version has build metadata. */
  [[nodiscard]] bool HasBuildMetadata() const noexcept {
    return !build_metadata.empty();
  }

  /** @brief Formats this version as a canonical string. */
  [[nodiscard]] std::string ToString() const { return View().ToString(); }

  /**
   * @brief Compares this runtime version with @p other by release precedence.
   *
   * Uses the same rules as `Version`; build metadata is ignored.
   *
   * @param other Runtime version to compare with.
   */
  [[nodiscard]] std::weak_ordering operator<=>(
      const RuntimeVersion& other) const noexcept {
    return View() <=> other.View();
  }

  /**
   * @brief Returns whether this runtime version and @p other have equal
   * precedence.
   *
   * Build metadata is ignored for equality.
   *
   * @param other Runtime version to compare with.
   */
  [[nodiscard]] bool operator==(const RuntimeVersion& other) const noexcept {
    return View() == other.View();
  }

  /**
   * @brief Compares this runtime version with @p other by release precedence.
   *
   * The reversed form, with the `Version` on the left, is also available.
   * Build metadata is ignored.
   *
   * @param other Version to compare with.
   */
  [[nodiscard]] std::weak_ordering operator<=>(
      const Version& other) const noexcept {
    return View() <=> other;
  }

  /**
   * @brief Returns whether this runtime version and @p other have equal
   * precedence.
   *
   * The reversed form, with the `Version` on the left, is also available.
   * Build metadata is ignored for equality.
   *
   * @param other Version to compare with.
   */
  [[nodiscard]] bool operator==(const Version& other) const noexcept {
    return View() == other;
  }

 private:
  // Builds a view of this object for the comparisons and formatting above.
  // The view must not outlive the full expression that uses it.
  [[nodiscard]] Version View() const noexcept {
    return Version{
        .major = major,
        .minor = minor,
        .patch = patch,
        .prerelease = prerelease,
        .build_metadata = build_metadata,
    };
  }
};

/**
 * @brief Copies a version view into an owning runtime version.
 *
 * Use this to keep a `Version` beyond the lifetime of the storage it views.
 *
 * @param version Version view to copy.
 * @return A runtime version that owns copies of the prerelease and build
 * metadata strings.
 */
[[nodiscard]] inline RuntimeVersion ToRuntimeVersion(const Version& version) {
  return RuntimeVersion{
      .major = version.major,
      .minor = version.minor,
      .patch = version.patch,
      .prerelease = std::string{version.prerelease},
      .build_metadata = std::string{version.build_metadata},
  };
}

/**
 * @brief Parses a version string into an owning version value.
 *
 * Accepted syntax is `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`. Numeric core
 * components must fit in `std::uint32_t`; prerelease and build metadata use the
 * same dot-separated identifier grammar as `eversion_declare()`. Leading zeros
 * are read as numbers: `01.2.3` parses as 1.2.3, and a numeric prerelease
 * identifier such as `007` keeps its text but compares equal to `7`.
 *
 * @param text Version string to parse.
 * @return Parsed runtime version, or `std::nullopt` when @p text is invalid.
 */
[[nodiscard]] inline std::optional<RuntimeVersion> TryParseVersion(
    std::string_view text) {
  const std::size_t plus = text.find('+');
  std::string_view build_metadata;
  if (plus != std::string_view::npos) {
    if (text.find('+', plus + 1U) != std::string_view::npos) {
      return std::nullopt;
    }
    build_metadata = text.substr(plus + 1U);
    if (!detail::IsVersionIdentifierList(build_metadata)) {
      return std::nullopt;
    }
    text = text.substr(0, plus);
  }

  const std::size_t hyphen = text.find('-');
  std::string_view prerelease;
  if (hyphen != std::string_view::npos) {
    prerelease = text.substr(hyphen + 1U);
    if (!detail::IsVersionIdentifierList(prerelease)) {
      return std::nullopt;
    }
    text = text.substr(0, hyphen);
  }

  const std::size_t first_dot = text.find('.');
  if (first_dot == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t second_dot = text.find('.', first_dot + 1U);
  if (second_dot == std::string_view::npos ||
      text.find('.', second_dot + 1U) != std::string_view::npos) {
    return std::nullopt;
  }

  const std::optional<std::uint32_t> major =
      detail::TryParseUint32(text.substr(0, first_dot));
  const std::optional<std::uint32_t> minor = detail::TryParseUint32(
      text.substr(first_dot + 1U, second_dot - first_dot - 1U));
  const std::optional<std::uint32_t> patch =
      detail::TryParseUint32(text.substr(second_dot + 1U));
  if (!major.has_value() || !minor.has_value() || !patch.has_value()) {
    return std::nullopt;
  }

  return RuntimeVersion{
      .major = *major,
      .minor = *minor,
      .patch = *patch,
      .prerelease = std::string{prerelease},
      .build_metadata = std::string{build_metadata},
  };
}

}  // namespace eversion

#endif  // EVERSION_RUNTIME_VERSION_H_
