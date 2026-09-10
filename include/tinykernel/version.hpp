#pragma once

#include <string_view>

namespace tinykernel {

inline constexpr std::string_view version = TINYKERNEL_VERSION;
inline constexpr std::string_view ontology_id = "TK-O";
inline constexpr std::string_view ontology_version = "0.2.0";
inline constexpr unsigned schema_version = 1;

} // namespace tinykernel
