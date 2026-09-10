#include "tinykernel/experiment/laboratory.hpp"

#include "tinykernel/evidence/digest.hpp"
#include "tinykernel/knowledge/analysis.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace tinykernel::experiment {
namespace {

using namespace ontology;

Identity id(std::string value) { return Identity{std::move(value), 1, "0.2.0"}; }

bool has_component(const Realization &realization, const std::string &component) {
  return std::find(realization.components.begin(), realization.components.end(), component) != realization.components.end();
}

class Tk0000Adapter final : public RealizationAdapter {
public:
  AdapterObservation observe(const Realization &realization) const override {
    const bool marker = has_component(realization, "sentinel");
    return {true, marker, marker, true, marker,
            std::string("sentinel_present=") + (marker ? "true" : "false")};
  }
};

class Tk0001Adapter final : public RealizationAdapter {
public:
  AdapterObservation observe(const Realization &realization) const override {
    const bool update = has_component(realization, "update");
    const bool persistence = has_component(realization, "persistence");
    const bool feedback = has_component(realization, "feedback") ||
                          has_component(realization, "feedback_equivalent");
    const bool state = has_component(realization, "state");
    const bool adaptive = update && persistence && feedback && state;
    return {
      true,
      update && state,
      adaptive,
      true,
      adaptive,
      "baseline_action=0;experience_target=1;state_after=" + std::string(adaptive ? "1" : "0") +
          ";later_action=" + std::string(adaptive ? "1" : "0") + ";experience_represented_later=false"
    };
  }
};

bool dimension_value(const AdapterObservation &result, const std::string &dimension) {
  if (dimension == "operational") return result.operational;
  if (dimension == "causal") return result.causal;
  if (dimension == "discriminative") return result.discriminative;
  if (dimension == "observational") return result.observable;
  if (dimension == "temporal") return result.temporal;
  throw std::logic_error("unknown witness dimension");
}

std::string classification_for(const AdapterObservation &result) {
  if (!result.observable) return "WITNESS_COMPROMISED";
  if (!result.operational) return "BROKEN_OPERATIONAL";
  if (!result.causal) return "BROKEN_CAUSAL";
  if (!result.discriminative) return "BROKEN_DISCRIMINATIVE";
  if (!result.temporal) return "BROKEN_TEMPORAL";
  return "PRESERVED";
}

Outcome outcome_for(const std::string &classification) {
  if (classification == "PRESERVED") return Outcome::preserving;
  if (classification == "WITNESS_COMPROMISED" || classification == "INCONCLUSIVE") {
    return Outcome::undetermined;
  }
  return Outcome::ruptured;
}

std::vector<Witness> witnesses(const std::string &prefix) {
  return {
      {id(prefix + ":W:OPERATIONAL"), "operational", "O sistema produz comportamento observável."},
      {id(prefix + ":W:CAUSAL"), "causal", "A saída depende da relação causal preregistrada."},
      {id(prefix + ":W:DISCRIMINATIVE"), "discriminative", "As distinções do perfil permanecem separáveis."},
      {id(prefix + ":W:OBSERVATIONAL"), "observational", "O aparato permanece capaz de observar."},
      {id(prefix + ":W:TEMPORAL"), "temporal", "A restrição temporal preregistrada é satisfeita."},
  };
}

void record_run(Study &study, const Realization &realization,
                const std::optional<Intervention> &intervention,
                const RealizationAdapter &adapter, const std::string &run_suffix) {
  const std::string run_id = study.investigation.identity.id + ":RUN:" + run_suffix;
  study.runs.push_back({id(run_id), study.investigation.identity.id,
                        intervention ? std::optional(intervention->identity.id) : std::nullopt,
                        intervention ? intervention->source_realization_id : realization.identity.id,
                        realization.identity.id, "completed"});
  const auto result = adapter.observe(realization);
  std::vector<std::string> evidence_ids;
  for (const auto &witness : study.witnesses) {
    const bool satisfied = dimension_value(result, witness.kind);
    const std::string observation_id = run_id + ":O:" + witness.kind;
    study.observations.push_back({id(observation_id), run_id, realization.identity.id,
                                  witness.identity.id, witness.kind,
                                  satisfied ? "satisfied" : "not_satisfied", satisfied});
    const std::string artifact = "run=" + run_id + "\nrealization=" + realization.identity.id +
        "\ndimension=" + witness.kind + "\nsatisfied=" + (satisfied ? "true" : "false") +
        "\ntrace=" + result.artifact + "\n";
    const std::string evidence_id = run_id + ":E:" + witness.kind;
    study.evidence.push_back({id(evidence_id), run_id, witness.identity.id,
                              {observation_id}, artifact, evidence::sha256(artifact)});
    evidence_ids.push_back(evidence_id);
  }
  const auto classification = classification_for(result);
  study.adjudications.push_back({id(run_id + ":A"), run_id, outcome_for(classification),
      classification, "TK-O-0.2.0:all-constitutive-dimensions-v1",
      classification == "PRESERVED"
          ? "Todos os witnesses constitutivos preregistrados foram satisfeitos."
          : "Ao menos uma dimensão constitutiva preregistrada não foi satisfeita.",
      std::move(evidence_ids)});
}

Study make_base(const std::string &name, const std::string &title) {
  Study study;
  study.investigation = {id(name), title, name + ":P", name + ":C", name + ":PHI",
                         "Gamma=active_causal_relations", "preregistered"};
  study.context = {id(name + ":C"), "Execução local determinística, processo único, inteiros binários."};
  study.witnesses = witnesses(name);
  study.provenance.push_back({id(name + ":PROV"), "preregistration",
      "deterministic built-in adapter", "2026-09-09T00:00:00-03:00",
      "Materialização TK-SYS-00 derivada de TK-FND-00 v0.2.0."});
  return study;
}

} // namespace

