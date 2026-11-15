/**
 * @file component_info.h
 * @brief Defines ABI-stable component metadata for dynamically loaded plugins.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#ifndef EVERSION_COMPONENT_INFO_H_
#define EVERSION_COMPONENT_INFO_H_

#include <eversion/detail/component_info_strings.h>
#include <eversion/version.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace eversion {

/** @brief ABI version for the `ComponentInfo` component metadata contract. */
inline constexpr std::uint32_t kComponentInfoAbiVersion = 1U;
/** @brief Default exported symbol name for plugin metadata entry points. */
inline constexpr std::string_view kPluginInfoSymbol = "eversion_plugin_info";

/**
 * @brief ABI-stable component version representation for dynamic-library
 * boundaries.
 *
 * This type avoids C++ library types at the plugin boundary. Convert it to
 * `eversion::Version` with `ToVersion()` after validation.
 */
struct ComponentVersion {
  /** @brief Major version component. */
  std::uint32_t major = 0;
  /** @brief Minor version component. */
  std::uint32_t minor = 0;
  /** @brief Patch version component. */
  std::uint32_t patch = 0;
  /** @brief Optional prerelease identifier, or an empty string. */
  const char* prerelease = "";
  /** @brief Optional build metadata, or an empty string. */
  const char* build_metadata = "";
};

/**
 * @brief ABI-stable component metadata returned by a plugin entry point.
 *
 * Fields may only be appended to the end of this structure within the same ABI
 * version. Consumers must use `struct_size` to guard reads of appended fields.
 */
struct ComponentInfo {
  /** @brief Component metadata ABI version. */
  std::uint32_t abi_version = 0;
  /** @brief Size of this structure as compiled by the producer. */
  std::size_t struct_size = 0;
  /** @brief Stable machine-readable component or plugin identifier. */
  const char* id = "";
  /** @brief Human-readable component or plugin name. */
  const char* name = "";
  /** @brief Component or plugin version. */
  ComponentVersion version;
};

/**
 * @brief Minimum `ComponentInfo::struct_size` accepted by ABI v1 hosts.
 *
 * Future optional fields must be appended after this required prefix and read
 * only after checking `IsComponentInfoFieldAvailable()`.
 */
inline constexpr std::size_t kComponentInfoRequiredSize =
    offsetof(ComponentInfo, version) + sizeof(ComponentVersion);

/**
 * @brief Function pointer type for a plugin metadata entry point.
 *
 * The function returns a pointer to static metadata owned by the plugin.
 */
using ComponentInfoFunction = const ComponentInfo* (*)() noexcept;

/**
 * @brief Converts an ABI-stable component version to `eversion::Version`.
 *
 * Null string pointers are treated as empty strings.
 *
 * @warning The returned `Version` contains string views into @p version. When
 * @p version came from a dynamic plugin, the returned value must not outlive
 * the plugin library or the storage backing the plugin's `const char*` fields.
 *
 * @param version Component version metadata to convert.
 * @return A non-owning EVersion view.
 */
[[nodiscard]] constexpr Version ToVersion(
    const ComponentVersion& version) noexcept {
  return Version{
      .major = version.major,
      .minor = version.minor,
      .patch = version.patch,
      .prerelease = detail::StringViewOrEmpty(version.prerelease),
      .build_metadata = detail::StringViewOrEmpty(version.build_metadata),
  };
}

/**
 * @brief Returns whether a byte range is present in component metadata.
 *
 * Use this check before reading optional fields appended to `ComponentInfo` in
 * a future ABI-compatible revision.
 *
 * @param info Metadata pointer returned by a plugin entry point.
 * @param offset Byte offset of the field to read.
 * @param size Size of the field to read.
 * @return `true` when @p info is non-null and the byte range is available.
 */
[[nodiscard]] constexpr bool IsComponentInfoFieldAvailable(
    const ComponentInfo* info, std::size_t offset, std::size_t size) noexcept {
  if (info == nullptr || offset > info->struct_size) {
    return false;
  }
  return size <= info->struct_size - offset;
}

/**
 * @brief Validates basic ABI invariants for component metadata.
 *
 * @param info Metadata pointer returned by a plugin entry point.
 * @return `true` when @p info is non-null and matches the supported ABI.
 */
[[nodiscard]] constexpr bool IsValidComponentInfo(
    const ComponentInfo* info) noexcept {
  return info != nullptr && info->abi_version == kComponentInfoAbiVersion &&
         IsComponentInfoFieldAvailable(info, 0U, kComponentInfoRequiredSize) &&
         info->id != nullptr && info->name != nullptr &&
         info->version.prerelease != nullptr &&
         info->version.build_metadata != nullptr;
}

}  // namespace eversion

#endif  // EVERSION_COMPONENT_INFO_H_
