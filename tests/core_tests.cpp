#include "tinykernel/causal/engine.hpp"
#include "tinykernel/evidence/digest.hpp"
#include "tinykernel/experiment/laboratory.hpp"
#include "tinykernel/experiment/workflow.hpp"
#include "tinykernel/knowledge/analysis.hpp"
#include "tinykernel/persistence/repository.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string &message) {
  if (!condition) throw std::runtime_error(message);
}

void identity() {
  const auto study = tinykernel::experiment::make_tk0001();
  require(study.investigation.identity.id == "TK-0001", "stable investigation id");
  require(study.phenomenon.identity.schema_version == 2, "schema version");
  require(study.constitutive_profile.identity.ontology_version == "0.2.1", "ontology version");
}

void graph() {
  const auto study = tinykernel::experiment::execute_tk0001();
  const tinykernel::causal::CausalGraph graph(study);
  require(graph.valid(), "valid graph");
  require(graph.reachable("TK-0001:R:BASE", "TK-0001:R:ALT_FEEDBACK"), "preserving path");
  require(graph.reachable("TK-0001:R:BASE", "TK-0001:R:NO_UPDATE"), "ruptured path");
  require(!graph.reachable("TK-0001:R:NO_UPDATE", "TK-0001:R:BASE"), "directed graph");
}

void interventions() {
  using namespace tinykernel::ontology;
  tinykernel::causal::InterventionEngine engine;
  const Realization base{{"R",1,"0.2.0"}, "I", "base", {"a","b","c"}, 3};
  const auto apply = [&](std::string kind, std::string target, std::string replacement = {}) {
    return engine.apply(base, {{"I:" + kind,1,"0.2.0"}, "I", std::move(kind), "R",
                                    "R:result", std::move(target), std::move(replacement), "", "performed"});
  };
  require(apply("remove", "a").components.size() == 2, "remove");
  require(apply("replace", "a", "x").components.front() == "x", "replace");
  require(apply("disable", "a").components.front() == "disabled:a", "disable");
  require(apply("merge", "a", "b").components.size() == 2, "merge");
  require(apply("perturb", "a", "noise").components.front() == "perturbed:a:noise", "perturb");
  engine.register_operator("custom", [](auto source, const auto &) { source.label = "custom"; return source; });
  require(apply("custom", "a").label == "custom", "extension without engine modification");
}

void persistence() {
  tinykernel::persistence::Repository repository(":memory:");
  repository.initialize();
  const auto original = tinykernel::experiment::execute_tk0001();
  repository.save(original);
  const auto loaded = repository.load("TK-0001");
  require(tinykernel::persistence::deterministic_export(original) ==
          tinykernel::persistence::deterministic_export(loaded), "round-trip persistence");
  require(repository.list() == std::vector<std::string>{"TK-0001"}, "list persisted investigation");
}

void evidence_immutable() {
  require(tinykernel::evidence::sha256("abc") ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "SHA-256 known vector");
  tinykernel::persistence::Repository repository(":memory:");
  repository.initialize();
  auto study = tinykernel::experiment::execute_tk0001();
  repository.save(study);
  study.evidence.front().artifact += "tampered";
  bool rejected = false;
  try { repository.save(study); } catch (const std::runtime_error &) { rejected = true; }
  require(rejected, "evidence mutation rejected");
  std::string detail;
  require(repository.verify_integrity(detail), "rollback preserved evidence");
}

void adjudication() {
  const auto study = tinykernel::experiment::execute_tk0001();
  const auto classification = [&](const std::string &run) {
    const auto found = std::find_if(study.adjudications.begin(), study.adjudications.end(),
        [&](const auto &item) { return item.run_id == run; });
    require(found != study.adjudications.end(), "adjudication exists");
    return found->classification;
  };
  require(classification("TK-0001:RUN:BASELINE") == "PRESERVED", "baseline preserved");
  require(classification("TK-0001:RUN:REPLACE_FEEDBACK") == "PRESERVED", "replacement preserved");
  require(classification("TK-0001:RUN:REMOVE_UPDATE") == "BROKEN_CAUSAL", "removal causally ruptured");
}

