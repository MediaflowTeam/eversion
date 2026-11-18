/**
 * @file version_hash.h
 * @brief Hashing primitives for version precedence equality.
 * @author EVersion contributors
 * @date 2026-11-18
 *
 * Provides hash building blocks that follow the same SemVer 2.0.0 precedence
 * rules as `Version::operator==`, so that versions which compare equal also
 * hash equally. Build metadata is deliberately excluded because it is ignored
 * by precedence comparison.
 *
 * These primitives are header-only so that `std::hash` specializations can use
 * them without adding a separate translation unit.
 * @internal
 */

#ifndef EVERSION_DETAIL_VERSION_HASH_H_
#define EVERSION_DETAIL_VERSION_HASH_H_

#include <eversion/detail/semver_precedence.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

/** @cond INTERNAL */
namespace eversion::detail {

/**
 * @brief  Mixing constant used by CombineHash().
 *
 * The value is the fractional part of the golden ratio scaled to a
 * pointer-sized integer. On 32-bit targets the explicit cast truncates the
 * constant, which is the usual boost-style behavior.
 */
inline constexpr std::size_t kHashMixConstant =
    static_cast<std::size_t>(0x9e3779b97f4a7c15ULL);

/**
 * @brief  Mixes @p value into @p seed.
 *
 * Boost-style hash combination with a named mixing constant.
 *
 * @param  seed   Accumulated hash state.
 * @param  value  Hash value to mix in.
 * @return        Combined hash state.
 */
constexpr std::size_t CombineHash(std::size_t seed,
                                  std::size_t value) noexcept {
  return seed ^ (value + kHashMixConstant + (seed << 6U) + (seed >> 2U));
}

/**
 * @brief  Hashes the SemVer precedence of a version.
 *
 * Mixes the three core numbers and then each dot-separated prerelease
 * identifier. Numeric identifiers are hashed after stripping leading zeros, so
 * `rc.007` and `rc.7` hash equally. The same parsing primitives as
 * `ComparePrerelease()` are used so that hashing and `operator==` cannot drift
 * apart. Build metadata is not part of precedence and is never mixed in.
 *
 * @param  major       Major version component.
 * @param  minor       Minor version component.
 * @param  patch       Patch version component.
 * @param  prerelease  Prerelease identifier list without the leading hyphen.
 * @return             Hash of the version's precedence.
 */
inline std::size_t HashVersionPrecedence(std::uint32_t major,
                                         std::uint32_t minor,
                                         std::uint32_t patch,
                                         std::string_view prerelease) noexcept {
  std::size_t hash = 0;
  hash = CombineHash(hash, static_cast<std::size_t>(major));
  hash = CombineHash(hash, static_cast<std::size_t>(minor));
  hash = CombineHash(hash, static_cast<std::size_t>(patch));

  while (!prerelease.empty()) {
    std::string_view identifier = FirstIdentifier(prerelease);
    if (IsNumericIdentifier(identifier)) {
      identifier = StripLeadingZeros(identifier);
    }
    hash = CombineHash(hash, std::hash<std::string_view>{}(identifier));
    prerelease = RemoveFirstIdentifier(prerelease);
  }

  return hash;
}

}  // namespace eversion::detail
/** @endcond */

#endif  // EVERSION_DETAIL_VERSION_HASH_H_
