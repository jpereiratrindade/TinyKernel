#include "tinykernel/causal/engine.hpp"
#include "tinykernel/evidence/digest.hpp"
#include "tinykernel/experiment/laboratory.hpp"
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
  require(study.phenomenon.identity.schema_version == 1, "schema version");
  require(study.constitutive_profile.identity.ontology_version == "0.2.0", "ontology version");
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
  require(study.evidence.front().evidence_type == "STRUCTURAL_RECORD", "evidence is structural record");
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

} // namespace

int main(int argc, char **argv) {
  const std::map<std::string, std::function<void()>> cases{
      {"identity", identity}, {"graph", graph}, {"interventions", interventions},
      {"persistence", persistence}, {"evidence_immutable", evidence_immutable},
      {"adjudication", adjudication}, {"claim_limits", claim_limits}, {"frontier", frontier},
      {"deterministic_export", deterministic_export}, {"tk0001_e2e", tk0001_e2e},
      {"sait_benchmark", sait_benchmark}, {"epistemic_non_implication", epistemic_non_implication}};
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
