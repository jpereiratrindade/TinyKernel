#include "tinykernel/knowledge/analysis.hpp"

#include "tinykernel/causal/engine.hpp"
#include "tinykernel/evidence/digest.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace tinykernel::knowledge {
namespace {

using namespace ontology;

bool contains(const std::vector<std::string> &values, const std::string &value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

std::string json_string(const std::string &value) {
  std::ostringstream Radar;
  Radar << '"';
  for (const unsigned char character : value) {
    switch (character) {
    case '"': Radar << "\\\""; break;
    case '\\': Radar << "\\\\"; break;
    case '\b': Radar << "\\b"; break;
    case '\f': Radar << "\\f"; break;
    case '\n': Radar << "\\n"; break;
    case '\r': Radar << "\\r"; break;
    case '\t': Radar << "\\t"; break;
    default:
      if (character < 0x20U) {
        constexpr char digits[] = "0123456789abcdef";
        Radar << "\\u00" << digits[character >> 4U] << digits[character & 0x0fU];
      } else {
        Radar << static_cast<char>(character);
      }
    }
  }
  Radar << '"';
  return Radar.str();
}

void json_array(std::ostringstream &out, const std::vector<std::string> &values) {
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) out << ',';
    out << json_string(values[i]);
  }
  out << ']';
}

} // namespace

ClaimDecision ClaimAdjudicator::may_support(const Claim &claim, const Study &study) const {
  if (claim.phenomenon_id != study.phenomenon.identity.id ||
      claim.context_id != study.context.identity.id) {
    return {false, "claim scope does not match the investigation"};
  }

  std::set<std::string> referenced_runs;
  bool has_empirical_evidence = false;
  for (const auto &reference : claim.evidence_references) {
    const auto found = std::find_if(study.evidence.begin(), study.evidence.end(),
        [&](const auto &item) { return item.identity.id == reference; });
    if (found == study.evidence.end()) return {false, "referenced evidence does not exist"};
    if (found->evidence_type == ontology::EvidenceType::empirical_observation) {
      has_empirical_evidence = true;
    }
    if (evidence::sha256(found->artifact) != found->sha256) return {false, "evidence digest mismatch"};
    referenced_runs.insert(found->run_id);
  }

  if (claim.level == ClaimLevel::l0_ontology) return {true, "versioned ontology scope exists"};
  if (claim.level == ClaimLevel::l1_observation) {
    return {has_empirical_evidence, "L1 requires recorded empirical evidence"};
  }

  if (!has_empirical_evidence) {
    return {false, "L2-L8 require recorded empirical evidence in the referenced scope"};
  }

  bool baseline_preserving = false;
  for (const auto &adjudication : study.adjudications) {
    if (!referenced_runs.contains(adjudication.run_id)) continue;
    if (adjudication.outcome == Outcome::preserving && adjudication.classification == "PRESERVED") {
      baseline_preserving = true;
    }
  }

  if (claim.level == ClaimLevel::l2_relative_sufficiency) {
    return {baseline_preserving, "L2 requires preserving adjudication in the declared scope"};
  }

  if (claim.level == ClaimLevel::l3_relative_necessity) {
    if (claim.intervention_scope.empty() || claim.witness_scope.empty()) {
      return {false, "L3 requires explicitly preregistered intervention_scope and witness_scope"};
    }
    if (!baseline_preserving) {
      return {false, "L3 requires a preserving baseline empirical adjudication"};
    }

    // Verify chain: claim.intervention_scope -> Run.intervention_id -> Adjudication.run_id -> Evidence.witness_id
    for (const auto &itv_id : claim.intervention_scope) {
      const auto itv = std::find_if(study.interventions.begin(), study.interventions.end(),
                                    [&](const auto &item) { return item.identity.id == itv_id && item.status == "performed"; });
      if (itv == study.interventions.end()) {
        return {false, "L3 intervention not performed: " + itv_id};
      }

      // Find run associated with this intervention
      const auto run = std::find_if(study.runs.begin(), study.runs.end(), [&](const auto &r) {
        return (r.intervention_id && *r.intervention_id == itv_id) ||
               (itv->target_realization_id && r.result_realization_id == *itv->target_realization_id);
      });
      if (run == study.runs.end() || !referenced_runs.contains(run->identity.id)) {
        return {false, "L3 requires executed run with referenced empirical evidence for intervention " + itv_id};
      }

      // Adjudication must be BROKEN_CAUSAL
      const auto adj = std::find_if(study.adjudications.begin(), study.adjudications.end(), [&](const auto &a) {
        return a.run_id == run->identity.id && a.outcome == Outcome::ruptured && a.classification == "BROKEN_CAUSAL";
      });
      if (adj == study.adjudications.end()) {
        return {false, "L3 requires BROKEN_CAUSAL adjudication for run " + run->identity.id};
      }
    }

    return {true, "L3 satisfied: preserving baseline and controlled causal rupture for all scoped interventions"};
  }

  const bool search_open = std::any_of(study.interventions.begin(), study.interventions.end(),
      [](const auto &item) { return item.status != "performed"; });
  if (claim.level == ClaimLevel::l4_irreducibility || claim.level == ClaimLevel::l5_relative_minimality) {
    return {!search_open && baseline_preserving,
            "L4/L5 require all declared immediate reductions to be adjudicated"};
  }
  if (claim.level == ClaimLevel::l6_experimental_causal_equivalence) {
    return {false, "L6 requires matched intervention signatures for distinct minimal realizations"};
  }
  if (claim.level == ClaimLevel::l7_contextual_robustness) {
    return {false, "L7 requires preregistered evidence from additional contexts"};
  }
  return {false, "L8 requires accumulated L5-L7 evidence and is not reachable in this investigation"};
}

