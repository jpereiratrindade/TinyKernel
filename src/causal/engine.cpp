#include "tinykernel/causal/engine.hpp"

#include <algorithm>
#include <queue>
#include <set>
#include <stdexcept>

namespace tinykernel::causal {
namespace {

auto find_component(ontology::Realization &result, const std::string &target) {
  return std::find(result.components.begin(), result.components.end(), target);
}

ontology::Realization result_base(const ontology::Realization &source,
                                  const ontology::Intervention &intervention) {
  auto result = source;
  result.identity.id = intervention.target_realization_id.value_or(
      source.identity.id + ":" + intervention.identity.id);
  result.label = source.label + " / " + intervention.kind + " " + intervention.target;
  return result;
}

} // namespace

InterventionEngine::InterventionEngine() {
  register_operator("remove", [](const auto &source, const auto &intervention) {
    auto result = result_base(source, intervention);
    const auto it = find_component(result, intervention.target);
    if (it == result.components.end()) throw std::invalid_argument("remove target not found");
    result.components.erase(it);
    if (result.reduction_rank > 0) --result.reduction_rank;
    return result;
  });
  register_operator("replace", [](const auto &source, const auto &intervention) {
    auto result = result_base(source, intervention);
    const auto it = find_component(result, intervention.target);
    if (it == result.components.end()) throw std::invalid_argument("replace target not found");
    *it = intervention.replacement;
    return result;
  });
  register_operator("disable", [](const auto &source, const auto &intervention) {
    auto result = result_base(source, intervention);
    const auto it = find_component(result, intervention.target);
    if (it == result.components.end()) throw std::invalid_argument("disable target not found");
    *it = "disabled:" + *it;
    if (result.reduction_rank > 0) --result.reduction_rank;
    return result;
  });
  register_operator("merge", [](const auto &source, const auto &intervention) {
    auto result = result_base(source, intervention);
    const auto first = find_component(result, intervention.target);
    const auto second = find_component(result, intervention.replacement);
    if (first == result.components.end() || second == result.components.end() || first == second) {
      throw std::invalid_argument("merge targets not found or identical");
    }
    const std::string merged = "merged:" + intervention.target + "+" + intervention.replacement;
    result.components.erase(std::remove(result.components.begin(), result.components.end(),
                                        intervention.target), result.components.end());
    result.components.erase(std::remove(result.components.begin(), result.components.end(),
                                        intervention.replacement), result.components.end());
    result.components.push_back(merged);
    if (result.reduction_rank > 0) --result.reduction_rank;
    return result;
  });
  register_operator("perturb", [](const auto &source, const auto &intervention) {
    auto result = result_base(source, intervention);
    const auto it = find_component(result, intervention.target);
    if (it == result.components.end()) throw std::invalid_argument("perturb target not found");
    *it = "perturbed:" + intervention.target + ":" + intervention.replacement;
    return result;
  });
}

void InterventionEngine::register_operator(std::string kind, Operator operation) {
  if (kind.empty() || !operation) throw std::invalid_argument("invalid intervention operator");
  operators_.insert_or_assign(std::move(kind), std::move(operation));
}

ontology::Realization InterventionEngine::apply(
    const ontology::Realization &source,
    const ontology::Intervention &intervention) const {
  const auto it = operators_.find(intervention.kind);
  if (it == operators_.end()) throw std::invalid_argument("unsupported intervention: " + intervention.kind);
  return it->second(source, intervention);
}

std::vector<std::string> InterventionEngine::supported_kinds() const {
  std::vector<std::string> result;
  for (const auto &[kind, ignored] : operators_) {
    static_cast<void>(ignored);
    result.push_back(kind);
  }
  return result;
}

CausalGraph::CausalGraph(const ontology::Study &study) {
  for (const auto &realization : study.realizations) nodes_.push_back(realization.identity.id);
  for (const auto &intervention : study.interventions) {
    if (!intervention.target_realization_id || intervention.status != "performed") continue;
    edges_.emplace_back(intervention.source_realization_id, *intervention.target_realization_id);
    if (!contains_realization(intervention.source_realization_id) ||
        !contains_realization(*intervention.target_realization_id)) valid_ = false;
  }
  std::sort(nodes_.begin(), nodes_.end());
  valid_ = valid_ && std::adjacent_find(nodes_.begin(), nodes_.end()) == nodes_.end();
}

bool CausalGraph::valid() const { return valid_; }

bool CausalGraph::contains_realization(const std::string &id) const {
  return std::find(nodes_.begin(), nodes_.end(), id) != nodes_.end();
}

bool CausalGraph::reachable(const std::string &from, const std::string &to) const {
  std::queue<std::string> pending;
  std::set<std::string> visited;
  pending.push(from);
  while (!pending.empty()) {
    auto current = pending.front();
    pending.pop();
    if (!visited.insert(current).second) continue;
    if (current == to) return true;
    for (const auto &[source, target] : edges_) if (source == current) pending.push(target);
  }
  return false;
}

} // namespace tinykernel::causal
