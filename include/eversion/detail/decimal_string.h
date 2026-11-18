/**
 * @file decimal_string.h
 * @brief Internal decimal formatting primitive for version components.
 * @author EVersion contributors
 * @date 2026-11-18
 *
 * Provides a `constexpr` decimal formatter for `std::uint32_t` so that version
 * types can format their numeric components in constant expressions.
 * @internal
 */

#ifndef EVERSION_DETAIL_DECIMAL_STRING_H_
#define EVERSION_DETAIL_DECIMAL_STRING_H_

#include <algorithm>
#include <cstdint>
#include <string>

/** @cond INTERNAL */
namespace eversion::detail {

/**
 * @brief  Formats @p value as an unsigned decimal string.
 *
 * The result has no leading zeros: `0` becomes `"0"` and `4294967295` becomes
 * `"4294967295"`, matching the usual decimal formatting of these values.
 * Digits are collected from the least significant one, then reversed.
 *
 * @param  value  Value to format.
 * @return        Decimal text for @p value.
 */
constexpr std::string ToDecimalString(std::uint32_t value) {
  constexpr std::uint32_t kDecimalBase = 10U;
  std::string text;
  do {
    text += static_cast<char>('0' + (value % kDecimalBase));
    value /= kDecimalBase;
  } while (value > 0U);
  std::ranges::reverse(text);
  return text;
}

}  // namespace eversion::detail
/** @endcond */

#endif  // EVERSION_DETAIL_DECIMAL_STRING_H_
