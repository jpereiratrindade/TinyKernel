#include "tinykernel/ontology/types.hpp"

#include <stdexcept>

namespace tinykernel::ontology {

std::string to_string(const Outcome value) {
  switch (value) {
  case Outcome::preserving: return "preserving";
  case Outcome::ruptured: return "ruptured";
  case Outcome::undetermined: return "undetermined";
  }
  throw std::logic_error("unknown outcome");
}

std::string to_string(const ClaimLevel value) {
  switch (value) {
  case ClaimLevel::l0_ontology: return "L0";
  case ClaimLevel::l1_observation: return "L1";
  case ClaimLevel::l2_relative_sufficiency: return "L2";
  case ClaimLevel::l3_relative_necessity: return "L3";
  case ClaimLevel::l4_irreducibility: return "L4";
  case ClaimLevel::l5_relative_minimality: return "L5";
  case ClaimLevel::l6_experimental_causal_equivalence: return "L6";
  case ClaimLevel::l7_contextual_robustness: return "L7";
  case ClaimLevel::l8_candidate_kernel: return "L8";
  }
  throw std::logic_error("unknown claim level");
}

std::string to_string(const ClaimStatus value) {
  switch (value) {
  case ClaimStatus::open: return "open";
  case ClaimStatus::supported: return "supported";
  case ClaimStatus::rejected: return "rejected";
  }
  throw std::logic_error("unknown claim status");
}

std::string to_string(const EvidenceType value) {
  switch (value) {
  case EvidenceType::structural_record: return "STRUCTURAL_RECORD";
  case EvidenceType::empirical_observation: return "EMPIRICAL_OBSERVATION";
  case EvidenceType::unspecified: return "UNSPECIFIED";
  }
  throw std::logic_error("unknown evidence type");
}

std::string to_string(const InvestigationPhase value) {
  switch (value) {
  case InvestigationPhase::formulated: return "formulated";
  case InvestigationPhase::materialized: return "materialized";
  case InvestigationPhase::observed: return "observed";
  case InvestigationPhase::adjudicated: return "adjudicated";
  case InvestigationPhase::inferred: return "inferred";
  }
  throw std::logic_error("unknown investigation phase");
}

ClaimLevel claim_level_from_string(const std::string &value) {
  if (value.size() == 2 && value[0] == 'L' && value[1] >= '0' && value[1] <= '8') {
    return static_cast<ClaimLevel>(value[1] - '0');
  }
  throw std::invalid_argument("invalid claim level: " + value);
}

ClaimStatus claim_status_from_string(const std::string &value) {
  if (value == "open") return ClaimStatus::open;
  if (value == "supported") return ClaimStatus::supported;
  if (value == "rejected") return ClaimStatus::rejected;
  throw std::invalid_argument("invalid claim status: " + value);
}

Outcome outcome_from_string(const std::string &value) {
  if (value == "preserving") return Outcome::preserving;
  if (value == "ruptured") return Outcome::ruptured;
  if (value == "undetermined") return Outcome::undetermined;
  throw std::invalid_argument("invalid outcome: " + value);
}

EvidenceType evidence_type_from_string(const std::string &value) {
  if (value == "STRUCTURAL_RECORD") return EvidenceType::structural_record;
  if (value == "EMPIRICAL_OBSERVATION") return EvidenceType::empirical_observation;
  if (value == "UNSPECIFIED" || value.empty()) return EvidenceType::unspecified;
  throw std::invalid_argument("invalid evidence type: " + value);
}

InvestigationPhase investigation_phase_from_string(const std::string &value) {
  if (value == "formulated" || value == "declared" || value == "preregistered") return InvestigationPhase::formulated;
  if (value == "materialized") return InvestigationPhase::materialized;
  if (value == "observed") return InvestigationPhase::observed;
  if (value == "adjudicated") return InvestigationPhase::adjudicated;
  if (value == "inferred" || value == "executed") return InvestigationPhase::inferred;
  return InvestigationPhase::formulated;
}

InvestigationPhase advance_phase(const InvestigationPhase current, const InvestigationPhase candidate) {
  return static_cast<InvestigationPhase>(std::max(static_cast<std::uint8_t>(current),
                                                  static_cast<std::uint8_t>(candidate)));
}

void advance_phase(Investigation &inv, const InvestigationPhase candidate) {
  const auto curr_phase = investigation_phase_from_string(inv.status);
  inv.status = to_string(advance_phase(curr_phase, candidate));
}

std::string advance_phase_string(const std::string &current, const std::string &candidate) {
  const auto curr_phase = investigation_phase_from_string(current);
  const auto cand_phase = investigation_phase_from_string(candidate);
  return to_string(advance_phase(curr_phase, cand_phase));
}

} // namespace tinykernel::ontology
