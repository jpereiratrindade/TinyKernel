#pragma once

#include "tinykernel/ontology/types.hpp"

#include <string>
#include <vector>

namespace tinykernel::knowledge {

struct ClaimDecision {
  bool allowed{};
  std::string reason;
};

class ClaimAdjudicator {
public:
  [[nodiscard]] ClaimDecision may_support(
      const ontology::Claim &claim,
      const ontology::Study &study) const;
};

struct Frontier {
  std::vector<std::string> known_realizations;
  std::vector<std::string> preserving_realizations;
  std::vector<std::string> ruptured_realizations;
  std::vector<std::string> undetermined_realizations;
  std::vector<std::string> currently_minimal_candidates;
  std::vector<std::string> known_incomparable_candidates;
  std::vector<std::string> unexplored_interventions;
  std::vector<std::string> supported_claims;
  std::vector<std::string> open_claims;
  std::string limitation;
};

[[nodiscard]] Frontier analyze_frontier(const ontology::Study &study);
[[nodiscard]] std::string frontier_json(const Frontier &frontier);
void infer_claims(ontology::Study &study);

} // namespace tinykernel::knowledge
