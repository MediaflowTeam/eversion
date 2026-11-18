/**
 * @file format.h
 * @brief Provides `std::format` and stream insertion support for versions.
 * @author EVersion contributors
 * @date 2026-11-18
 *
 * This header is separate from `version.h` and `runtime_version.h` because
 * including `<format>` makes every translation unit parse substantially more
 * standard-library code (measured at roughly 2.5 times the parse time in the
 * review). Include it only where formatting is actually needed.
 */

#ifndef EVERSION_FORMAT_H_
#define EVERSION_FORMAT_H_

#include <eversion/runtime_version.h>
#include <eversion/version.h>

#include <format>
#include <ostream>
#include <string_view>

namespace eversion {

/**
 * @brief Writes @p version to @p stream in canonical form.
 *
 * The output is exactly `version.ToString()`.
 *
 * @param stream  Output stream to write to.
 * @param version Version to write.
 * @return        @p stream, for chaining.
 */
inline std::ostream& operator<<(std::ostream& stream, const Version& version) {
  stream << version.ToString();
  return stream;
}

/**
 * @brief Writes @p version to @p stream in canonical form.
 *
 * The output is exactly `version.ToString()`.
 *
 * @param stream  Output stream to write to.
 * @param version Runtime version to write.
 * @return        @p stream, for chaining.
 */
inline std::ostream& operator<<(std::ostream& stream,
                                const RuntimeVersion& version) {
  stream << version.ToString();
  return stream;
}

}  // namespace eversion

/** @cond STD_SPECIALIZATIONS */
template <>
struct std::formatter<eversion::Version> : std::formatter<std::string_view> {
  /**
   * @brief Formats @p version using the standard string format specification.
   *
   * Width, alignment, fill, and precision behave as they do for
   * `std::string_view`.
   */
  template <typename FormatContext>
  auto format(const eversion::Version& version, FormatContext& context) const {
    return std::formatter<std::string_view>::format(version.ToString(),
                                                    context);
  }
};

template <>
struct std::formatter<eversion::RuntimeVersion>
    : std::formatter<std::string_view> {
  /**
   * @brief Formats @p version using the standard string format specification.
   *
   * Width, alignment, fill, and precision behave as they do for
   * `std::string_view`.
   */
  template <typename FormatContext>
  auto format(const eversion::RuntimeVersion& version,
              FormatContext& context) const {
    return std::formatter<std::string_view>::format(version.ToString(),
                                                    context);
  }
};
/** @endcond */

#endif  // EVERSION_FORMAT_H_
