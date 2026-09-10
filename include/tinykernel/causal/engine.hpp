#pragma once

#include "tinykernel/ontology/types.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace tinykernel::causal {

class InterventionEngine {
public:
  using Operator = std::function<ontology::Realization(
      const ontology::Realization &, const ontology::Intervention &)>;

  InterventionEngine();
  void register_operator(std::string kind, Operator operation);
  [[nodiscard]] ontology::Realization apply(
      const ontology::Realization &source,
      const ontology::Intervention &intervention) const;
  [[nodiscard]] std::vector<std::string> supported_kinds() const;

private:
  std::map<std::string, Operator, std::less<>> operators_;
};

class CausalGraph {
public:
  explicit CausalGraph(const ontology::Study &study);
  [[nodiscard]] bool valid() const;
  [[nodiscard]] bool contains_realization(const std::string &id) const;
  [[nodiscard]] bool reachable(const std::string &from, const std::string &to) const;

private:
  std::vector<std::string> nodes_;
  std::vector<std::pair<std::string, std::string>> edges_;
  bool valid_{true};
};

} // namespace tinykernel::causal
