#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tinykernel::ontology {

struct Identity {
  std::string id;
  std::uint32_t schema_version{1};
  std::string ontology_version{"0.2.0"};

  auto operator<=>(const Identity &) const = default;
};

struct Investigation {
  Identity identity;
  std::string title;
  std::string phenomenon_id;
  std::string context_id;
  std::string constitutive_profile_id;
  std::string reduction_order;
  std::string status;
};

struct Phenomenon { Identity identity; std::string name; std::string definition; };
struct Context { Identity identity; std::string description; };

struct ConstitutiveProfile {
  Identity identity;
  std::vector<std::string> distinctions;
  std::vector<std::string> relations;
  std::vector<std::string> temporal_constraints;
};

struct Realization {
  Identity identity;
  std::string investigation_id;
  std::string label;
  std::vector<std::string> components;
  std::uint32_t reduction_rank{};
};

struct Structure {
  Identity identity;
  std::string realization_id;
  std::vector<std::string> parts;
  std::vector<std::string> relations;
};

struct Intervention {
  Identity identity;
  std::string investigation_id;
  std::string kind;
  std::string source_realization_id;
  std::optional<std::string> target_realization_id;
  std::string target;
  std::string replacement;
  std::string prediction;
  std::string status;
};

struct Observation {
  Identity identity;
  std::string run_id;
  std::string realization_id;
  std::string witness_id;
  std::string dimension;
  std::string value;
  bool satisfied{};
};

struct Witness {
  Identity identity;
  std::string kind;
  std::string description;
};

struct Evidence {
  Identity identity;
  std::string run_id;
  std::string witness_id;
  std::vector<std::string> observation_ids;
  std::string artifact;
  std::string sha256;
};

enum class Outcome { preserving, ruptured, undetermined };

struct Adjudication {
  Identity identity;
  std::string run_id;
  Outcome outcome{Outcome::undetermined};
  std::string classification;
  std::string rule;
  std::string rationale;
  std::vector<std::string> evidence_references;
};

enum class ClaimLevel : std::uint8_t {
  l0_ontology = 0,
  l1_observation = 1,
  l2_relative_sufficiency = 2,
  l3_relative_necessity = 3,
  l4_irreducibility = 4,
  l5_relative_minimality = 5,
  l6_experimental_causal_equivalence = 6,
  l7_contextual_robustness = 7,
  l8_candidate_kernel = 8
};

enum class ClaimStatus { open, supported, rejected };

struct Claim {
  Identity identity;
  std::string subject;
  std::string assertion;
  std::string phenomenon_id;
  std::string context_id;
  std::vector<std::string> intervention_scope;
  std::vector<std::string> witness_scope;
  std::vector<std::string> evidence_references;
  ClaimLevel level{ClaimLevel::l0_ontology};
  ClaimStatus status{ClaimStatus::open};
  std::string limitations;
  std::string provenance_id;
};

struct Run {
  Identity identity;
  std::string investigation_id;
  std::optional<std::string> intervention_id;
  std::string source_realization_id;
  std::string result_realization_id;
  std::string status;
};

struct Provenance {
  Identity identity;
  std::string source;
  std::string method;
  std::string timestamp;
  std::string detail;
};

struct Study {
  Investigation investigation;
  Phenomenon phenomenon;
  Context context;
  ConstitutiveProfile constitutive_profile;
  std::vector<Realization> realizations;
  std::vector<Structure> structures;
  std::vector<Intervention> interventions;
  std::vector<Observation> observations;
  std::vector<Witness> witnesses;
  std::vector<Evidence> evidence;
  std::vector<Adjudication> adjudications;
  std::vector<Claim> claims;
  std::vector<Run> runs;
  std::vector<Provenance> provenance;
};

std::string to_string(Outcome value);
std::string to_string(ClaimLevel value);
std::string to_string(ClaimStatus value);
ClaimLevel claim_level_from_string(const std::string &value);
ClaimStatus claim_status_from_string(const std::string &value);
Outcome outcome_from_string(const std::string &value);

} // namespace tinykernel::ontology
