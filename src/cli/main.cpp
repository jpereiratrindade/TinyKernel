#include "tinykernel/causal/engine.hpp"
#include "tinykernel/evidence/digest.hpp"
#include "tinykernel/experiment/laboratory.hpp"
#include "tinykernel/knowledge/analysis.hpp"
#include "tinykernel/persistence/repository.hpp"
#include "tinykernel/version.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#endif

namespace {

using tinykernel::ontology::ClaimStatus;

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
      << "commands: version, verify, init, list, show, run, experiment, frontier, claims, export, gui\n";
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

int launch_gui(const Arguments &arguments) {
#ifdef __linux__
  const auto self = std::filesystem::canonical("/proc/self/exe");
  const auto gui = self.parent_path() / "tinykernel-gui";
  const auto workspace = arguments.positional.size() > 1
      ? arguments.positional[1]
      : arguments.workspace.string();
  execl(gui.c_str(), gui.c_str(), "--workspace", workspace.c_str(), static_cast<char *>(nullptr));
  throw std::runtime_error("cannot launch tinykernel-gui at " + gui.string());
#else
  static_cast<void>(arguments);
  throw std::runtime_error("GUI launcher currently supports Linux");
#endif
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
    if (command == "gui") return launch_gui(arguments);

    if (command == "init") {
      if (arguments.positional.size() != 2) throw std::invalid_argument("init requires <workspace>");
      const auto workspace = std::filesystem::path(arguments.positional[1]);
      tinykernel::persistence::Repository repository(workspace / "tinykernel.sqlite3");
      repository.initialize();
      repository.save(tinykernel::experiment::make_tk0000());
      repository.save(tinykernel::experiment::make_tk0001());
      std::cout << "Initialized TinyKernel workspace: " << workspace << "\nInvestigations: TK-0000, TK-0001 (preregistered)\n";
      return 0;
    }

    tinykernel::persistence::Repository repository(database_path(arguments));
    repository.initialize();
    if (command == "list") {
      const auto ids = repository.list();
      if (arguments.json) {
        std::cout << '[';
        for (std::size_t i = 0; i < ids.size(); ++i) { if (i) std::cout << ','; std::cout << '"' << ids[i] << '"'; }
        std::cout << "]\n";
      } else for (const auto &id : ids) std::cout << id << '\n';
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
