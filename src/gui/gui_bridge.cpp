#include "gui_bridge.hpp"

#include "tinykernel/experiment/laboratory.hpp"
#include "tinykernel/knowledge/analysis.hpp"
#include "tinykernel/persistence/repository.hpp"

#include <QVariantMap>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

QString joined(const std::vector<std::string> &values) {
  QStringList result;
  for (const auto &value : values) result.push_back(QString::fromStdString(value));
  return result.join("\n• ");
}

} // namespace

GuiBridge::GuiBridge(const QString &workspace, QObject *parent)
    : QObject(parent), workspace_(workspace) {
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
  status_message_ = study_.runs.empty()
      ? "TK-0001 preregistrado — execute a investigação para produzir evidência."
      : "TK-0001 carregado — evidência e claims disponíveis para inspeção.";
  selected_details_ = "Selecione uma realização, intervenção, run ou claim para inspecionar evidência e proveniência.";
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
  for (const auto &item : study_.realizations) {
    std::string outcome = "undetermined";
    if (std::find(frontier.preserving_realizations.begin(), frontier.preserving_realizations.end(), item.identity.id) != frontier.preserving_realizations.end()) outcome = "preserving";
    if (std::find(frontier.ruptured_realizations.begin(), frontier.ruptured_realizations.end(), item.identity.id) != frontier.ruptured_realizations.end()) outcome = "ruptured";
    QVariantMap value;
    value["id"] = QString::fromStdString(item.identity.id);
    const bool baseline = item.identity.id == "TK-0001:R:BASE";
    const bool alternative = item.identity.id == "TK-0001:R:ALT_FEEDBACK";
    value["label"] = baseline ? "baseline adaptativo"
        : alternative ? "feedback equivalente" : "sem atualização";
    value["outcome"] = QString::fromStdString(outcome);
    value["x"] = baseline ? 30 : 390;
    value["y"] = baseline ? 142 : (alternative ? 55 : 235);
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
    value["prediction"] = QString::fromStdString(item.prediction);
    value["x"] = 244;
    value["y"] = item.kind == "replace" ? 92 : 272;
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

QString GuiBridge::statusMessage() const { return status_message_; }
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
    return;
  }
  const auto run = std::find_if(study_.runs.begin(), study_.runs.end(), [&](const auto &item) { return item.identity.id == target; });
  if (run != study_.runs.end()) {
    const auto adjudication = std::find_if(study_.adjudications.begin(), study_.adjudications.end(),
        [&](const auto &item) { return item.run_id == target; });
    QStringList evidence;
    for (const auto &item : study_.evidence) if (item.run_id == target) {
      evidence.push_back(QString::fromStdString(item.witness_id + "\n  sha256: " + item.sha256));
    }
    selected_details_ = id + "\nResultado: " + QString::fromStdString(run->result_realization_id) +
        "\nAdjudicação: " + (adjudication == study_.adjudications.end()
            ? QString("pendente") : QString::fromStdString(adjudication->classification)) +
        "\n\nEVIDÊNCIA\n" + evidence.join("\n");
    emit selectedDetailsChanged();
    return;
  }
  const auto claim = std::find_if(study_.claims.begin(), study_.claims.end(), [&](const auto &item) { return item.identity.id == target; });
  if (claim != study_.claims.end()) {
    selected_details_ = id + "  [" + QString::fromStdString(tinykernel::ontology::to_string(claim->level)) + "]\n" +
        QString::fromStdString(claim->assertion) + "\nStatus: " +
        QString::fromStdString(tinykernel::ontology::to_string(claim->status)) + "\n\nLimite\n" +
        QString::fromStdString(claim->limitations) + "\n\nEvidências: " + QString::number(claim->evidence_references.size());
    emit selectedDetailsChanged();
  }
}

void GuiBridge::runInvestigation() {
  try {
    study_ = tinykernel::experiment::execute_tk0001();
    tinykernel::persistence::Repository repository(
        std::filesystem::path(workspace_.toStdString()) / "tinykernel.sqlite3");
    repository.initialize();
    repository.save(study_);
    selected_details_ = "Execução concluída. Selecione um run para inspecionar adjudicação e digests.";
    emit dataChanged();
    emit selectedDetailsChanged();
    setStatus("TK-0001 executado — 3 runs e 15 evidências imutáveis persistidos.");
  } catch (const std::exception &error) {
    setStatus("Falha ao executar: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::exportInvestigation() {
  try {
    const auto directory = std::filesystem::path(workspace_.toStdString());
    std::filesystem::create_directories(directory);
    const auto destination = directory / "TK-0001.json";
    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    output << tinykernel::persistence::deterministic_export(study_) << '\n';
    if (!output) throw std::runtime_error("não foi possível gravar o export");
    setStatus("Export JSON salvo em " + QString::fromStdString(destination.string()));
  } catch (const std::exception &error) {
    setStatus("Falha ao exportar: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::setStatus(QString message) {
  status_message_ = std::move(message);
  emit statusMessageChanged();
}
