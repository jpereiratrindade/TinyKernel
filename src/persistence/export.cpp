#include "tinykernel/persistence/repository.hpp"

#include <algorithm>
#include <sstream>

namespace tinykernel::persistence {
namespace {

using namespace ontology;

std::string quote(const std::string &value) {
  std::ostringstream out;
  out << '"';
  for (const unsigned char character : value) {
    switch (character) {
    case '"': out << "\\\""; break;
    case '\\': out << "\\\\"; break;
    case '\b': out << "\\b"; break;
    case '\f': out << "\\f"; break;
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
}

void strings(std::ostringstream &out, const std::vector<std::string> &values) {
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << quote(values[i]); }
  out << ']';
}

void identity(std::ostringstream &out, const Identity &value) {
  out << "\"id\":" << quote(value.id) << ",\"schema_version\":" << value.schema_version
      << ",\"ontology_version\":" << quote(value.ontology_version);
}

template <typename T> std::vector<T> ordered(std::vector<T> values) {
  std::sort(values.begin(), values.end(), [](const auto &left, const auto &right) {
    return left.identity.id < right.identity.id;
  });
  return values;
}

template <typename T, typename Writer>
void objects(std::ostringstream &out, std::vector<T> values, Writer writer) {
  values = ordered(std::move(values));
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << '{'; writer(out, values[i]); out << '}'; }
  out << ']';
}

} // namespace

std::string deterministic_export(const Study &study) {
  std::ostringstream out;
  out << '{';
  out << "\"format\":\"tinykernel-investigation-json\",\"format_version\":1,";
  out << "\"ontology\":{\"id\":\"TK-O\",\"version\":\"0.2.1\"},";
  out << "\"investigation\":{"; identity(out, study.investigation.identity);
  out << ",\"title\":" << quote(study.investigation.title)
      << ",\"phenomenon_id\":" << quote(study.investigation.phenomenon_id)
      << ",\"context_id\":" << quote(study.investigation.context_id)
      << ",\"constitutive_profile_id\":" << quote(study.investigation.constitutive_profile_id)
      << ",\"reduction_order\":" << quote(study.investigation.reduction_order)
      << ",\"status\":" << quote(study.investigation.status) << "},";
  out << "\"phenomenon\":{"; identity(out, study.phenomenon.identity);
  out << ",\"name\":" << quote(study.phenomenon.name) << ",\"definition\":" << quote(study.phenomenon.definition) << "},";
  out << "\"context\":{"; identity(out, study.context.identity);
  out << ",\"description\":" << quote(study.context.description) << "},";
  out << "\"constitutive_profile\":{"; identity(out, study.constitutive_profile.identity);
  out << ",\"distinctions\":"; strings(out, study.constitutive_profile.distinctions);
  out << ",\"relations\":"; strings(out, study.constitutive_profile.relations);
  out << ",\"temporal_constraints\":"; strings(out, study.constitutive_profile.temporal_constraints); out << "},";

  out << "\"realizations\":"; objects(out, study.realizations, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"investigation_id\":" << quote(item.investigation_id)
      << ",\"label\":" << quote(item.label) << ",\"components\":"; strings(o, item.components);
    o << ",\"reduction_rank\":" << item.reduction_rank;
  });
  out << ",\"structures\":"; objects(out, study.structures, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"realization_id\":" << quote(item.realization_id) << ",\"parts\":";
    strings(o, item.parts); o << ",\"relations\":"; strings(o, item.relations);
  });
  out << ",\"interventions\":"; objects(out, study.interventions, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"kind\":" << quote(item.kind)
      << ",\"source_realization_id\":" << quote(item.source_realization_id) << ",\"target_realization_id\":";
    if (item.target_realization_id) o << quote(*item.target_realization_id); else o << "null";
    o << ",\"target\":" << quote(item.target) << ",\"replacement\":" << quote(item.replacement)
      << ",\"prediction\":" << quote(item.prediction) << ",\"status\":" << quote(item.status);
  });
  out << ",\"runs\":"; objects(out, study.runs, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"investigation_id\":" << quote(item.investigation_id) << ",\"intervention_id\":";
    if (item.intervention_id) o << quote(*item.intervention_id); else o << "null";
    o << ",\"source_realization_id\":" << quote(item.source_realization_id)
      << ",\"result_realization_id\":" << quote(item.result_realization_id) << ",\"status\":" << quote(item.status);
  });
  out << ",\"observations\":"; objects(out, study.observations, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"run_id\":" << quote(item.run_id)
      << ",\"realization_id\":" << quote(item.realization_id) << ",\"witness_id\":" << quote(item.witness_id)
      << ",\"dimension\":" << quote(item.dimension) << ",\"value\":" << quote(item.value)
      << ",\"satisfied\":" << (item.satisfied ? "true" : "false");
  });
  out << ",\"witnesses\":"; objects(out, study.witnesses, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"kind\":" << quote(item.kind) << ",\"description\":" << quote(item.description);
  });
  out << ",\"evidence\":"; objects(out, study.evidence, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"run_id\":" << quote(item.run_id) << ",\"witness_id\":" << quote(item.witness_id)
      << ",\"observation_ids\":"; strings(o, item.observation_ids);
    o << ",\"artifact\":" << quote(item.artifact) << ",\"sha256\":" << quote(item.sha256)
      << ",\"evidence_type\":" << quote(to_string(item.evidence_type));
  });
  out << ",\"adjudications\":"; objects(out, study.adjudications, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"run_id\":" << quote(item.run_id) << ",\"outcome\":" << quote(to_string(item.outcome))
      << ",\"classification\":" << quote(item.classification) << ",\"rule\":" << quote(item.rule)
      << ",\"rationale\":" << quote(item.rationale) << ",\"evidence_references\":"; strings(o, item.evidence_references);
  });
  out << ",\"claims\":"; objects(out, study.claims, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"subject\":" << quote(item.subject) << ",\"assertion\":" << quote(item.assertion)
      << ",\"phenomenon_id\":" << quote(item.phenomenon_id) << ",\"context_id\":" << quote(item.context_id)
      << ",\"intervention_scope\":"; strings(o, item.intervention_scope);
    o << ",\"witness_scope\":"; strings(o, item.witness_scope);
    o << ",\"evidence_references\":"; strings(o, item.evidence_references);
    o << ",\"level\":" << quote(to_string(item.level)) << ",\"status\":" << quote(to_string(item.status))
      << ",\"limitations\":" << quote(item.limitations) << ",\"provenance_id\":" << quote(item.provenance_id);
  });
  out << ",\"provenance\":"; objects(out, study.provenance, [](auto &o, const auto &item) {
    identity(o, item.identity); o << ",\"source\":" << quote(item.source) << ",\"method\":" << quote(item.method)
      << ",\"timestamp\":" << quote(item.timestamp) << ",\"detail\":" << quote(item.detail);
  });
  out << '}';
  return out.str();
}

} // namespace tinykernel::persistence
