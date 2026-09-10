#pragma once

#include "tinykernel/causal/engine.hpp"
#include "tinykernel/ontology/types.hpp"

#include <optional>

namespace tinykernel::experiment {

struct AdapterObservation {
  bool operational{};
  bool causal{};
  bool discriminative{};
  bool observable{};
  bool temporal{};
  std::string artifact;
};

class RealizationAdapter {
public:
  virtual ~RealizationAdapter() = default;
  [[nodiscard]] virtual AdapterObservation observe(
      const ontology::Realization &realization) const = 0;
};

class Laboratory {
public:
  explicit Laboratory(causal::InterventionEngine engine = {});
  [[nodiscard]] ontology::Study execute(
      ontology::Study study, const RealizationAdapter &adapter) const;

private:
  causal::InterventionEngine engine_;
};

[[nodiscard]] ontology::Study make_tk0000();
[[nodiscard]] ontology::Study make_tk0001();
[[nodiscard]] ontology::Study make_tk_sait_001();
[[nodiscard]] ontology::Study execute_tk0000();
[[nodiscard]] ontology::Study execute_tk0001();

} // namespace tinykernel::experiment