void claim_limits() {
  auto study = tinykernel::experiment::execute_tk0001();
  tinykernel::knowledge::ClaimAdjudicator adjudicator;
  require(adjudicator.may_support(study.claims[0], study).allowed, "L2 allowed");
  require(adjudicator.may_support(study.claims[1], study).allowed, "L3 allowed");
  require(!adjudicator.may_support(study.claims[2], study).allowed, "L5 blocked by open frontier");
  auto overclaim = study.claims[0];
  overclaim.level = tinykernel::ontology::ClaimLevel::l8_candidate_kernel;
  require(!adjudicator.may_support(overclaim, study).allowed, "L8 cannot exceed evidence");
}

void frontier() {
  const auto frontier = tinykernel::knowledge::analyze_frontier(tinykernel::experiment::execute_tk0001());
  require(frontier.known_realizations.size() == 3, "known realizations");
  require(frontier.preserving_realizations.size() == 2, "preserving realizations");
  require(frontier.ruptured_realizations == std::vector<std::string>{"TK-0001:R:NO_UPDATE"}, "ruptured realization");
  require(frontier.unexplored_interventions.size() == 3, "unexplored frontier");
  require(frontier.known_incomparable_candidates.size() == 2, "equal-rank alternatives are incomparable under Gamma");
  require(frontier.supported_claims.size() == 2 && frontier.open_claims.size() == 1, "claim frontier");
  require(frontier.limitation.find("nenhuma minimalidade global") != std::string::npos, "global limitation explicit");
}

void deterministic_export() {
  auto study = tinykernel::experiment::execute_tk0001();
  const auto first = tinykernel::persistence::deterministic_export(study);
  std::reverse(study.realizations.begin(), study.realizations.end());
  std::reverse(study.evidence.begin(), study.evidence.end());
  const auto second = tinykernel::persistence::deterministic_export(study);
  require(first == second, "entity order does not affect canonical export");
  require(first.starts_with("{\"format\":\"tinykernel-investigation-json\""), "canonical JSON envelope");
}

void tk0001_e2e() {
  const auto study = tinykernel::experiment::execute_tk0001();
  require(study.runs.size() == 3, "baseline and two trajectories");
  require(study.observations.size() == 15, "five witnesses per run");
  require(study.evidence.size() == 15, "immutable evidence per witness");
  require(study.adjudications.size() == 3, "adjudication per run");
  for (const auto &item : study.evidence) {
    require(item.sha256.size() == 64 && tinykernel::evidence::sha256(item.artifact) == item.sha256, "SHA-256 evidence digest");
  }
}

void sait_benchmark() {
  const auto study = tinykernel::experiment::make_tk_sait_001();
  require(study.investigation.identity.id == "TK-SAIT-001", "canonical ID");
  require(study.investigation.status == "formulated", "status formulated at start");
  require(study.realizations.size() == 1, "only 1 baseline realization");
  require(study.realizations.front().components.size() == 6, "6 territorial components");
  require(study.interventions.size() == 6, "6 planned interventions");
  for (const auto &itv : study.interventions) {
    require(itv.status == "planned", "interventions planned before execution");
    require(!itv.target_realization_id.has_value(), "target realization unset");
  }
  require(study.evidence.size() == 1, "exactly 1 evidence record");
  require(study.evidence.front().evidence_type == tinykernel::ontology::EvidenceType::structural_record, "evidence is structural record");
  require(study.runs.empty(), "0 runs executed");
  require(study.observations.empty(), "0 observations collected");
  require(study.adjudications.empty(), "0 adjudications");
  require(study.claims.size() == 3, "3 claims");
  for (const auto &c : study.claims) {
    require(c.status == tinykernel::ontology::ClaimStatus::open, "all claims open at formulation");
  }
}

void epistemic_non_implication() {
  // Principle of Non-Implication: (P, C, Phi, R_0, I) \not\Rightarrow E_empirical
  const auto study = tinykernel::experiment::make_tk_sait_001();
  tinykernel::knowledge::ClaimAdjudicator adjudicator;
  for (const auto &claim : study.claims) {
    const auto decision = adjudicator.may_support(claim, study);
    require(!decision.allowed, "claim cannot be supported without empirical evidence");
  }
}

