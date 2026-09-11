#include "tinykernel/causal/engine.hpp"
#include "tinykernel/evidence/digest.hpp"
#include "tinykernel/experiment/laboratory.hpp"
#include "tinykernel/experiment/workflow.hpp"
#include "tinykernel/knowledge/analysis.hpp"
#include "tinykernel/persistence/repository.hpp"
#include "tinykernel/version.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using tinykernel::ontology::ClaimStatus;
using Form = std::map<std::string, std::vector<std::string>, std::less<>>;

struct Arguments {
  std::vector<std::string> positional;
  std::filesystem::path workspace{".tinykernel"};
  bool json{};
};

Arguments parse(int argc, char **argv) {
  Arguments result;
  for (int i = 1; i < argc; ++i) {
    const std::string value = argv[i];
    if (value == "--json") { result.json = true; continue; }
    if (value == "--workspace") {
      if (++i >= argc) throw std::invalid_argument("--workspace requires a path");
      result.workspace = argv[i];
      continue;
    }
    result.positional.push_back(value);
  }
  return result;
}

std::filesystem::path database_path(const Arguments &arguments) {
  return arguments.workspace / "tinykernel.sqlite3";
}

void usage() {
  std::cout << "TinyKernel " << tinykernel::version << "\n"
      << "usage: tinykernel [--workspace PATH] [--json] COMMAND [ARG]\n"
      << "commands: version, verify, init, integrity, list, show, run, experiment, create, materialize, observe, adjudicate, infer, delete, frontier, claims, export, project\n";
}

int hex_digit(const char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  return -1;
}

std::string url_decode(const std::string &value) {
  std::string result;
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '+') result.push_back(' ');
    else if (value[index] == '%' && index + 2 < value.size()) {
      const int high = hex_digit(value[index + 1]);
      const int low = hex_digit(value[index + 2]);
      if (high < 0 || low < 0) throw std::invalid_argument("invalid form encoding");
      result.push_back(static_cast<char>((high << 4) | low));
      index += 2;
    } else result.push_back(value[index]);
  }
  return result;
}

Form read_form() {
  std::ostringstream input;
  input << std::cin.rdbuf();
  Form form;
  const auto encoded = input.str();
  std::size_t begin = 0;
  while (begin <= encoded.size()) {
    const auto end = encoded.find('&', begin);
    const auto token = encoded.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    if (!token.empty()) {
      const auto separator = token.find('=');
      const auto key = url_decode(token.substr(0, separator));
      const auto value = separator == std::string::npos ? std::string{} : url_decode(token.substr(separator + 1));
      form[key].push_back(value);
    }
    if (end == std::string::npos) break;
    begin = end + 1;
  }
  return form;
}

std::string one(const Form &form, const std::string &key, const std::string &fallback = {}) {
  const auto found = form.find(key);
  return found == form.end() || found->second.empty() ? fallback : found->second.front();
}

std::vector<std::string> many(const Form &form, const std::string &key) {
  const auto found = form.find(key);
  return found == form.end() ? std::vector<std::string>{} : found->second;
}

void output_study(const tinykernel::ontology::Study &study) {
  std::cout << tinykernel::persistence::deterministic_export(study) << '\n';
}

tinykernel::ontology::Study execute_named(const std::string &id) {
  if (id == "TK-0000") return tinykernel::experiment::execute_tk0000();
  if (id == "TK-0001") return tinykernel::experiment::execute_tk0001();
  throw std::invalid_argument("no built-in adapter for investigation: " + id);
}

