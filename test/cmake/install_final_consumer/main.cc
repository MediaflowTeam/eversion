/**
 * @file main.cc
 * @brief Tests consumption of a generated EVersion header from an installed
 * producer.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#include <producer/version_info.h>

int main() {
  static_assert(producer::version_info::kVersionMajor == 4U);
  static_assert(producer::version_info::kVersionMinor == 5U);
  static_assert(producer::version_info::kVersionPatch == 6U);
  static_assert(producer::version_info::kVersionString == "4.5.6");
  return producer::version_info::kVersion.IsStable() ? 0 : 1;
}