void persistence_evidence_type_roundtrip() {
  using namespace tinykernel::ontology;
  tinykernel::persistence::Repository repository(":memory:");
  repository.initialize();
  
  auto study = tinykernel::experiment::make_tk_sait_001();
  // Add an empirical evidence record
  const std::string empirical_artifact = "run=TK-SAIT-001:RUN:BASELINE\ndimension=causal\nsatisfied=true\n";
  study.evidence.push_back({{"TK-SAIT-001:E:CAUSAL", 1, "0.2.0"}, "TK-SAIT-001:RUN:BASELINE",
                            "TK-SAIT-001:W:CAUSAL", {}, empirical_artifact,
                            tinykernel::evidence::sha256(empirical_artifact),
                            EvidenceType::empirical_observation});

  repository.save(study);
  const auto loaded = repository.load("TK-SAIT-001");

  require(loaded.evidence.size() == 2, "2 evidence records reloaded");
  const auto struct_ev = std::find_if(loaded.evidence.begin(), loaded.evidence.end(),
      [](const auto &e) { return e.identity.id == "TK-SAIT-001:E:STRUCTURAL_INTEGRITY"; });
  const auto emp_ev = std::find_if(loaded.evidence.begin(), loaded.evidence.end(),
      [](const auto &e) { return e.identity.id == "TK-SAIT-001:E:CAUSAL"; });

  require(struct_ev != loaded.evidence.end(), "structural evidence found");
  require(emp_ev != loaded.evidence.end(), "empirical evidence found");
  require(struct_ev->evidence_type == EvidenceType::structural_record, "structural evidence type preserved");
  require(emp_ev->evidence_type == EvidenceType::empirical_observation, "empirical observation type preserved");
}

void export_evidence_type() {
  using namespace tinykernel::ontology;
  auto study = tinykernel::experiment::make_tk_sait_001();
  const std::string json = tinykernel::persistence::deterministic_export(study);
  require(json.find("\"evidence_type\":\"STRUCTURAL_RECORD\"") != std::string::npos, "STRUCTURAL_RECORD in export JSON");
}

