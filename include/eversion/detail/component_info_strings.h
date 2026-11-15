/**
 * @file component_info_strings.h
 * @brief Internal string primitives used by EVersion component metadata APIs.
 * @author EVersion contributors
 * @date 2026-06-05
 * @internal
 */

#ifndef EVERSION_DETAIL_COMPONENT_INFO_STRINGS_H_
#define EVERSION_DETAIL_COMPONENT_INFO_STRINGS_H_

#include <string_view>

/** @cond INTERNAL */
namespace eversion::detail {

/**
 * @internal
 * @brief Converts a nullable C string pointer to a non-null string view.
 */
[[nodiscard]] constexpr std::string_view StringViewOrEmpty(
    const char* text) noexcept {
  return text == nullptr ? std::string_view{} : std::string_view{text};
}

}  // namespace eversion::detail
/** @endcond */

#endif  // EVERSION_DETAIL_COMPONENT_INFO_STRINGS_H_
