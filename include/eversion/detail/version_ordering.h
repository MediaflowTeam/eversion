/**
 * @file version_ordering.h
 * @brief Internal ordering primitives used by EVersion version views.
 * @author EVersion contributors
 * @date 2026-06-05
 * @internal
 */

#ifndef EVERSION_DETAIL_VERSION_ORDERING_H_
#define EVERSION_DETAIL_VERSION_ORDERING_H_

#include <compare>

/** @cond INTERNAL */
namespace eversion::detail {

/** @internal @brief Converts a strong ordering result to weak ordering. */
constexpr std::weak_ordering ToWeakOrdering(
    std::strong_ordering order) noexcept {
  if (order == std::strong_ordering::less) {
    return std::weak_ordering::less;
  }
  if (order == std::strong_ordering::greater) {
    return std::weak_ordering::greater;
  }
  return std::weak_ordering::equivalent;
}

}  // namespace eversion::detail
/** @endcond */

#endif  // EVERSION_DETAIL_VERSION_ORDERING_H_