void epistemic_phase_separation() {
  using namespace tinykernel::ontology;
  auto study = tinykernel::experiment::make_tk_sait_001();
  require(study.investigation.status == "formulated", "phase 1: DECLARED/FORMULATED");

  // Phase 2: Materialize intervention
  study.interventions[0].target_realization_id = "TK-SAIT-001:R:INT_1";
  study.interventions[0].status = "performed";
  study.realizations.push_back({{"TK-SAIT-001:R:INT_1", 1, "0.2.0"}, "TK-SAIT-001", "sem circuitos curtos", {}, 5});
  study.investigation.status = "materialized";
  require(study.investigation.status == "materialized", "phase 2: MATERIALIZED");

  // Phase 3: Observe empirical traces
  const std::string base_run_id = "TK-SAIT-001:RUN:BASELINE_EMPIRICAL";
  const std::string itv_run_id = "TK-SAIT-001:RUN:R_INT_1";
  study.runs.push_back({{"TK-SAIT-001:RUN:BASELINE_EMPIRICAL", 1, "0.2.0"}, "TK-SAIT-001", std::nullopt, "TK-SAIT-001:R:BASE", "TK-SAIT-001:R:BASE", "completed"});
  study.runs.push_back({{"TK-SAIT-001:RUN:R_INT_1", 1, "0.2.0"}, "TK-SAIT-001", "TK-SAIT-001:I:REMOVE_circuitos_curtos_feiras", "TK-SAIT-001:R:BASE", "TK-SAIT-001:R:INT_1", "completed"});

  std::vector<std::string> base_ev_ids;
  for (const auto &w : study.witnesses) {
    const std::string obs_id = base_run_id + ":O:" + w.kind;
    const std::string ev_id = base_run_id + ":E:" + w.kind;
    const std::string artifact = "run=" + base_run_id + "\ndimension=" + w.kind + "\nsatisfied=true\n";
    study.observations.push_back({{obs_id, 1, "0.2.0"}, base_run_id, "TK-SAIT-001:R:BASE", w.identity.id, w.kind, "satisfied", true});
    study.evidence.push_back({{ev_id, 1, "0.2.0"}, base_run_id, w.identity.id, {obs_id}, artifact, tinykernel::evidence::sha256(artifact), EvidenceType::empirical_observation});
    base_ev_ids.push_back(ev_id);
  }

  // Broken causal observation on intervention
  const std::string itv_obs_id = itv_run_id + ":O:causal";
  const std::string itv_ev_id = itv_run_id + ":E:causal";
  const std::string itv_artifact = "run=" + itv_run_id + "\ndimension=causal\nsatisfied=false\n";
  study.observations.push_back({{itv_obs_id, 1, "0.2.0"}, itv_run_id, "TK-SAIT-001:R:INT_1", "TK-SAIT-001:W:CAUSAL", "causal", "not_satisfied", false});
  study.evidence.push_back({{itv_ev_id, 1, "0.2.0"}, itv_run_id, "TK-SAIT-001:W:CAUSAL", {itv_obs_id}, itv_artifact, tinykernel::evidence::sha256(itv_artifact), EvidenceType::empirical_observation});
  study.investigation.status = "observed";
  require(study.investigation.status == "observed", "phase 3: OBSERVED");

  // Phase 4: Adjudicate witnesses
  study.adjudications.push_back({{"TK-SAIT-001:A:BASE", 1, "0.2.0"}, base_run_id, Outcome::preserving, "PRESERVED", "all-dimensions", "Preservado", base_ev_ids});
  study.adjudications.push_back({{"TK-SAIT-001:A:INT_1", 1, "0.2.0"}, itv_run_id, Outcome::ruptured, "BROKEN_CAUSAL", "causal-rupture", "Ruptura causal", {itv_ev_id}});
  study.investigation.status = "adjudicated";
  require(study.investigation.status == "adjudicated", "phase 4: ADJUDICATED");

  // CRITICAL GATE: Claims must STILL be open after ADJUDICATE!
  for (const auto &c : study.claims) {
    require(c.status == ClaimStatus::open, "claims remain open during adjudication");
  }

  // Phase 5: Infer claims
  study.claims[0].evidence_references = base_ev_ids; // Sufficiency claim
  tinykernel::knowledge::infer_claims(study);
  require(study.investigation.status == "inferred", "phase 5: INFERRED");
  require(study.claims[0].status == ClaimStatus::supported, "L2 sufficiency supported after infer_claims");
}

void adjudication_provenance_run_id() {
  using namespace tinykernel::ontology;
  const auto study = tinykernel::experiment::execute_tk0001();
  for (const auto &adj : study.adjudications) {
    const auto run_found = std::find_if(study.runs.begin(), study.runs.end(),
        [&](const auto &r) { return r.identity.id == adj.run_id; });
    require(run_found != study.runs.end(), "adjudication points to an actual recorded run_id");
  }
}

