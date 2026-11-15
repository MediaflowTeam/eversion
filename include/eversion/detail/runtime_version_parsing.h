/**
 * @file runtime_version_parsing.h
 * @brief Internal parsing primitives used by EVersion runtime versions.
 * @author EVersion contributors
 * @date 2026-06-05
 * @internal
 */

#ifndef EVERSION_DETAIL_RUNTIME_VERSION_PARSING_H_
#define EVERSION_DETAIL_RUNTIME_VERSION_PARSING_H_

#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

/** @cond INTERNAL */
namespace eversion::detail {

/** @internal @brief Returns whether @p value is valid in a version identifier.
 */
constexpr bool IsVersionIdentifierCharacter(char value) noexcept {
  return (value >= '0' && value <= '9') || (value >= 'A' && value <= 'Z') ||
         (value >= 'a' && value <= 'z') || value == '-';
}

/** @internal @brief Validates a dot-separated prerelease/build identifier list.
 */
constexpr bool IsVersionIdentifierList(std::string_view text) noexcept {
  if (text.empty()) {
    return false;
  }

  bool previous_was_dot = true;
  for (const char value : text) {
    if (value == '.') {
      if (previous_was_dot) {
        return false;
      }
      previous_was_dot = true;
      continue;
    }
    if (!IsVersionIdentifierCharacter(value)) {
      return false;
    }
    previous_was_dot = false;
  }
  return !previous_was_dot;
}

/** @internal @brief Parses a non-empty decimal uint32 component. */
constexpr std::optional<std::uint32_t> TryParseUint32(
    std::string_view text) noexcept {
  if (text.empty()) {
    return std::nullopt;
  }

  std::uint32_t value = 0;
  for (const char digit : text) {
    if (digit < '0' || digit > '9') {
      return std::nullopt;
    }
    const auto component = static_cast<std::uint32_t>(digit - '0');
    if (value > (std::numeric_limits<std::uint32_t>::max() - component) / 10U) {
      return std::nullopt;
    }
    value = (value * 10U) + component;
  }
  return value;
}

}  // namespace eversion::detail
/** @endcond */

#endif  // EVERSION_DETAIL_RUNTIME_VERSION_PARSING_H_