Laboratory::Laboratory(causal::InterventionEngine engine) : engine_(std::move(engine)) {}

Study Laboratory::execute(Study study, const RealizationAdapter &adapter) const {
  if (!study.runs.empty()) return study;
  if (study.realizations.empty()) throw std::invalid_argument("investigation has no baseline realization");

  const auto baseline = study.realizations.front();
  record_run(study, baseline, std::nullopt, adapter, "BASELINE");

  for (auto &intervention : study.interventions) {
    if (intervention.status != "preregistered") continue;
    const auto source = std::find_if(study.realizations.begin(), study.realizations.end(),
        [&](const auto &item) { return item.identity.id == intervention.source_realization_id; });
    if (source == study.realizations.end()) throw std::logic_error("intervention source missing");
    auto result = engine_.apply(*source, intervention);
    intervention.target_realization_id = result.identity.id;
    intervention.status = "performed";
    study.realizations.push_back(result);
    study.structures.push_back({id(result.identity.id + ":SIGMA"), result.identity.id,
                                result.components, result.components});
    record_run(study, result, intervention, adapter, intervention.identity.id.substr(intervention.identity.id.rfind(':') + 1));
  }

  knowledge::ClaimAdjudicator adjudicator;
  for (auto &claim : study.claims) {
    const auto decision = adjudicator.may_support(claim, study);
    claim.status = decision.allowed ? ClaimStatus::supported : ClaimStatus::open;
  }
  study.investigation.status = "executed";
  return study;
}

Study make_tk0000() {
  auto study = make_base("TK-0000", "Bootstrap do aparato experimental");
  study.phenomenon = {id("TK-0000:P"), "sentinela operacional",
                      "Presença observável de um marcador sentinela."};
  study.constitutive_profile = {id("TK-0000:PHI"), {"sentinela presente/ausente"},
                                {"sentinela determina witness"}, {"observação após intervenção"}};
  study.realizations.push_back({id("TK-0000:R:BASE"), "TK-0000", "baseline sentinela",
                                {"sentinel"}, 1});
  study.structures.push_back({id("TK-0000:R:BASE:SIGMA"), "TK-0000:R:BASE",
                              {"sentinel"}, {"sentinel->witness"}});
  study.interventions.push_back({id("TK-0000:I:REMOVE_SENTINEL"), "TK-0000", "remove",
      "TK-0000:R:BASE", "TK-0000:R:NO_SENTINEL", "sentinel", "",
      "BROKEN_CAUSAL", "preregistered"});
  return study;
}