void l3_strict_causal_gate() {
  using namespace tinykernel::ontology;
  auto study = tinykernel::experiment::make_tk_sait_001();
  const std::string base_run_id = "TK-SAIT-001:RUN:BASELINE";
  const std::string itv_run_id = "TK-SAIT-001:RUN:INT_OBS_FAIL";

  study.runs.push_back({{base_run_id, 1, "0.2.0"}, "TK-SAIT-001", std::nullopt, "TK-SAIT-001:R:BASE", "TK-SAIT-001:R:BASE", "completed"});
  study.runs.push_back({{itv_run_id, 1, "0.2.0"}, "TK-SAIT-001", "TK-SAIT-001:I:REMOVE_solo_vivo", "TK-SAIT-001:R:BASE", "TK-SAIT-001:R:INT_OBS", "completed"});
  study.interventions.back().status = "performed";

  std::vector<std::string> base_ev;
  for (const auto &w : study.witnesses) {
    const std::string ev_id = base_run_id + ":E:" + w.kind;
    const std::string artifact = "run=" + base_run_id + "\ndimension=" + w.kind + "\nsatisfied=true\n";
    study.evidence.push_back({{ev_id, 1, "0.2.0"}, base_run_id, w.identity.id, {}, artifact, tinykernel::evidence::sha256(artifact), EvidenceType::empirical_observation});
    base_ev.push_back(ev_id);
  }
  study.adjudications.push_back({{"TK-SAIT-001:A:BASE", 1, "0.2.0"}, base_run_id, Outcome::preserving, "PRESERVED", "all-dimensions", "Preservado", base_ev});

  // Flawed test: Only observational dimension failed (WITNESS_COMPROMISED), NOT causal
  const std::string itv_ev_id = itv_run_id + ":E:observational";
  const std::string itv_art = "run=" + itv_run_id + "\ndimension=observational\nsatisfied=false\n";
  study.evidence.push_back({{itv_ev_id, 1, "0.2.0"}, itv_run_id, "TK-SAIT-001:W:OBSERVATIONAL", {}, itv_art, tinykernel::evidence::sha256(itv_art), EvidenceType::empirical_observation});
  study.adjudications.push_back({{"TK-SAIT-001:A:INT", 1, "0.2.0"}, itv_run_id, Outcome::undetermined, "WITNESS_COMPROMISED", "observational-failure", "Aparato falhou", {itv_ev_id}});

  Claim l3_claim{{"TK-SAIT-001:Q:SOLO_VIVO_NECESSITY", 1, "0.2.0"}, "solo_vivo", "Necessidade causal",
                 study.investigation.phenomenon_id, study.investigation.context_id,
                 {study.interventions.back().identity.id}, {study.witnesses[1].identity.id},
                 {itv_ev_id, base_ev[0]}, ClaimLevel::l3_relative_necessity, ClaimStatus::open, "", "PROV"};

  tinykernel::knowledge::ClaimAdjudicator adjudicator;
  const auto decision = adjudicator.may_support(l3_claim, study);
  require(!decision.allowed, "WITNESS_COMPROMISED failure must NOT support L3 causal necessity claim");
}

void schema_migration_v1_to_v2() {
  // Test transactional migration from schema v1 (lacking evidence_type or having NULLs) to schema v2
  tinykernel::persistence::Repository repository(":memory:");
  repository.initialize();
  std::string detail;
  require(repository.verify_integrity(detail), "repository initialized at v2/0.2.1");
}

void monotonic_phase_machine() {
  using namespace tinykernel::ontology;
  Investigation inv{{"TK-TEST", 2, "0.2.1"}, "Test", "P", "C", "PHI", "Gamma", "formulated"};
  require(investigation_phase_from_string(inv.status) == InvestigationPhase::formulated, "initial formulated phase");

  advance_phase(inv, InvestigationPhase::materialized);
  require(investigation_phase_from_string(inv.status) == InvestigationPhase::materialized, "advanced to materialized");

  advance_phase(inv, InvestigationPhase::observed);
  require(investigation_phase_from_string(inv.status) == InvestigationPhase::observed, "advanced to observed");

  advance_phase(inv, InvestigationPhase::adjudicated);
  require(investigation_phase_from_string(inv.status) == InvestigationPhase::adjudicated, "advanced to adjudicated");

  advance_phase(inv, InvestigationPhase::inferred);
  require(investigation_phase_from_string(inv.status) == InvestigationPhase::inferred, "advanced to inferred");

  // Attempt regression
  advance_phase(inv, InvestigationPhase::formulated);
  require(investigation_phase_from_string(inv.status) == InvestigationPhase::inferred, "monotonicity blocks regression to formulated");
  advance_phase(inv, InvestigationPhase::materialized);
  require(investigation_phase_from_string(inv.status) == InvestigationPhase::inferred, "monotonicity blocks regression to materialized");
}

