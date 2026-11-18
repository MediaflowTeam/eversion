/**
 * @file component_info.h
 * @brief Defines ABI-stable component metadata for dynamically loaded plugins.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#ifndef EVERSION_COMPONENT_INFO_H_
#define EVERSION_COMPONENT_INFO_H_

#include <eversion/detail/component_info_strings.h>
#include <eversion/detail/runtime_version_parsing.h>
#include <eversion/runtime_version.h>

#include <cstddef>
#include <cstdint>
#include <string>
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
 * This type avoids C++ library types at the plugin boundary. Copy it into an
 * `eversion::RuntimeVersion` with `ToRuntimeVersion()` after validation.
 *
 * This type is fixed to the five SemVer 2.0.0 parts and will not gain fields.
 * Channels, build numbers, commit identifiers, and dates belong in
 * `prerelease` or `build_metadata`; component metadata that is not part of the
 * version is appended to the end of `ComponentInfo` instead. `ComponentVersion`
 * is embedded by value in `ComponentInfo`, so its size contributes to
 * `kComponentInfoRequiredSize`; changing it would shift every field appended
 * after it and make a new host reject all existing plugins.
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
 *
 * This layout is stable only between a host and a plugin built for the same
 * architecture, with the same pointer width and alignment.
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
 * @brief Copies an ABI-stable component version into an owning runtime
 * version.
 *
 * Null string pointers are treated as empty strings. The result owns copies of
 * the strings, so it stays valid after the plugin library is unloaded.
 *
 * @param version Component version metadata to copy.
 * @return A runtime version that owns copies of the prerelease and build
 * metadata strings.
 */
[[nodiscard]] inline RuntimeVersion ToRuntimeVersion(
    const ComponentVersion& version) {
  return RuntimeVersion{
      .major = version.major,
      .minor = version.minor,
      .patch = version.patch,
      .prerelease = std::string{detail::StringViewOrEmpty(version.prerelease)},
      .build_metadata =
          std::string{detail::StringViewOrEmpty(version.build_metadata)},
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

/**
 * @brief Validates the text content of component version metadata.
 *
 * `IsValidComponentInfo()` checks only the ABI structure: ABI version, minimum
 * structure size, and non-null pointers. This function checks the content of
 * the prerelease and build metadata strings that crosses the same plugin
 * boundary. Both are checks a host should perform before trusting plugin
 * metadata.
 *
 * This check is optional. When a host does not call it, malformed strings are
 * copied unchanged and take part in comparisons in lexicographic order.
 *
 * Accepted syntax is the same as `TryParseVersion()`: an empty string means the
 * value is absent; otherwise the text must be a dot-separated list of non-empty
 * identifiers containing only `[0-9A-Za-z-]`. Leading zeros in numeric
 * identifiers are accepted.
 *
 * @param version Component version metadata to validate.
 * @return `true` when both strings are well formed, or `false` when either
 * string is null or malformed.
 */
[[nodiscard]] constexpr bool IsWellFormedComponentVersion(
    const ComponentVersion& version) noexcept {
  // A null pointer means broken metadata. An empty string is valid: it means
  // the version has no prerelease or no build metadata.
  if (version.prerelease == nullptr || version.build_metadata == nullptr) {
    return false;
  }

  const std::string_view prerelease{version.prerelease};
  const std::string_view build_metadata{version.build_metadata};
  const bool prerelease_is_well_formed =
      prerelease.empty() || detail::IsVersionIdentifierList(prerelease);
  const bool build_metadata_is_well_formed =
      build_metadata.empty() || detail::IsVersionIdentifierList(build_metadata);
  return prerelease_is_well_formed && build_metadata_is_well_formed;
}

}  // namespace eversion

#endif  // EVERSION_COMPONENT_INFO_H_