int verify() {
  using namespace tinykernel;
  bool ready = true;
  const auto report = [&](const std::string &gate, const bool passed, const std::string &detail) {
    std::cout << (passed ? "PASS " : "FAIL ") << gate << " — " << detail << '\n';
    ready = ready && passed;
  };

  report("versioning", ontology_version == "0.2.1" && schema_version == 2, "TK-O v0.2.1 / schema 2");
  causal::InterventionEngine engine;
  report("interventions", engine.supported_kinds() == std::vector<std::string>({"disable","merge","perturb","remove","replace"}),
         "remove, replace, disable, merge, perturb registered");
  const auto bootstrap = experiment::execute_tk0000();
  report("TK-0000", bootstrap.runs.size() == 2 && bootstrap.adjudications.size() == 2,
         "apparatus baseline and subtraction executed");
  const auto tk0001 = experiment::execute_tk0001();
  causal::CausalGraph graph(tk0001);
  report("causal-space", graph.valid() && graph.reachable("TK-0001:R:BASE", "TK-0001:R:ALT_FEEDBACK") &&
         graph.reachable("TK-0001:R:BASE", "TK-0001:R:NO_UPDATE"), "two distinct trajectories are connected");
  const auto frontier = knowledge::analyze_frontier(tk0001);
  report("frontier", !frontier.preserving_realizations.empty() && !frontier.ruptured_realizations.empty() &&
         !frontier.unexplored_interventions.empty(), "preserving, ruptured, and unexplored sets represented");
  const auto supported = std::count_if(tk0001.claims.begin(), tk0001.claims.end(),
      [](const auto &claim) { return claim.status == ClaimStatus::supported; });
  const auto open = std::count_if(tk0001.claims.begin(), tk0001.claims.end(),
      [](const auto &claim) { return claim.status == ClaimStatus::open; });
  report("claim-limits", supported == 2 && open == 1,
         "L2/L3 supported; L5 remains open; no global minimality asserted");
  persistence::Repository repository(":memory:");
  repository.initialize(); repository.save(bootstrap); repository.save(tk0001);
  std::string integrity;
  report("persistence", repository.verify_integrity(integrity), integrity);
  const auto loaded = repository.load("TK-0001");
  report("deterministic-export", persistence::deterministic_export(loaded) == persistence::deterministic_export(repository.load("TK-0001")),
         "repeated canonical JSON export is byte-identical");

  std::cout << '\n' << (ready ? "READY" : "NOT READY") << '\n';
  std::cout << "KNOWN INCOMPLETE / INCOMPLETE BY DESIGN\n";
  std::cout << "Open frontier: 3 planned interventions; no L4-L8 claim is supported.\n";
  return ready ? 0 : 1;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto arguments = parse(argc, argv);
    if (arguments.positional.empty()) { usage(); return 2; }
    const auto &command = arguments.positional[0];
    if (command == "version") {
      std::cout << "TinyKernel " << tinykernel::version << " (TK-O " << tinykernel::ontology_version << ")\n";
      return 0;
    }
    if (command == "verify") return verify();
    if (command == "init") {
      if (arguments.positional.size() != 2) throw std::invalid_argument("init requires <workspace>");
      const auto workspace = std::filesystem::path(arguments.positional[1]);
      tinykernel::persistence::Repository repository(workspace / "tinykernel.sqlite3");
      repository.initialize();
      repository.save(tinykernel::experiment::make_tk0000());
      repository.save(tinykernel::experiment::make_tk0001());
      repository.save(tinykernel::experiment::make_tk_sait_001());
      std::cout << "Initialized TinyKernel workspace: " << workspace
                << "\nInvestigations: TK-0000, TK-0001 (preregistered), TK-SAIT-001 (formulated)\n";
      return 0;
    }

    tinykernel::persistence::Repository repository(database_path(arguments));
    repository.initialize();
    if (command == "integrity") {
      std::string detail;
      if (!repository.verify_integrity(detail)) throw std::runtime_error(detail);
      for (const auto &study_id : repository.list()) static_cast<void>(repository.load(study_id));
      std::cout << "{\"status\":\"ready\",\"detail\":\"" << detail << "\"}\n";
      return 0;
    }
    if (command == "list") {
      const auto ids = repository.list();
      if (arguments.json) {
        std::cout << '[';
        for (std::size_t i = 0; i < ids.size(); ++i) { if (i) std::cout << ','; std::cout << '"' << ids[i] << '"'; }
        std::cout << "]\n";
      } else for (const auto &id : ids) std::cout << id << '\n';
      return 0;
    }
    if (command == "create") {
      const auto form = read_form();
      tinykernel::experiment::StudyDraft draft;
      draft.id = one(form, "id");
      draft.title = one(form, "title");
      draft.phenomenon_description = one(form, "phenomenon_description");
      draft.context_description = one(form, "context_description");
      draft.baseline_label = one(form, "baseline_label", "baseline completa");
      draft.components = many(form, "component");
      draft.distinctions = many(form, "distinction");
      draft.relations = many(form, "relation");
      draft.temporal_constraints = many(form, "temporal_constraint");
      const auto kinds = many(form, "intervention_kind");
      const auto targets = many(form, "intervention_target");
      const auto replacements = many(form, "intervention_replacement");
      if (kinds.size() != targets.size() || kinds.size() != replacements.size()) {
        throw std::invalid_argument("intervention fields have different lengths");
      }
      for (std::size_t index = 0; index < kinds.size(); ++index) {
        draft.interventions.push_back({kinds[index], targets[index], replacements[index]});
      }
      const auto existing = repository.list();
      if (std::find(existing.begin(), existing.end(), draft.id) != existing.end()) {
        throw std::invalid_argument("investigation already exists: " + draft.id);
      }
      const auto created = tinykernel::experiment::create_study(draft);
      repository.save(created);
      output_study(created);
      return 0;
    }
    if (arguments.positional.size() < 2) throw std::invalid_argument(command + " requires <investigation>");
    const auto &id = arguments.positional[1];
    if (command == "run" || command == "experiment") {
      const auto study = execute_named(id);
      repository.save(study);
      const auto frontier = tinykernel::knowledge::analyze_frontier(study);
      if (arguments.json) std::cout << tinykernel::knowledge::frontier_json(frontier) << '\n';
      else std::cout << id << " executed: " << study.runs.size() << " runs, " << study.evidence.size()
                     << " immutable evidence records, " << frontier.supported_claims.size() << " supported claims.\n";
      return 0;
    }
    if (command == "delete") {
      if (id == "TK-0000" || id == "TK-0001") throw std::invalid_argument("canonical investigation is read-only");
      if (!repository.delete_investigation(id)) throw std::invalid_argument("investigation not found: " + id);
      std::cout << "{\"deleted\":true}\n";
      return 0;
    }
    if (command == "materialize" || command == "observe" || command == "adjudicate" || command == "infer") {
      auto mutable_study = repository.load(id);
      if (command == "materialize") {
        const auto form = read_form();
        tinykernel::experiment::materialize_intervention(mutable_study, one(form, "planned_id"),
            one(form, "source_id", id + ":R:BASE"), one(form, "kind"), one(form, "target"),
            one(form, "replacement"), one(form, "protocol"));
      } else if (command == "observe") {
        const auto form = read_form();
        tinykernel::experiment::record_observation(mutable_study, one(form, "realization_id"),
            one(form, "dimension"), one(form, "satisfied") == "true", one(form, "trace"));
      } else if (command == "adjudicate") {
        tinykernel::experiment::adjudicate_observations(mutable_study);
      } else {
        tinykernel::experiment::infer_observed_claims(mutable_study);
      }
      repository.save(mutable_study);
      output_study(mutable_study);
      return 0;
    }
    const auto study = repository.load(id);
    if (command == "show") {
      if (arguments.json) std::cout << tinykernel::persistence::deterministic_export(study) << '\n';
      else std::cout << study.investigation.identity.id << " — " << study.investigation.title << '\n'
                     << "Phenomenon: " << study.phenomenon.name << '\n'
                     << "Status: " << study.investigation.status << '\n'
                     << "Realizations: " << study.realizations.size() << ", runs: " << study.runs.size()
                     << ", evidence: " << study.evidence.size() << '\n';
      return 0;
    }
    if (command == "project") {
      std::cout << tinykernel::experiment::workflow_projection_json(study) << '\n';
      return 0;
    }
    if (command == "frontier") {
      const auto frontier = tinykernel::knowledge::analyze_frontier(study);
      if (arguments.json) std::cout << tinykernel::knowledge::frontier_json(frontier) << '\n';
      else {
        const auto line = [](const std::string &label, const auto &values) {
          std::cout << label << " (" << values.size() << ")";
          for (const auto &value : values) std::cout << "\n  - " << value;
          std::cout << '\n';
        };
        line("Known realizations", frontier.known_realizations);
        line("Preserving", frontier.preserving_realizations);
        line("Ruptured", frontier.ruptured_realizations);
        line("Undetermined", frontier.undetermined_realizations);
        line("Currently minimal candidates", frontier.currently_minimal_candidates);
        line("Known incomparable candidates", frontier.known_incomparable_candidates);
        line("Unexplored/intervention frontier", frontier.unexplored_interventions);
        line("Supported claims", frontier.supported_claims);
        line("Open claims", frontier.open_claims);
        std::cout << "Limit: " << frontier.limitation << '\n';
      }
      return 0;
    }
    if (command == "claims") {
      if (arguments.json) {
        std::cout << "[";
        for (std::size_t i = 0; i < study.claims.size(); ++i) {
          const auto &claim = study.claims[i]; if (i) std::cout << ',';
          std::cout << "{\"id\":\"" << claim.identity.id << "\",\"level\":\""
                    << tinykernel::ontology::to_string(claim.level) << "\",\"status\":\""
                    << tinykernel::ontology::to_string(claim.status) << "\"}";
        }
        std::cout << "]\n";
      } else for (const auto &claim : study.claims) {
        std::cout << claim.identity.id << " [" << tinykernel::ontology::to_string(claim.level) << "] "
                  << tinykernel::ontology::to_string(claim.status) << "\n  " << claim.assertion
                  << "\n  Limit: " << claim.limitations << '\n';
      }
      return 0;
    }
    if (command == "export") {
      std::cout << tinykernel::persistence::deterministic_export(study) << '\n';
      return 0;
    }
    usage(); return 2;
  } catch (const std::exception &error) {
    std::cerr << "tinykernel: " << error.what() << '\n';
    return 1;
  }
}