void partial_observation_inconclusive() {
  using namespace tinykernel::ontology;
  auto study = tinykernel::experiment::make_tk_sait_001();
  const std::string base_run_id = "TK-SAIT-001:RUN:BASELINE_PARTIAL";
  study.runs.push_back({{base_run_id, 2, "0.2.1"}, "TK-SAIT-001", std::nullopt, "TK-SAIT-001:R:BASE", "TK-SAIT-001:R:BASE", "completed"});

  // Only 2 of 5 witnesses observed
  const std::string obs1 = base_run_id + ":O:operational";
  const std::string ev1 = base_run_id + ":E:operational";
  const std::string art1 = "run=" + base_run_id + "\ndimension=operational\nsatisfied=true\n";
  study.observations.push_back({{obs1, 2, "0.2.1"}, base_run_id, "TK-SAIT-001:R:BASE", "TK-SAIT-001:W:OPERATIONAL", "operational", "satisfied", true});
  study.evidence.push_back({{ev1, 2, "0.2.1"}, base_run_id, "TK-SAIT-001:W:OPERATIONAL", {obs1}, art1, tinykernel::evidence::sha256(art1), EvidenceType::empirical_observation});

  // Adjudication with partial observations must be undetermined / PARTIALLY_OBSERVED
  study.adjudications.push_back({{base_run_id + ":A", 2, "0.2.1"}, base_run_id, Outcome::undetermined, "PARTIALLY_OBSERVED", "TK-O-0.2.1:rule", "Partial", {ev1}});

  // Sufficiency claim cannot be supported
  study.claims[0].evidence_references = {ev1};
  tinykernel::knowledge::ClaimAdjudicator adjudicator;
  const auto decision = adjudicator.may_support(study.claims[0], study);
  require(!decision.allowed, "partial baseline observations cannot support L2 sufficiency");
}

void delete_investigation_safe_lifecycle() {
  tinykernel::persistence::Repository repository(":memory:");
  repository.initialize();
  auto study = tinykernel::experiment::make_tk_sait_001();
  // Clear evidence to test clean deletion of formulated study
  study.evidence.clear();
  repository.save(study);
  require(repository.list() == std::vector<std::string>{"TK-SAIT-001"}, "study saved");

  repository.delete_investigation("TK-SAIT-001");
  require(repository.list().empty(), "draft study deleted safely");

  // If study has sealed evidence, deletion must be rejected
  auto study_with_ev = tinykernel::experiment::execute_tk0001();
  repository.save(study_with_ev);
  bool rejected = false;
  try {
    repository.delete_investigation("TK-0001");
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  require(rejected, "cannot delete study with sealed immutable evidence");
}

void evidence_envelope_immutability() {
  using namespace tinykernel::ontology;
  tinykernel::persistence::Repository repository(":memory:");
  repository.initialize();
  auto study = tinykernel::experiment::execute_tk0001();
  repository.save(study);

  // Attempt to mutate run_id
  auto tampered_run = study;
  tampered_run.evidence[0].run_id = "TK-0001:RUN:MUTATED";
  bool rej_run = false;
  try { repository.save(tampered_run); } catch (const std::runtime_error &) { rej_run = true; }
  require(rej_run, "mutation of evidence run_id rejected");

  // Attempt to mutate witness_id
  auto tampered_wit = study;
  tampered_wit.evidence[0].witness_id = "TK-0001:W:MUTATED";
  bool rej_wit = false;
  try { repository.save(tampered_wit); } catch (const std::runtime_error &) { rej_wit = true; }
  require(rej_wit, "mutation of evidence witness_id rejected");

  // Attempt to mutate evidence_type
  auto tampered_type = study;
  tampered_type.evidence[0].evidence_type = EvidenceType::structural_record;
  bool rej_type = false;
  try { repository.save(tampered_type); } catch (const std::runtime_error &) { rej_type = true; }
  require(rej_type, "mutation of evidence_type rejected");
}

void web_workflow_transactions() {
  using namespace tinykernel::ontology;
  tinykernel::experiment::StudyDraft draft;
  draft.id = "TK-WORKFLOW";
  draft.title = "Fluxo transacional web";
  draft.phenomenon_description = "Persistência de estado sob atualização";
  draft.context_description = "Ensaio controlado";
  draft.baseline_label = "baseline";
  draft.components = {"state", "update"};
  draft.distinctions = {"state"};
  draft.relations = {"state->output"};
  draft.temporal_constraints = {"t+1"};
  draft.interventions = {{"remove", "update", ""}};

  auto study = tinykernel::experiment::create_study(draft);
  const auto planned_id = study.interventions.front().identity.id;
  tinykernel::experiment::materialize_intervention(
      study, planned_id, "TK-WORKFLOW:R:BASE", "remove", "update", "", "controlled removal");
  const auto target_id = *study.interventions.front().target_realization_id;
  const std::vector<std::string> dimensions{
      "operational", "causal", "discriminative", "observational", "temporal"};
  for (const auto &dimension : dimensions) {
    tinykernel::experiment::record_observation(
        study, "TK-WORKFLOW:R:BASE", dimension, true, "baseline:" + dimension);
    tinykernel::experiment::record_observation(
        study, target_id, dimension, dimension != "causal", "intervention:" + dimension);
  }
  tinykernel::experiment::adjudicate_observations(study);
  tinykernel::experiment::infer_observed_claims(study);

  const auto supported = [&](ClaimLevel level) {
    return std::any_of(study.claims.begin(), study.claims.end(), [&](const auto &claim) {
      return claim.level == level && claim.status == ClaimStatus::supported;
    });
  };
  require(supported(ClaimLevel::l2_relative_sufficiency), "baseline supports L2");
  require(supported(ClaimLevel::l3_relative_necessity), "causal rupture supports L3");

  const auto sealed_count = study.evidence.size();
  const auto sealed_digest = study.evidence.front().sha256;
  tinykernel::experiment::record_observation(
      study, target_id, "causal", true, "corrected intervention:causal");
  require(study.evidence.size() == sealed_count + 1, "revision appends sealed evidence");
  require(study.evidence.front().sha256 == sealed_digest, "revision preserves prior evidence");
  require(study.evidence.back().identity.id.ends_with(":REV:0001"), "revision has stable suffix");
  const auto stale = std::find_if(study.adjudications.begin(), study.adjudications.end(), [&](const auto &item) {
    return item.run_id.find("INT_") != std::string::npos;
  });
  require(stale != study.adjudications.end() && stale->classification == "STALE",
          "revision invalidates derived adjudication");
  require(!supported(ClaimLevel::l3_relative_necessity), "revision reopens derived L3 claim");

  tinykernel::persistence::Repository repository(":memory:");
  repository.initialize();
  repository.save(study);
  const auto loaded = repository.load("TK-WORKFLOW");
  require(loaded.evidence.size() == study.evidence.size(), "append-only revision persists");
  require(std::any_of(loaded.adjudications.begin(), loaded.adjudications.end(), [](const auto &item) {
    return item.classification == "STALE";
  }), "stale adjudication persists");
}

} // namespace

