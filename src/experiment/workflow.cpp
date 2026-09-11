#include "tinykernel/experiment/workflow.hpp"

#include "tinykernel/causal/engine.hpp"
#include "tinykernel/evidence/digest.hpp"
#include "tinykernel/knowledge/analysis.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <map>
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
  if (draft.components.empty()) throw std::invalid_argument("baseline requires at least one component");
  Study study;
  study.investigation = {identity(draft.id), draft.title, draft.id + ":P", draft.id + ":C",
      draft.id + ":PHI", "Gamma=active_causal_relations", "formulated"};
  study.phenomenon = {identity(draft.id + ":P"), draft.title, draft.phenomenon_description};
  study.context = {identity(draft.id + ":C"), draft.context_description};
  study.constitutive_profile = {identity(draft.id + ":PHI"), draft.distinctions,
      draft.relations, draft.temporal_constraints};
  study.witnesses = default_witnesses(draft.id);
  study.realizations.push_back({identity(draft.id + ":R:BASE"), draft.id,
      draft.baseline_label.empty() ? "baseline completa" : draft.baseline_label,
      draft.components, static_cast<std::uint32_t>(draft.components.size())});
  study.structures.push_back({identity(draft.id + ":R:BASE:SIGMA"), draft.id + ":R:BASE",
      draft.components, draft.relations});
  study.provenance.push_back({identity(draft.id + ":PROV"), "web_investigation_desk",
      "user supplied empirical protocol", timestamp_utc(), "Investigação criada pela API local do TinyKernel."});

  for (std::size_t index = 0; index < draft.interventions.size(); ++index) {
    const auto &item = draft.interventions[index];
    const auto suffix = upper_identifier(item.kind + "_" + (item.target.empty() ? std::to_string(index + 1) : item.target));
    const auto intervention_id = draft.id + ":I:" + suffix;
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
  const auto source = std::find_if(study.realizations.begin(), study.realizations.end(),
      [&](const auto &item) { return item.identity.id == source_id; });
  if (source == study.realizations.end()) throw std::invalid_argument("source realization not found");
  auto planned = planned_id.empty() ? study.interventions.end() :
      std::find_if(study.interventions.begin(), study.interventions.end(),
          [&](const auto &item) { return item.identity.id == planned_id; });
  if (!planned_id.empty() && planned == study.interventions.end()) throw std::invalid_argument("planned intervention not found");
  if (planned != study.interventions.end() && planned->status == "performed") throw std::invalid_argument("intervention already performed");

  std::size_t sequence = study.realizations.size();
  std::string result_id;
  do { result_id = study.investigation.identity.id + ":R:INT_" + std::to_string(sequence++); }
  while (std::any_of(study.realizations.begin(), study.realizations.end(), [&](const auto &item) { return item.identity.id == result_id; }));
  const auto intervention_id = planned != study.interventions.end() ? planned->identity.id :
      study.investigation.identity.id + ":I:" + upper_identifier(kind + "_" + target + "_" + std::to_string(sequence));
  Intervention intervention{identity(intervention_id), study.investigation.identity.id, kind, source_id,
      result_id, target, replacement, "BROKEN_CAUSAL", "performed"};
  causal::InterventionEngine engine;
  auto result = engine.apply(*source, intervention);
  result.investigation_id = study.investigation.identity.id;
  if (planned != study.interventions.end()) *planned = intervention;
  else study.interventions.push_back(intervention);
  study.realizations.push_back(result);
  study.structures.push_back({identity(result_id + ":SIGMA"), result_id, result.components, result.components});
  const auto run_id = study.investigation.identity.id + ":RUN:" + result_id.substr(result_id.rfind(':') + 1);
  study.runs.push_back({identity(run_id), study.investigation.identity.id, intervention_id,
      source_id, result_id, "untested"});
  const std::string artifact = "run=" + run_id + "\nsource=" + source_id + "\nintervention=" + intervention_id +
      "\nrealization=" + result_id + "\nprotocol=" + (protocol.empty() ? "default" : protocol) + "\n";
  study.evidence.push_back({identity(intervention_id + ":E:STRUCTURAL_ASSEMBLY"), run_id,
      study.investigation.identity.id + ":W:OBSERVATIONAL", {}, artifact,
      evidence::sha256(artifact), EvidenceType::structural_record});
  advance_phase(study.investigation, InvestigationPhase::materialized);
}

void record_observation(Study &study, const std::string &realization_id,
                        const std::string &dimension, bool satisfied,
                        const std::string &trace) {
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

} // namespace tinykernel::experiment