void infer_claims(Study &study) {
  ClaimAdjudicator adjudicator;
  bool any_supported = false;
  for (auto &claim : study.claims) {
    const auto decision = adjudicator.may_support(claim, study);
    if (decision.allowed) {
      claim.status = ClaimStatus::supported;
      any_supported = true;
    } else {
      claim.status = ClaimStatus::open;
    }
  }
  if (any_supported || study.investigation.status == "adjudicated") {
    advance_phase(study.investigation, InvestigationPhase::inferred);
  }
}

Frontier analyze_frontier(const Study &study) {
  Frontier result;
  std::map<std::string, Outcome> outcomes;
  std::map<std::string, std::uint32_t> ranks;
  for (const auto &realization : study.realizations) {
    result.known_realizations.push_back(realization.identity.id);
    ranks[realization.identity.id] = realization.reduction_rank;
  }
  for (const auto &adjudication : study.adjudications) {
    const auto run = std::find_if(study.runs.begin(), study.runs.end(),
        [&](const auto &item) { return item.identity.id == adjudication.run_id; });
    if (run != study.runs.end()) outcomes[run->result_realization_id] = adjudication.outcome;
  }
  for (const auto &id : result.known_realizations) {
    const auto found = outcomes.find(id);
    const auto outcome = found == outcomes.end() ? Outcome::undetermined : found->second;
    if (outcome == Outcome::preserving) result.preserving_realizations.push_back(id);
    if (outcome == Outcome::ruptured) result.ruptured_realizations.push_back(id);
    if (outcome == Outcome::undetermined) result.undetermined_realizations.push_back(id);
  }

  if (!result.preserving_realizations.empty()) {
    const auto minimum = std::min_element(result.preserving_realizations.begin(), result.preserving_realizations.end(),
        [&](const auto &left, const auto &right) { return ranks[left] < ranks[right]; });
    const auto minimum_rank = ranks[*minimum];
    for (const auto &candidate : result.preserving_realizations) {
      if (ranks[candidate] == minimum_rank) result.currently_minimal_candidates.push_back(candidate);
    }
  }

  causal::CausalGraph graph(study);
  for (std::size_t i = 0; i < result.currently_minimal_candidates.size(); ++i) {
    for (std::size_t j = i + 1; j < result.currently_minimal_candidates.size(); ++j) {
      const auto &left = result.currently_minimal_candidates[i];
      const auto &right = result.currently_minimal_candidates[j];
      const bool ordered_by_reduction = ranks[left] != ranks[right] &&
          (graph.reachable(left, right) || graph.reachable(right, left));
      if (!ordered_by_reduction) {
        if (!contains(result.known_incomparable_candidates, left)) result.known_incomparable_candidates.push_back(left);
        if (!contains(result.known_incomparable_candidates, right)) result.known_incomparable_candidates.push_back(right);
      }
    }
  }
  for (const auto &intervention : study.interventions) {
    if (intervention.status != "performed") result.unexplored_interventions.push_back(intervention.identity.id);
  }
  for (const auto &claim : study.claims) {
    if (claim.status == ClaimStatus::supported) result.supported_claims.push_back(claim.identity.id);
    if (claim.status == ClaimStatus::open) result.open_claims.push_back(claim.identity.id);
  }

  auto sort_all = [](auto &values) { std::sort(values.begin(), values.end()); };
  sort_all(result.known_realizations);
  sort_all(result.preserving_realizations);
  sort_all(result.ruptured_realizations);
  sort_all(result.undetermined_realizations);
  sort_all(result.currently_minimal_candidates);
  sort_all(result.known_incomparable_candidates);
  sort_all(result.unexplored_interventions);
  sort_all(result.supported_claims);
  sort_all(result.open_claims);
  result.limitation = result.unexplored_interventions.empty()
      ? "Minimalidade permanece relativa ao espaço e à ordem Gamma declarados."
      : "Fronteira de intervenções aberta; nenhuma minimalidade global é sustentada.";
  return result;
}

std::string frontier_json(const Frontier &frontier) {
  std::ostringstream out;
  out << '{';
  out << "\"known_realizations\":"; json_array(out, frontier.known_realizations);
  out << ",\"preserving_realizations\":"; json_array(out, frontier.preserving_realizations);
  out << ",\"ruptured_realizations\":"; json_array(out, frontier.ruptured_realizations);
  out << ",\"undetermined_realizations\":"; json_array(out, frontier.undetermined_realizations);
  out << ",\"currently_minimal_candidates\":"; json_array(out, frontier.currently_minimal_candidates);
  out << ",\"known_incomparable_candidates\":"; json_array(out, frontier.known_incomparable_candidates);
  out << ",\"unexplored_intervention_frontier\":"; json_array(out, frontier.unexplored_interventions);
  out << ",\"supported_claims\":"; json_array(out, frontier.supported_claims);
  out << ",\"open_claims\":"; json_array(out, frontier.open_claims);
  out << ",\"limitations\":" << json_string(frontier.limitation) << '}';
  return out.str();
}

} // namespace tinykernel::knowledge