Study make_tk0001() {
  auto study = make_base("TK-0001", "Persistência adaptativa");
  study.phenomenon = {id("TK-0001:P"), "persistência adaptativa",
      "Uma experiência altera estado persistido e comportamento posterior sem reapresentação da experiência."};
  study.constitutive_profile = {id("TK-0001:PHI"),
      {"estado antes/depois", "experiência presente/ausente", "comportamento baseline/posterior"},
      {"experiência->alteração persistida", "alteração persistida->comportamento posterior"},
      {"alteração após experiência", "persistência até interação posterior sem experiência"}};
  study.realizations.push_back({id("TK-0001:R:BASE"), "TK-0001", "baseline adaptativo",
      {"state", "input", "action", "feedback", "difference", "update", "persistence", "later_interaction"}, 5});
  study.structures.push_back({id("TK-0001:R:BASE:SIGMA"), "TK-0001:R:BASE",
      {"state", "input", "action", "feedback", "difference", "update", "persistence", "later_interaction"},
      {"experience->difference", "feedback->difference", "difference->update", "update->persistence", "persistence->later_action"}});
  study.interventions = {
      {id("TK-0001:I:REPLACE_FEEDBACK"), "TK-0001", "replace", "TK-0001:R:BASE",
       "TK-0001:R:ALT_FEEDBACK", "feedback", "feedback_equivalent", "PRESERVED", "preregistered"},
      {id("TK-0001:I:REMOVE_UPDATE"), "TK-0001", "remove", "TK-0001:R:BASE",
       "TK-0001:R:NO_UPDATE", "update", "", "BROKEN_CAUSAL", "preregistered"},
      {id("TK-0001:I:DISABLE_PERSISTENCE"), "TK-0001", "disable", "TK-0001:R:BASE",
       std::nullopt, "persistence", "", "unexecuted", "planned"},
      {id("TK-0001:I:MERGE_STATE_ACTION"), "TK-0001", "merge", "TK-0001:R:BASE",
       std::nullopt, "state", "action", "unexecuted", "planned"},
      {id("TK-0001:I:PERTURB_FEEDBACK"), "TK-0001", "perturb", "TK-0001:R:BASE",
       std::nullopt, "feedback", "noise", "unexecuted", "planned"},
  };

  std::vector<std::string> baseline_evidence;
  std::vector<std::string> removal_evidence;
  for (const auto &witness : study.witnesses) {
    baseline_evidence.push_back("TK-0001:RUN:BASELINE:E:" + witness.kind);
    removal_evidence.push_back("TK-0001:RUN:REMOVE_UPDATE:E:" + witness.kind);
  }
  auto necessity_evidence = baseline_evidence;
  necessity_evidence.insert(necessity_evidence.end(), removal_evidence.begin(), removal_evidence.end());
  study.claims = {
      {id("TK-0001:Q:SUFFICIENCY"), "TK-0001:R:BASE",
       "A realização baseline é suficiente sob o protocolo preregistrado.", "TK-0001:P", "TK-0001:C",
       {}, {"TK-0001:W:OPERATIONAL", "TK-0001:W:CAUSAL", "TK-0001:W:DISCRIMINATIVE",
            "TK-0001:W:OBSERVATIONAL", "TK-0001:W:TEMPORAL"}, baseline_evidence,
       ClaimLevel::l2_relative_sufficiency, ClaimStatus::open,
       "Limitado ao contexto, perfil, witnesses e realização TK-0001 declarados.", "TK-0001:PROV"},
      {id("TK-0001:Q:UPDATE_NECESSITY"), "update",
       "A relação de atualização possui necessidade relativa nesta realização.", "TK-0001:P", "TK-0001:C",
       {"TK-0001:I:REMOVE_UPDATE"}, {"TK-0001:W:CAUSAL", "TK-0001:W:TEMPORAL"}, necessity_evidence,
       ClaimLevel::l3_relative_necessity, ClaimStatus::open,
       "Não transfere necessidade a outras realizações, contextos ou granularidades.", "TK-0001:PROV"},
      {id("TK-0001:Q:RELATIVE_MINIMALITY"), "TK-0001:R:BASE",
       "A realização é minimal na ordem Gamma declarada.", "TK-0001:P", "TK-0001:C",
       {}, {}, {}, ClaimLevel::l5_relative_minimality, ClaimStatus::open,
       "Espaço incompleto: reduções planejadas e realizações alternativas permanecem abertas.", "TK-0001:PROV"},
  };
  return study;
}

Study execute_tk0000() { return Laboratory{}.execute(make_tk0000(), Tk0000Adapter{}); }
Study execute_tk0001() { return Laboratory{}.execute(make_tk0001(), Tk0001Adapter{}); }

} // namespace tinykernel::experiment
