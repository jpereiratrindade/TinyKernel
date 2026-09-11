#pragma once

#include "tinykernel/ontology/types.hpp"

#include <string>
#include <vector>

namespace tinykernel::experiment {

struct PlannedIntervention {
  std::string kind;
  std::string target;
  std::string replacement;
};

struct StudyDraft {
  std::string id;
  std::string title;
  std::string phenomenon_description;
  std::string context_description;
  std::string baseline_label;
  std::vector<std::string> components;
  std::vector<std::string> distinctions;
  std::vector<std::string> relations;
  std::vector<std::string> temporal_constraints;
  std::vector<PlannedIntervention> interventions;
};

[[nodiscard]] ontology::Study create_study(const StudyDraft &draft);
void materialize_intervention(ontology::Study &study, const std::string &planned_id,
                              const std::string &source_id, const std::string &kind,
                              const std::string &target, const std::string &replacement,
                              const std::string &protocol);
void record_observation(ontology::Study &study, const std::string &realization_id,
                        const std::string &dimension, bool satisfied,
                        const std::string &trace);
void adjudicate_observations(ontology::Study &study);
void infer_observed_claims(ontology::Study &study);

} // namespace tinykernel::experiment
