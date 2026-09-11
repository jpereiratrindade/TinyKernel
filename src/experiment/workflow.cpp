#include "tinykernel/experiment/workflow.hpp"

#include "tinykernel/causal/engine.hpp"
#include "tinykernel/evidence/digest.hpp"
#include "tinykernel/knowledge/analysis.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace tinykernel::experiment {
namespace {

using namespace ontology;

Identity identity(std::string value) { return {std::move(value), 2, "0.2.1"}; }

std::string timestamp_utc() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm value{};
#ifdef _WIN32
  gmtime_s(&value, &time);
#else
  gmtime_r(&time, &value);
#endif
  std::ostringstream out;
  out << std::put_time(&value, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

std::vector<Witness> default_witnesses(const std::string &id) {
  return {
      {identity(id + ":W:OPERATIONAL"), "operational", "O sistema produz comportamento observável."},
      {identity(id + ":W:CAUSAL"), "causal", "A saída depende da relação causal preregistrada."},
      {identity(id + ":W:DISCRIMINATIVE"), "discriminative", "As distinções do perfil permanecem separáveis."},
      {identity(id + ":W:OBSERVATIONAL"), "observational", "O aparato permanece capaz de observar."},
      {identity(id + ":W:TEMPORAL"), "temporal", "A restrição temporal preregistrada é satisfeita."},
  };
}

std::string upper_identifier(std::string value) {
  for (auto &character : value) {
    if ((character >= 'a' && character <= 'z')) character = static_cast<char>(character - 'a' + 'A');
    else if (!((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9'))) character = '_';
  }
  return value;
}

std::string replace_all(std::string value, const std::string &from, const std::string &to) {
  if (from.empty()) return value;
  std::size_t position = 0;
  while ((position = value.find(from, position)) != std::string::npos) {
    value.replace(position, from.size(), to);
    position += to.size();
  }
  return value;
}

std::vector<std::string> derived_relations(const std::vector<std::string> &relations,
                                           const std::string &kind, const std::string &target,
                                           const std::string &replacement) {
  std::vector<std::string> result;
  for (const auto &relation : relations) {
    if ((kind == "remove" || kind == "disable") && relation.find(target) != std::string::npos) continue;
    if (kind == "replace") result.push_back(replace_all(relation, target, replacement));
    else if (kind == "merge") {
      auto value = replace_all(relation, target, "__TK_MERGED__");
      value = replace_all(value, replacement, "__TK_MERGED__");
      result.push_back(replace_all(value, "__TK_MERGED__", target + "+" + replacement));
    }
    else if (kind == "perturb") result.push_back(replace_all(relation, target, target + "~perturbed"));
    else result.push_back(relation);
  }
  return result;
}

Outcome outcome_for(const std::string &classification) {
  if (classification == "PRESERVED") return Outcome::preserving;
  if (classification == "PARTIALLY_OBSERVED" || classification == "WITNESS_COMPROMISED" ||
      classification == "STALE") return Outcome::undetermined;
  return Outcome::ruptured;
}

void upsert_adjudication(Study &study, const Run &run, const std::string &classification,
                         std::vector<std::string> evidence_ids, std::string rationale) {
  const auto found = std::find_if(study.adjudications.begin(), study.adjudications.end(),
      [&](const auto &item) { return item.run_id == run.identity.id; });
  Adjudication value{identity(run.identity.id + ":A"), run.identity.id, outcome_for(classification),
      classification, "TK-O-0.2.1:dimensional-adjudication-v1", std::move(rationale), std::move(evidence_ids)};
  if (found == study.adjudications.end()) study.adjudications.push_back(std::move(value));
  else *found = std::move(value);
}

} // namespace

Study create_study(const StudyDraft &draft) {
  if (draft.id.empty() || draft.title.empty()) throw std::invalid_argument("id and title are required");
  if (draft.phenomenon_description.empty()) throw std::invalid_argument("phenomenon description is required");
  if (draft.context_description.empty()) throw std::invalid_argument("context description is required");
  if (draft.baseline_label.empty()) throw std::invalid_argument("baseline label is required");
  if (draft.components.empty()) throw std::invalid_argument("baseline requires at least one component");
  if (draft.distinctions.empty() || draft.relations.empty() || draft.temporal_constraints.empty()) {
    throw std::invalid_argument("constitutive profile requires distinctions, relations, and temporal constraints");
  }
  Study study;
  study.investigation = {identity(draft.id), draft.title, draft.id + ":P", draft.id + ":C",
      draft.id + ":PHI", "Gamma=active_causal_relations", "formulated"};
  study.phenomenon = {identity(draft.id + ":P"), draft.title, draft.phenomenon_description};
  study.context = {identity(draft.id + ":C"), draft.context_description};
  study.constitutive_profile = {identity(draft.id + ":PHI"), draft.distinctions,
      draft.relations, draft.temporal_constraints};
  study.witnesses = default_witnesses(draft.id);
  study.realizations.push_back({identity(draft.id + ":R:BASE"), draft.id,
      draft.baseline_label,
      draft.components, static_cast<std::uint32_t>(draft.components.size())});
  study.structures.push_back({identity(draft.id + ":R:BASE:SIGMA"), draft.id + ":R:BASE",
      draft.components, draft.relations});
  study.provenance.push_back({identity(draft.id + ":PROV"), "web_investigation_desk",
      "user supplied empirical protocol", timestamp_utc(), "Investigação criada pela API local do TinyKernel."});

  std::set<std::string> intervention_ids;
  for (std::size_t index = 0; index < draft.interventions.size(); ++index) {
    const auto &item = draft.interventions[index];
    const std::set<std::string> supported{"remove", "replace", "disable", "merge", "perturb"};
    if (!supported.contains(item.kind)) throw std::invalid_argument("unsupported preregistered intervention: " + item.kind);
    if (item.target.empty() || std::find(draft.components.begin(), draft.components.end(), item.target) == draft.components.end()) {
      throw std::invalid_argument("preregistered intervention target must exist in the baseline");
    }
    if ((item.kind == "replace" || item.kind == "merge" || item.kind == "perturb") && item.replacement.empty()) {
      throw std::invalid_argument("preregistered intervention requires replacement or perturbation metadata");
    }
    if (item.kind == "merge" && (item.replacement == item.target ||
        std::find(draft.components.begin(), draft.components.end(), item.replacement) == draft.components.end())) {
      throw std::invalid_argument("merge preregistration requires a distinct baseline component");
    }
    const auto suffix = upper_identifier(item.kind + "_" + (item.target.empty() ? std::to_string(index + 1) : item.target));
    const auto intervention_id = draft.id + ":I:" + suffix;
    if (!intervention_ids.insert(intervention_id).second) throw std::invalid_argument("duplicate preregistered intervention");
    study.interventions.push_back({identity(intervention_id), draft.id, item.kind,
        draft.id + ":R:BASE", std::nullopt, item.target, item.replacement,
        "BROKEN_CAUSAL", "planned"});
    if (item.kind == "remove" || item.kind == "disable") {
      study.claims.push_back({identity(draft.id + ":Q:" + upper_identifier(item.target) + "_NECESSITY"),
          item.target, "A relação associada possui necessidade relativa nesta realização.",
          draft.id + ":P", draft.id + ":C", {intervention_id},
          {draft.id + ":W:CAUSAL", draft.id + ":W:TEMPORAL"}, {},
          ClaimLevel::l3_relative_necessity, ClaimStatus::open,
          "Aguardando ruptura causal empírica adjudicada.", draft.id + ":PROV"});
    }
  }
  study.claims.insert(study.claims.begin(), {identity(draft.id + ":Q:SUFFICIENCY"), draft.id + ":R:BASE",
      "A realização baseline é suficiente sob o protocolo preregistrado.", draft.id + ":P", draft.id + ":C", {},
      {draft.id + ":W:OPERATIONAL", draft.id + ":W:CAUSAL", draft.id + ":W:DISCRIMINATIVE",
       draft.id + ":W:OBSERVATIONAL", draft.id + ":W:TEMPORAL"}, {}, ClaimLevel::l2_relative_sufficiency,
      ClaimStatus::open, "Aguardando observação empírica completa da baseline.", draft.id + ":PROV"});
  study.claims.push_back({identity(draft.id + ":Q:RELATIVE_MINIMALITY"), draft.id + ":R:BASE",
      "A realização é minimal na ordem Gamma declarada.", draft.id + ":P", draft.id + ":C", {}, {}, {},
      ClaimLevel::l5_relative_minimality, ClaimStatus::open,
      "Espaço incompleto: intervenções planejadas permanecem abertas.", draft.id + ":PROV"});

  const std::string artifact = "investigation=" + draft.id + "\nstatus=FORMULATED\nbaseline=" +
      draft.id + ":R:BASE\ncomponents=" + std::to_string(draft.components.size()) + "\n";
  study.evidence.push_back({identity(draft.id + ":E:FORMULATION_PROTOCOL"), draft.id + ":RUN:SPECIFICATION",
      draft.id + ":W:OBSERVATIONAL", {}, artifact, evidence::sha256(artifact), EvidenceType::structural_record});
  return study;
}

void materialize_intervention(Study &study, const std::string &planned_id,
                              const std::string &source_id, const std::string &kind,
                              const std::string &target, const std::string &replacement,
                              const std::string &protocol) {
  if (planned_id.empty()) throw std::invalid_argument("materialization requires a sealed preregistration id");
  if (protocol.empty()) throw std::invalid_argument("materialization protocol is required");
  auto planned = planned_id.empty() ? study.interventions.end() :
      std::find_if(study.interventions.begin(), study.interventions.end(),
          [&](const auto &item) { return item.identity.id == planned_id; });
  if (!planned_id.empty() && planned == study.interventions.end()) throw std::invalid_argument("planned intervention not found");
  if (planned != study.interventions.end() && planned->status == "performed") throw std::invalid_argument("intervention already performed");

  const auto effective_source = planned != study.interventions.end() ? planned->source_realization_id : source_id;
  const auto effective_kind = planned != study.interventions.end() ? planned->kind : kind;
  const auto effective_target = planned != study.interventions.end() ? planned->target : target;
  const auto effective_replacement = planned != study.interventions.end() ? planned->replacement : replacement;
  if (planned != study.interventions.end()) {
    const auto conflicts = [](const std::string &supplied, const std::string &sealed) {
      return !supplied.empty() && supplied != sealed;
    };
    if (conflicts(source_id, effective_source) || conflicts(kind, effective_kind) ||
        conflicts(target, effective_target) || conflicts(replacement, effective_replacement)) {
      throw std::invalid_argument("materialization parameters conflict with sealed preregistration");
    }
  }

  const auto baseline = std::find_if(study.realizations.begin(), study.realizations.end(), [](const auto &item) {
    return item.identity.id.find(":BASE") != std::string::npos;
  });
  const auto baseline_run = baseline == study.realizations.end() ? study.runs.end() :
      std::find_if(study.runs.begin(), study.runs.end(), [&](const auto &item) {
        return item.result_realization_id == baseline->identity.id;
      });
  const bool baseline_preserved = baseline_run != study.runs.end() &&
      std::any_of(study.adjudications.begin(), study.adjudications.end(), [&](const auto &item) {
        return item.run_id == baseline_run->identity.id && item.classification == "PRESERVED";
      });
  if (!baseline_preserved) throw std::invalid_argument("materialization requires a PRESERVED baseline adjudication");

  const auto source = std::find_if(study.realizations.begin(), study.realizations.end(),
      [&](const auto &item) { return item.identity.id == effective_source; });
  if (source == study.realizations.end()) throw std::invalid_argument("source realization not found");

  std::size_t sequence = study.realizations.size();
  std::string result_id;
  do { result_id = study.investigation.identity.id + ":R:INT_" + std::to_string(sequence++); }
  while (std::any_of(study.realizations.begin(), study.realizations.end(), [&](const auto &item) { return item.identity.id == result_id; }));
  const auto intervention_id = planned != study.interventions.end() ? planned->identity.id :
      study.investigation.identity.id + ":I:" + upper_identifier(effective_kind + "_" + effective_target + "_" + std::to_string(sequence));
  Intervention intervention{identity(intervention_id), study.investigation.identity.id, effective_kind, effective_source,
      result_id, effective_target, effective_replacement, "BROKEN_CAUSAL", "performed"};
  causal::InterventionEngine engine;
  auto result = engine.apply(*source, intervention);
  result.investigation_id = study.investigation.identity.id;
  if (planned != study.interventions.end()) *planned = intervention;
  else study.interventions.push_back(intervention);
  study.realizations.push_back(result);
  const auto source_structure = std::find_if(study.structures.begin(), study.structures.end(),
      [&](const auto &item) { return item.realization_id == effective_source; });
  const auto source_relations = source_structure == study.structures.end()
      ? study.constitutive_profile.relations : source_structure->relations;
  study.structures.push_back({identity(result_id + ":SIGMA"), result_id, result.components,
      derived_relations(source_relations, effective_kind, effective_target, effective_replacement)});
  const auto run_id = study.investigation.identity.id + ":RUN:" + result_id.substr(result_id.rfind(':') + 1);
  study.runs.push_back({identity(run_id), study.investigation.identity.id, intervention_id,
      effective_source, result_id, "untested"});
  const std::string artifact = "run=" + run_id + "\nsource=" + effective_source + "\nintervention=" + intervention_id +
      "\nrealization=" + result_id + "\nprotocol=" + protocol + "\n";
  study.evidence.push_back({identity(intervention_id + ":E:STRUCTURAL_ASSEMBLY"), run_id,
      study.investigation.identity.id + ":W:OBSERVATIONAL", {}, artifact,
      evidence::sha256(artifact), EvidenceType::structural_record});
  advance_phase(study.investigation, InvestigationPhase::materialized);
}

void record_observation(Study &study, const std::string &realization_id,
                        const std::string &dimension, bool satisfied,
                        const std::string &trace) {
  if (trace.empty()) throw std::invalid_argument("empirical trace is required");
  const auto realization = std::find_if(study.realizations.begin(), study.realizations.end(),
      [&](const auto &item) { return item.identity.id == realization_id; });
  if (realization == study.realizations.end()) throw std::invalid_argument("realization not found");
  const auto witness = std::find_if(study.witnesses.begin(), study.witnesses.end(),
      [&](const auto &item) { return item.kind == dimension; });
  if (witness == study.witnesses.end()) throw std::invalid_argument("witness dimension not found");
  auto run = std::find_if(study.runs.begin(), study.runs.end(),
      [&](const auto &item) { return item.result_realization_id == realization_id; });
  if (run == study.runs.end()) {
    const auto run_id = study.investigation.identity.id + ":RUN:BASELINE_EMPIRICAL";
    study.runs.push_back({identity(run_id), study.investigation.identity.id, std::nullopt,
        realization_id, realization_id, "in_progress"});
    run = std::prev(study.runs.end());
  }
  std::size_t revision = static_cast<std::size_t>(std::count_if(study.observations.begin(), study.observations.end(),
      [&](const auto &item) { return item.run_id == run->identity.id && item.dimension == dimension; }));
  std::ostringstream suffix;
  if (revision > 0) suffix << ":REV:" << std::setw(4) << std::setfill('0') << revision;
  const auto observation_id = run->identity.id + ":O:" + dimension + suffix.str();
  const auto evidence_id = run->identity.id + ":E:" + dimension + suffix.str();
  study.observations.push_back({identity(observation_id), run->identity.id, realization_id,
      witness->identity.id, dimension, satisfied ? "satisfied" : "not_satisfied", satisfied});
  const std::string artifact = "run=" + run->identity.id + "\nrealization=" + realization_id +
      "\ndimension=" + dimension + "\nsatisfied=" + (satisfied ? "true" : "false") +
      "\nempirical_trace=" + trace + "\n";
  study.evidence.push_back({identity(evidence_id), run->identity.id, witness->identity.id,
      {observation_id}, artifact, evidence::sha256(artifact), EvidenceType::empirical_observation});

  // A revision never erases sealed evidence, but it does invalidate conclusions
  // computed from the previous observation set until adjudication runs again.
  const auto belongs_to_run = [&](const std::string &reference) {
    const auto item = std::find_if(study.evidence.begin(), study.evidence.end(), [&](const auto &candidate) {
      return candidate.identity.id == reference;
    });
    return item != study.evidence.end() && item->run_id == run->identity.id;
  };
  for (auto &claim : study.claims) {
    if (std::any_of(claim.evidence_references.begin(), claim.evidence_references.end(), belongs_to_run)) {
      claim.status = ClaimStatus::open;
      std::erase_if(claim.evidence_references, belongs_to_run);
      claim.limitations = "Observação revisada; aguardando nova adjudicação e inferência.";
    }
  }
  const auto prior_adjudication = std::find_if(study.adjudications.begin(), study.adjudications.end(),
      [&](const auto &item) { return item.run_id == run->identity.id; });
  if (prior_adjudication != study.adjudications.end()) {
    upsert_adjudication(study, *run, "STALE", {},
        "Uma observação foi revisada; esta adjudicação precisa ser recalculada.");
  }
  run->status = "in_progress";
  advance_phase(study.investigation, InvestigationPhase::observed);
}

void adjudicate_observations(Study &study) {
  if (study.observations.empty()) throw std::invalid_argument("adjudication requires recorded observations");
  for (auto &run : study.runs) {
    std::map<std::string, const Observation *> latest;
    for (const auto &observation : study.observations) {
      if (observation.run_id == run.identity.id) latest[observation.dimension] = &observation;
    }
    if (latest.empty()) continue;
    std::vector<std::string> evidence_ids;
    for (const auto &[dimension, observation] : latest) {
      static_cast<void>(dimension);
      const auto item = std::find_if(study.evidence.begin(), study.evidence.end(), [&](const auto &candidate) {
        return std::find(candidate.observation_ids.begin(), candidate.observation_ids.end(), observation->identity.id) != candidate.observation_ids.end();
      });
      if (item != study.evidence.end()) evidence_ids.push_back(item->identity.id);
    }
    std::string classification = "PRESERVED";
    std::string rationale = "Todos os witnesses constitutivos foram empiricamente satisfeitos.";
    if (latest.size() < study.witnesses.size()) {
      classification = "PARTIALLY_OBSERVED";
      rationale = "Observação parcial; todos os witnesses preregistrados são necessários para PRESERVED.";
    } else {
      const auto broken = [&](const std::string &kind) {
        const auto found = latest.find(kind);
        return found != latest.end() && !found->second->satisfied;
      };
      if (broken("observational")) classification = "WITNESS_COMPROMISED";
      else if (broken("causal")) classification = "BROKEN_CAUSAL";
      else if (broken("operational")) classification = "BROKEN_OPERATIONAL";
      else if (broken("discriminative")) classification = "BROKEN_DISCRIMINATIVE";
      else if (broken("temporal")) classification = "BROKEN_TEMPORAL";
      if (classification != "PRESERVED") rationale = "Ao menos um witness constitutivo preregistrado não foi satisfeito.";
    }
    upsert_adjudication(study, run, classification, std::move(evidence_ids), std::move(rationale));
    run.status = "completed";
  }
  advance_phase(study.investigation, InvestigationPhase::adjudicated);
}

void infer_observed_claims(Study &study) {
  if (study.adjudications.empty()) throw std::invalid_argument("inference requires adjudication");
  if (std::any_of(study.adjudications.begin(), study.adjudications.end(), [](const auto &item) {
        return item.classification == "STALE";
      })) throw std::invalid_argument("inference blocked by stale adjudication");
  const auto baseline = std::find_if(study.runs.begin(), study.runs.end(), [&](const auto &run) {
    return run.result_realization_id == study.investigation.identity.id + ":R:BASE";
  });
  const auto baseline_adjudication = baseline == study.runs.end() ? study.adjudications.end() :
      std::find_if(study.adjudications.begin(), study.adjudications.end(), [&](const auto &item) {
        return item.run_id == baseline->identity.id && item.classification == "PRESERVED";
      });
  for (auto &claim : study.claims) {
    if (claim.level == ClaimLevel::l2_relative_sufficiency && baseline_adjudication != study.adjudications.end()) {
      claim.evidence_references = baseline_adjudication->evidence_references;
    }
    if (claim.level != ClaimLevel::l3_relative_necessity || baseline_adjudication == study.adjudications.end()) continue;
    for (const auto &intervention_id : claim.intervention_scope) {
      const auto run = std::find_if(study.runs.begin(), study.runs.end(), [&](const auto &item) {
        return item.intervention_id && *item.intervention_id == intervention_id;
      });
      if (run == study.runs.end()) continue;
      const auto adjudication = std::find_if(study.adjudications.begin(), study.adjudications.end(), [&](const auto &item) {
        return item.run_id == run->identity.id && item.classification == "BROKEN_CAUSAL";
      });
      if (adjudication == study.adjudications.end()) continue;
      claim.evidence_references = baseline_adjudication->evidence_references;
      claim.evidence_references.insert(claim.evidence_references.end(), adjudication->evidence_references.begin(), adjudication->evidence_references.end());
    }
  }
  knowledge::infer_claims(study);
}

std::string workflow_projection_json(const Study &study) {
  const auto quote = [](const std::string &value) {
    std::ostringstream out;
    out << '"';
    for (const unsigned char character : value) {
      switch (character) {
      case '"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (character < 0x20U) {
          constexpr char digits[] = "0123456789abcdef";
          out << "\\u00" << digits[character >> 4U] << digits[character & 0x0fU];
        } else out << static_cast<char>(character);
      }
    }
    out << '"';
    return out.str();
  };
  const auto string_array = [&](std::ostringstream &out, const auto &values) {
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
      if (index) out << ',';
      out << quote(values[index]);
    }
    out << ']';
  };
  const auto baseline = study.realizations.empty() ? study.realizations.end() :
      std::find_if(study.realizations.begin(), study.realizations.end(), [](const auto &item) {
        return item.identity.id.find(":BASE") != std::string::npos;
      });
  const auto actual_baseline = baseline == study.realizations.end() && !study.realizations.empty()
      ? study.realizations.begin() : baseline;
  const auto missing_for = [&](const std::string &realization_id) {
    std::set<std::string> observed;
    for (const auto &observation : study.observations) {
      if (observation.realization_id == realization_id) observed.insert(observation.dimension);
    }
    std::vector<std::string> missing;
    for (const auto &witness : study.witnesses) if (!observed.contains(witness.kind)) missing.push_back(witness.kind);
    return missing;
  };

  struct Ranked { const Intervention *intervention{}; int score{}; std::string reason; };
  std::vector<Ranked> ranking;
  for (const auto &intervention : study.interventions) {
    if (intervention.status == "performed") continue;
    const auto claim_match = std::any_of(study.claims.begin(), study.claims.end(), [&](const auto &claim) {
      return claim.status != ClaimStatus::supported &&
          (claim.subject == intervention.target || claim.identity.id.find(upper_identifier(intervention.target)) != std::string::npos);
    });
    const bool causal_operator = intervention.kind == "remove" || intervention.kind == "disable" || intervention.kind == "replace";
    ranking.push_back({&intervention, (claim_match ? 4 : 0) + (causal_operator ? 2 : 1),
        claim_match
          ? "Esta possibilidade testa diretamente um claim aberto e reduz a fronteira experimental."
          : "Esta possibilidade ainda não foi explorada e amplia a cobertura do espaço causal."});
  }
  std::sort(ranking.begin(), ranking.end(), [](const auto &left, const auto &right) {
    if (left.score != right.score) return left.score > right.score;
    return left.intervention->identity.id < right.intervention->identity.id;
  });

  struct Action {
    std::string type, title, reason, realization_id, dimension, intervention_id;
    std::vector<std::string> blockers;
  } action;
  std::vector<std::string> baseline_missing = actual_baseline == study.realizations.end()
      ? std::vector<std::string>{} : missing_for(actual_baseline->identity.id);
  const Realization *complete_unadjudicated = nullptr;
  for (const auto &realization : study.realizations) {
    if (!missing_for(realization.identity.id).empty()) continue;
    const auto run = std::find_if(study.runs.begin(), study.runs.end(), [&](const auto &item) {
      return item.result_realization_id == realization.identity.id;
    });
    if (run == study.runs.end()) continue;
    const bool adjudicated = std::any_of(study.adjudications.begin(), study.adjudications.end(), [&](const auto &item) {
      return item.run_id == run->identity.id && item.classification != "STALE";
    });
    if (!adjudicated) { complete_unadjudicated = &realization; break; }
  }
  const Intervention *incomplete_intervention = nullptr;
  std::vector<std::string> incomplete_missing;
  for (const auto &intervention : study.interventions) {
    if (intervention.status != "performed" || !intervention.target_realization_id) continue;
    auto missing = missing_for(*intervention.target_realization_id);
    if (!missing.empty()) { incomplete_intervention = &intervention; incomplete_missing = std::move(missing); break; }
  }
  const bool has_fresh_adjudication = std::any_of(study.adjudications.begin(), study.adjudications.end(), [](const auto &item) {
    return item.classification != "STALE";
  });
  const auto projected_baseline_run = actual_baseline == study.realizations.end() ? study.runs.end() :
      std::find_if(study.runs.begin(), study.runs.end(), [&](const auto &run) {
        return run.result_realization_id == actual_baseline->identity.id;
      });
  const bool projected_baseline_preserved = projected_baseline_run != study.runs.end() &&
      std::any_of(study.adjudications.begin(), study.adjudications.end(), [&](const auto &item) {
        return item.run_id == projected_baseline_run->identity.id && item.classification == "PRESERVED";
      });
  const bool has_stale_adjudication = std::any_of(study.adjudications.begin(), study.adjudications.end(),
      [](const auto &item) { return item.classification == "STALE"; });
  bool has_inferable_claim = false;
  if (has_fresh_adjudication && !has_stale_adjudication) {
    auto projected_inference = study;
    infer_observed_claims(projected_inference);
    for (std::size_t index = 0; index < study.claims.size(); ++index) {
      if (study.claims[index].status == ClaimStatus::open &&
          projected_inference.claims[index].status == ClaimStatus::supported) {
        has_inferable_claim = true;
        break;
      }
    }
  }
  if (actual_baseline == study.realizations.end()) {
    action = {"formulate", "Definir a realização baseline", "Não existe um mundo de referência para comparar intervenções.", {}, {}, {}, {"baseline ausente"}};
  } else if (!baseline_missing.empty()) {
    action = {"observe", "Completar a observação da baseline",
        "A baseline ainda não possui todos os witnesses preregistrados.", actual_baseline->identity.id,
        baseline_missing.front(), {}, baseline_missing};
  } else if (complete_unadjudicated) {
    action = {"adjudicate", "Adjudicar observações completas",
        complete_unadjudicated->label + " já possui todos os witnesses necessários.", {}, {}, {}, {}};
  } else if (incomplete_intervention) {
    action = {"observe", "Observar realização derivada",
        "A intervenção foi materializada, mas ainda faltam dimensões empíricas.",
        *incomplete_intervention->target_realization_id, incomplete_missing.front(), {}, incomplete_missing};
  } else if (has_inferable_claim) {
    action = {"infer", "Inferir claims elegíveis", "Há adjudicações disponíveis para avaliação formal dos claims.", {}, {}, {}, {}};
  } else if (!ranking.empty()) {
    action = {"materialize", "Explorar " + ranking.front().intervention->kind + "(" + ranking.front().intervention->target + ")",
        ranking.front().reason, {}, {}, ranking.front().intervention->identity.id,
        {std::to_string(ranking.size()) + " possibilidades abertas"}};
  } else {
    action = {"complete", "Revisar a fronteira conhecida",
        "Não há operações mecânicas pendentes. Revise limitações ou formule uma nova intervenção.", {}, {}, {}, {}};
  }

  const std::map<std::string, int> phase_rank{{"formulated",0},{"materialized",1},{"observed",2},{"adjudicated",3},{"inferred",4},{"executed",4}};
  const auto rank_item = phase_rank.find(study.investigation.status);
  const int current_phase_index = rank_item == phase_rank.end() ? 0 : rank_item->second;
  const std::vector<std::string> phases{"formulated","materialized","observed","adjudicated","inferred"};
  const auto performed_count = static_cast<std::size_t>(std::count_if(study.interventions.begin(), study.interventions.end(),
      [](const auto &item) { return item.status == "performed"; }));

  std::ostringstream out;
  out << "{\"current_phase\":" << quote(phases[static_cast<std::size_t>(current_phase_index)])
      << ",\"current_phase_index\":" << current_phase_index << ",\"phases\":";
  string_array(out, phases);
  out << ",\"action\":{\"type\":" << quote(action.type) << ",\"title\":" << quote(action.title)
      << ",\"reason\":" << quote(action.reason) << ",\"target_realization_id\":" << quote(action.realization_id)
      << ",\"target_dimension\":" << quote(action.dimension) << ",\"intervention_id\":" << quote(action.intervention_id)
      << ",\"blockers\":"; string_array(out, action.blockers); out << "}";
  out << ",\"completeness\":{\"baseline\":{\"observed\":"
      << (actual_baseline == study.realizations.end() ? 0 : study.witnesses.size() - baseline_missing.size())
      << ",\"total\":" << study.witnesses.size()
      << ",\"missing\":"; string_array(out, baseline_missing);
  out << "},\"performed_interventions\":" << performed_count
      << ",\"planned_interventions\":" << ranking.size() << "}";
  out << ",\"allowed_actions\":[";
  bool separator = false;
  const auto allowed = [&](const std::string &value) {
    if (separator) out << ',';
    out << quote(value);
    separator = true;
  };
  if (!study.realizations.empty()) allowed("observe");
  if (!ranking.empty() && projected_baseline_preserved) allowed("materialize");
  if (!study.observations.empty()) allowed("adjudicate");
  if (has_fresh_adjudication && has_inferable_claim) allowed("infer");
  out << "]";
  out << ",\"intervention_ranking\":[";
  for (std::size_t index = 0; index < ranking.size(); ++index) {
    if (index) out << ',';
    out << "{\"intervention_id\":" << quote(ranking[index].intervention->identity.id)
        << ",\"score\":" << ranking[index].score << ",\"reason\":" << quote(ranking[index].reason) << "}";
  }
  out << "],\"counterfactual_previews\":[";
  bool first_preview = true;
  causal::InterventionEngine engine;
  for (const auto &ranked : ranking) {
    const auto source = std::find_if(study.realizations.begin(), study.realizations.end(), [&](const auto &item) {
      return item.identity.id == ranked.intervention->source_realization_id;
    });
    if (source == study.realizations.end()) continue;
    try {
      const auto result = engine.apply(*source, *ranked.intervention);
      std::vector<std::string> removed, added, affected;
      for (const auto &part : source->components) if (std::find(result.components.begin(), result.components.end(), part) == result.components.end()) removed.push_back(part);
      for (const auto &part : result.components) if (std::find(source->components.begin(), source->components.end(), part) == source->components.end()) added.push_back(part);
      for (const auto &claim : study.claims) if (claim.subject == ranked.intervention->target) affected.push_back(claim.identity.id);
      if (!first_preview) out << ',';
      first_preview = false;
      out << "{\"intervention_id\":" << quote(ranked.intervention->identity.id)
          << ",\"source_id\":" << quote(source->identity.id) << ",\"before\":"; string_array(out, source->components);
      out << ",\"after\":"; string_array(out, result.components); out << ",\"removed\":"; string_array(out, removed);
      out << ",\"added\":"; string_array(out, added); out << ",\"affected_claims\":"; string_array(out, affected); out << '}';
    } catch (const std::invalid_argument &) {}
  }
  out << "],\"claim_explanations\":[";
  for (std::size_t index = 0; index < study.claims.size(); ++index) {
    const auto &claim = study.claims[index];
    if (index) out << ',';
    std::vector<std::pair<std::string, bool>> steps;
    const auto baseline_run = std::find_if(study.runs.begin(), study.runs.end(), [](const auto &run) {
      return run.identity.id.find("BASELINE") != std::string::npos;
    });
    const bool baseline_preserved = baseline_run != study.runs.end() &&
        std::any_of(study.adjudications.begin(), study.adjudications.end(), [&](const auto &item) {
          return item.run_id == baseline_run->identity.id && item.classification == "PRESERVED";
        });
    steps.emplace_back("Baseline adjudicada como PRESERVED", baseline_preserved);
    if (claim.level == ClaimLevel::l3_relative_necessity) {
      const auto intervention = std::find_if(study.interventions.begin(), study.interventions.end(), [&](const auto &item) {
        return std::find(claim.intervention_scope.begin(), claim.intervention_scope.end(), item.identity.id) != claim.intervention_scope.end();
      });
      const bool performed = intervention != study.interventions.end() && intervention->status == "performed";
      steps.emplace_back("Intervenção sobre " + claim.subject + " materializada", performed);
      const bool ruptured = performed && std::any_of(study.runs.begin(), study.runs.end(), [&](const auto &run) {
        if (!run.intervention_id || *run.intervention_id != intervention->identity.id) return false;
        return std::any_of(study.adjudications.begin(), study.adjudications.end(), [&](const auto &item) {
          return item.run_id == run.identity.id && item.classification == "BROKEN_CAUSAL";
        });
      });
      steps.emplace_back("Ruptura causal empírica adjudicada", ruptured);
    } else if (claim.level == ClaimLevel::l5_relative_minimality) {
      steps.emplace_back("Todas as intervenções preregistradas exploradas", ranking.empty());
      steps.emplace_back("Limite de escopo explicitamente preservado", !claim.limitations.empty());
    }
    steps.emplace_back("Evidências vinculadas (" + std::to_string(claim.evidence_references.size()) + ")", !claim.evidence_references.empty());
    const auto blocker = std::find_if(steps.begin(), steps.end(), [](const auto &step) { return !step.second; });
    out << "{\"claim_id\":" << quote(claim.identity.id) << ",\"supported\":"
        << (claim.status == ClaimStatus::supported ? "true" : "false") << ",\"next_blocker\":"
        << (blocker == steps.end() ? "null" : quote(blocker->first)) << ",\"steps\":[";
    for (std::size_t step_index = 0; step_index < steps.size(); ++step_index) {
      if (step_index) out << ',';
      out << "{\"label\":" << quote(steps[step_index].first) << ",\"passed\":" << (steps[step_index].second ? "true" : "false") << "}";
    }
    out << "]}";
  }
  out << "]}";
  return out.str();
}

} // namespace tinykernel::experiment