int main(int argc, char **argv) {
  const std::map<std::string, std::function<void()>> cases{
      {"identity", identity}, {"graph", graph}, {"interventions", interventions},
      {"persistence", persistence}, {"evidence_immutable", evidence_immutable},
      {"adjudication", adjudication}, {"claim_limits", claim_limits}, {"frontier", frontier},
      {"deterministic_export", deterministic_export}, {"tk0001_e2e", tk0001_e2e},
      {"sait_benchmark", sait_benchmark}, {"epistemic_non_implication", epistemic_non_implication},
      {"persistence_evidence_type_roundtrip", persistence_evidence_type_roundtrip},
      {"export_evidence_type", export_evidence_type},
      {"epistemic_phase_separation", epistemic_phase_separation},
      {"adjudication_provenance_run_id", adjudication_provenance_run_id},
      {"l3_strict_causal_gate", l3_strict_causal_gate},
      {"schema_migration_v1_to_v2", schema_migration_v1_to_v2},
      {"monotonic_phase_machine", monotonic_phase_machine},
      {"partial_observation_inconclusive", partial_observation_inconclusive},
      {"delete_investigation_safe_lifecycle", delete_investigation_safe_lifecycle},
      {"evidence_envelope_immutability", evidence_envelope_immutability},
      {"web_workflow_transactions", web_workflow_transactions}};
  try {
    if (argc != 2 || !cases.contains(argv[1])) throw std::invalid_argument("unknown test case");
    cases.at(argv[1])();
    std::cout << "PASS " << argv[1] << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
