#include "gui_bridge.hpp"

#include "tinykernel/experiment/laboratory.hpp"
#include "tinykernel/knowledge/analysis.hpp"
#include "tinykernel/persistence/repository.hpp"

#include <QVariantMap>

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace {

QString joined(const std::vector<std::string> &values) {
  QStringList result;
  for (const auto &value : values) result.push_back(QString::fromStdString(value));
  return result.join("\n• ");
}

} // namespace

GuiBridge::GuiBridge(const QString &workspace, QObject *parent) : QObject(parent) {
  const auto database = std::filesystem::path(workspace.toStdString()) / "tinykernel.sqlite3";
  if (std::filesystem::exists(database)) {
    tinykernel::persistence::Repository repository(database);
    repository.initialize();
    if (const auto investigations = repository.list();
        std::find(investigations.begin(), investigations.end(), "TK-0001") != investigations.end()) {
      study_ = repository.load("TK-0001");
    }
  }
  if (study_.investigation.identity.id.empty()) study_ = tinykernel::experiment::execute_tk0001();
  selected_details_ = "Selecione uma realização ou intervenção para inspecionar sua proveniência.";
}

QString GuiBridge::phenomenon() const { return QString::fromStdString(study_.phenomenon.name + "\n" + study_.phenomenon.definition); }
QString GuiBridge::context() const { return QString::fromStdString(study_.context.description); }
QString GuiBridge::profile() const {
  return "Distinções\n• " + joined(study_.constitutive_profile.distinctions) +
         "\n\nRelações\n• " + joined(study_.constitutive_profile.relations) +
         "\n\nTemporalidade\n• " + joined(study_.constitutive_profile.temporal_constraints);
}

QVariantList GuiBridge::realizations() const {
  const auto frontier = tinykernel::knowledge::analyze_frontier(study_);
  QVariantList values;
  for (std::size_t index = 0; index < study_.realizations.size(); ++index) {
    const auto &item = study_.realizations[index];
    std::string outcome = "undetermined";
    if (std::find(frontier.preserving_realizations.begin(), frontier.preserving_realizations.end(), item.identity.id) != frontier.preserving_realizations.end()) outcome = "preserving";
    if (std::find(frontier.ruptured_realizations.begin(), frontier.ruptured_realizations.end(), item.identity.id) != frontier.ruptured_realizations.end()) outcome = "ruptured";
    QVariantMap value;
    value["id"] = QString::fromStdString(item.identity.id);
    value["label"] = QString::fromStdString(item.label);
    value["outcome"] = QString::fromStdString(outcome);
    value["x"] = index == 0 ? 24 : 260;
    value["y"] = index == 0 ? 105 : (index == 1 ? 30 : 180);
    values.push_back(value);
  }
  return values;
}

QVariantList GuiBridge::interventions() const {
  QVariantList values;
  for (const auto &item : study_.interventions) {
    QVariantMap value;
    value["id"] = QString::fromStdString(item.identity.id);
    value["kind"] = QString::fromStdString(item.kind);
    value["source"] = QString::fromStdString(item.source_realization_id);
    value["target"] = QString::fromStdString(item.target_realization_id.value_or("frontier"));
    value["status"] = QString::fromStdString(item.status);
    values.push_back(value);
  }
  return values;
}

QVariantList GuiBridge::runs() const {
  QVariantList values;
  for (const auto &item : study_.runs) {
    const auto count = std::count_if(study_.evidence.begin(), study_.evidence.end(), [&](const auto &e) { return e.run_id == item.identity.id; });
    QVariantMap value;
    value["id"] = QString::fromStdString(item.identity.id);
    value["result"] = QString::fromStdString(item.result_realization_id);
    value["evidenceCount"] = static_cast<int>(count);
    values.push_back(value);
  }
  return values;
}

QVariantList GuiBridge::claims() const {
  QVariantList values;
  for (const auto &item : study_.claims) {
    QVariantMap value;
    value["id"] = QString::fromStdString(item.identity.id);
    value["level"] = QString::fromStdString(tinykernel::ontology::to_string(item.level));
    value["status"] = QString::fromStdString(tinykernel::ontology::to_string(item.status));
    value["assertion"] = QString::fromStdString(item.assertion);
    values.push_back(value);
  }
  return values;
}

QString GuiBridge::frontier() const {
  const auto value = tinykernel::knowledge::analyze_frontier(study_);
  return QString("Conhecidas: %1  •  Preservadoras: %2  •  Rompidas: %3\nCandidatas minimais atuais: %4  •  Intervenções abertas: %5\n%6")
      .arg(value.known_realizations.size()).arg(value.preserving_realizations.size())
      .arg(value.ruptured_realizations.size()).arg(value.currently_minimal_candidates.size())
      .arg(value.unexplored_interventions.size()).arg(QString::fromStdString(value.limitation));
}

QString GuiBridge::selectedDetails() const { return selected_details_; }

void GuiBridge::selectEntity(const QString &id) {
  const auto target = id.toStdString();
  const auto realization = std::find_if(study_.realizations.begin(), study_.realizations.end(), [&](const auto &item) { return item.identity.id == target; });
  if (realization != study_.realizations.end()) {
    selected_details_ = id + "\nSchema 1 • TK-O " + QString::fromStdString(realization->identity.ontology_version) +
        "\nComponentes\n• " + joined(realization->components) +
        "\n\nProveniência: " + QString::fromStdString(study_.provenance.front().detail);
    emit selectedDetailsChanged(); return;
  }
  const auto intervention = std::find_if(study_.interventions.begin(), study_.interventions.end(), [&](const auto &item) { return item.identity.id == target; });
  if (intervention != study_.interventions.end()) {
    selected_details_ = id + "\n" + QString::fromStdString(intervention->kind + " " + intervention->target) +
        "\nPredição: " + QString::fromStdString(intervention->prediction) +
        "\nStatus: " + QString::fromStdString(intervention->status) +
        "\nProveniência: preregistration / TK-O v0.2.0";
    emit selectedDetailsChanged();
  }
}
