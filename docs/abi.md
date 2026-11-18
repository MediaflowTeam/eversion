# Plugin Metadata ABI

This document is the reference for the binary interface between a host
application and the dynamic plugins it loads: the `ComponentInfo` and
`ComponentVersion` structures in `<eversion/component_info.h>` and the exported
entry point that returns them. It records the current layout, the rules for
changing it, and every past layout.

The C++ types `eversion::Version` and `eversion::RuntimeVersion` are not part of
this ABI. They never cross a plugin boundary.

## Entry Point

`eversion_declare_plugin()` generates one exported function in the plugin:

```cpp
extern "C" const eversion::ComponentInfo* eversion_plugin_info() noexcept;
```

- The name is `eversion::kPluginInfoSymbol` unless `SYMBOL` overrides it.
- The function returns a pointer to static data owned by the plugin. The pointer
  and every `const char*` field stay valid only while the plugin is loaded.
- The C++ function type is `eversion::ComponentInfoFunction`.

## Evolution Rules

1. **Existing fields never change.** Within one ABI version, no field is
   removed, reordered, resized, retyped, or given a new meaning.
2. **`ComponentVersion` is fixed.** It holds exactly the five parts of a
   SemVer 2.0.0 version, `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`, and never
   gains fields. A release channel, build number, commit, or date belongs in
   the prerelease or build metadata.
3. **New fields are appended to the end of `ComponentInfo`.** Each append is a
   new *revision* of the same ABI version. Plugins report the size they were
   built with in `struct_size`. Before reading a field that a later revision
   appended, a host calls `IsComponentInfoFieldAvailable()`; if the plugin is
   older and the field is missing, the host uses a default value or treats the
   field as unavailable. New hosts therefore accept old plugins, and old hosts
   ignore fields they do not know.
4. **Any other change is incompatible and bumps `kComponentInfoAbiVersion`.**
   This includes changing `ComponentVersion`, changing a field, or changing the
   entry point signature. `IsValidComponentInfo()` rejects metadata whose
   `abi_version` is not the version the host was built with.
5. **The contract holds only within one architecture.** The layout contains
   `std::size_t` and pointers, so offsets and sizes depend on pointer width and
   alignment, as for any C ABI. A host and its plugins must be built for the
   same architecture. The CMake package is `ARCH_INDEPENDENT` only because
   consumers compile the headers themselves; that does not make plugin
   binaries portable across architectures.

`test/component_info_test.cc` locks the current layout with `static_assert` on
64-bit Linux, so an accidental change fails to compile.

## Validating Plugin Metadata

A plugin is code that the host does not control, so the host checks what it
receives before using it. There are two separate checks:

| Check | What it verifies | Required |
| --- | --- | --- |
| `IsValidComponentInfo(info)` | Structure: the pointer is not null, `abi_version` matches, `struct_size` covers the required prefix, and no required string pointer is null. | Yes, before reading any field. |
| `IsWellFormedComponentVersion(info->version)` | Content: the prerelease and build metadata strings follow the SemVer identifier grammar, the same grammar that `TryParseVersion()` and `eversion_declare()` accept. Empty strings mean "absent" and are valid. | Optional. |

Valid structure does not mean trustworthy content. Without the second check, a
plugin can report a prerelease such as `"bad id!! "`; the host would copy it as
it is, and it would take part in comparisons by plain byte order. Hosts that
display, store, or compare plugin versions should call
`IsWellFormedComponentVersion()` and reject plugins that fail it.

All strings in `ComponentInfo` point into the plugin's memory. Copy the version
with `ToRuntimeVersion(info->version)`, and copy `id` and `name` into
`std::string`, before the plugin is unloaded.

## Current ABI: Version 1, Revision 1

`kComponentInfoAbiVersion` is `1`. Layout on 64-bit Linux (LP64):

### `ComponentVersion`

| Offset | Size | Field | Type | Meaning |
| ---: | ---: | --- | --- | --- |
| 0 | 4 | `major` | `std::uint32_t` | Major version. |
| 4 | 4 | `minor` | `std::uint32_t` | Minor version. |
| 8 | 4 | `patch` | `std::uint32_t` | Patch version. |
| 12 | 4 | | | Padding. |
| 16 | 8 | `prerelease` | `const char*` | Prerelease identifiers without the leading `-`, or `""`. Never null. |
| 24 | 8 | `build_metadata` | `const char*` | Build metadata without the leading `+`, or `""`. Never null. |

Size: 32 bytes.

### `ComponentInfo`

| Offset | Size | Field | Type | Meaning |
| ---: | ---: | --- | --- | --- |
| 0 | 4 | `abi_version` | `std::uint32_t` | `kComponentInfoAbiVersion` of the producer. |
| 4 | 4 | | | Padding. |
| 8 | 8 | `struct_size` | `std::size_t` | `sizeof(ComponentInfo)` as compiled by the producer. |
| 16 | 8 | `id` | `const char*` | Stable machine-readable identifier, such as `example.report.plugin`. Never null. |
| 24 | 8 | `name` | `const char*` | Human-readable name. Never null. |
| 32 | 32 | `version` | `ComponentVersion` | Version of the plugin. |

Size: 64 bytes. `kComponentInfoRequiredSize`, the smallest `struct_size` that
an ABI 1 host accepts, is 64.

## History

Entries are never edited after the release that introduced them. A new
revision or ABI version adds a row here and a section above, and the previous
"Current ABI" section moves below as a past layout.

| ABI version | Revision | Introduced in | Change | `sizeof(ComponentInfo)` on LP64 |
| ---: | ---: | --- | --- | ---: |
| 1 | 1 | EVersion 0.1.0 (unreleased) | First layout: `abi_version`, `struct_size`, `id`, `name`, `version`. | 64 |

There are no past layouts yet: version 1, revision 1 is the first ABI.

## Changing the ABI

To append a field (a new revision):

1. Append the field to the end of `ComponentInfo` with a default member
   initializer, and document it.
2. Do not change `kComponentInfoRequiredSize`: old plugins without the field
   must stay valid.
3. Read the field only after `IsComponentInfoFieldAvailable()` succeeds, and
   define what a host does when it is missing.
4. Extend the layout assertions in `test/component_info_test.cc`, and add a
   test with a plugin built against the previous revision.
5. Add a row to the history table, update the "Current ABI" section, and note
   the change in `CHANGELOG.md`.

For an incompatible change, bump `kComponentInfoAbiVersion`, keep the previous
layout in this document as a past ABI, and state in `CHANGELOG.md` that hosts
built with the new version reject plugins built with the old one.
