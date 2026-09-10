#pragma once

#include <string>
#include <string_view>

namespace tinykernel::evidence {

[[nodiscard]] std::string sha256(std::string_view input);

} // namespace tinykernel::evidence
