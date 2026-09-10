#include "gui_bridge.hpp"

#include "tinykernel/causal/engine.hpp"
#include "tinykernel/evidence/digest.hpp"
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

tinykernel::ontology::Identity make_id(std::string value) {
  return tinykernel::ontology::Identity{std::move(value), 1, "0.2.0"};
}

tinykernel::ontology::Study build_tk_sait_001() {
  return tinykernel::experiment::make_tk_sait_001();
}

} // namespace

GuiBridge::GuiBridge(const QString &workspace, QObject *parent)
    : QObject(parent), workspace_(workspace) {
  refreshStudies();
  status_message_ = "Laboratório TinyKernel pronto — selecione ou crie uma investigação.";
  selected_details_ = "Selecione uma realização, intervenção, run ou claim para inspecionar evidência e proveniência.";
}

void GuiBridge::refreshStudies() {
  all_studies_.clear();
  all_studies_.push_back(tinykernel::experiment::execute_tk0000());
  all_studies_.push_back(tinykernel::experiment::execute_tk0001());
  all_studies_.push_back(build_tk_sait_001());

  const auto database = std::filesystem::path(workspace_.toStdString()) / "tinykernel.sqlite3";
  if (std::filesystem::exists(database)) {
    try {
      tinykernel::persistence::Repository repository(database);
      repository.initialize();
      const auto list = repository.list();
      for (const auto &id : list) {
        if (id != "TK-0000" && id != "TK-0001" && id != "TK-SAIT-001") {
          all_studies_.push_back(repository.load(id));
        }
      }
    } catch (...) {}
  }

  loadStudy(active_id_);
  emit investigationsChanged();
  emit globalStatsChanged();
}

void GuiBridge::loadStudy(const std::string &id) {
  active_id_ = id;
  const auto it = std::find_if(all_studies_.begin(), all_studies_.end(),
                               [&](const auto &s) { return s.investigation.identity.id == id; });
  if (it != all_studies_.end()) {
    study_ = *it;
  } else {
    study_ = (id == "TK-0000") ? tinykernel::experiment::execute_tk0000()
           : (id == "TK-SAIT-001") ? build_tk_sait_001()
           : tinykernel::experiment::execute_tk0001();
  }
  emit activeInvestigationIdChanged();
  emit dataChanged();
}

QString GuiBridge::currentView() const { return current_view_; }
QString GuiBridge::activeInvestigationId() const { return QString::fromStdString(active_id_); }

QVariantList GuiBridge::investigations() const {
  QVariantList list;
  for (const auto &s : all_studies_) {
    QVariantMap item;
    item["id"] = QString::fromStdString(s.investigation.identity.id);
    item["title"] = QString::fromStdString(s.investigation.title);
    item["phenomenonName"] = QString::fromStdString(s.phenomenon.name);
    item["phenomenonDesc"] = QString::fromStdString(s.phenomenon.definition);
    item["realizationsCount"] = static_cast<int>(s.realizations.size());
    item["interventionsCount"] = static_cast<int>(s.interventions.size());
    int empiricalCount = 0;
    int structuralCount = 0;
    for (const auto &e : s.evidence) {
      if (e.evidence_type == tinykernel::ontology::EvidenceType::empirical_observation ||
          e.evidence_type == tinykernel::ontology::EvidenceType::witness_adjudication) empiricalCount++;
      else structuralCount++;
    }
    item["evidenceCount"] = static_cast<int>(s.evidence.size());
    item["empiricalEvidenceCount"] = empiricalCount;
    item["structuralEvidenceCount"] = structuralCount;
    
    int supportedCount = 0;
    for (const auto &c : s.claims) {
      if (c.status == tinykernel::ontology::ClaimStatus::supported) supportedCount++;
    }
    item["supportedClaimsCount"] = supportedCount;
    item["totalClaimsCount"] = static_cast<int>(s.claims.size());
    item["isCanonical"] = (s.investigation.identity.id == "TK-0001");
    item["isSanity"] = (s.investigation.identity.id == "TK-0000");
    item["isBenchmark"] = (s.investigation.identity.id == "TK-SAIT-001");
    item["status"] = QString::fromStdString(s.investigation.status.empty() ? "executed" : s.investigation.status);

    list.push_back(item);
  }
  return list;
}

QVariantMap GuiBridge::globalStats() const {
  QVariantMap stats;
  int totalEvidence = 0;
  int totalEmpiricalEvidence = 0;
  int totalStructuralEvidence = 0;
  int totalClaimsSupported = 0;
  for (const auto &s : all_studies_) {
    totalEvidence += static_cast<int>(s.evidence.size());
    for (const auto &e : s.evidence) {
      if (e.evidence_type == tinykernel::ontology::EvidenceType::empirical_observation ||
          e.evidence_type == tinykernel::ontology::EvidenceType::witness_adjudication) totalEmpiricalEvidence++;
      else totalStructuralEvidence++;
    }
    for (const auto &c : s.claims) {
      if (c.status == tinykernel::ontology::ClaimStatus::supported) totalClaimsSupported++;
    }
  }
  stats["totalInvestigations"] = static_cast<int>(all_studies_.size());
  stats["totalEvidence"] = totalEvidence;
  stats["totalEmpiricalEvidence"] = totalEmpiricalEvidence;
  stats["totalStructuralEvidence"] = totalStructuralEvidence;
  stats["totalClaimsSupported"] = totalClaimsSupported;
  return stats;
}

QString GuiBridge::phenomenon() const {
  return QString::fromStdString(study_.phenomenon.name + "\n" + study_.phenomenon.definition);
}
QString GuiBridge::context() const { return QString::fromStdString(study_.context.description); }
QString GuiBridge::profile() const {
  return "Distinções\n• " + joined(study_.constitutive_profile.distinctions) +
         "\n\nRelações\n• " + joined(study_.constitutive_profile.relations) +
         "\n\nTemporalidade\n• " + joined(study_.constitutive_profile.temporal_constraints);
}

QVariantList GuiBridge::realizations() const {
  const auto frontier = tinykernel::knowledge::analyze_frontier(study_);
  QVariantList values;
  int index = 0;
  for (const auto &item : study_.realizations) {
    std::string outcome = "undetermined";
    if (std::find(frontier.preserving_realizations.begin(), frontier.preserving_realizations.end(), item.identity.id) != frontier.preserving_realizations.end()) outcome = "preserving";
    if (std::find(frontier.ruptured_realizations.begin(), frontier.ruptured_realizations.end(), item.identity.id) != frontier.ruptured_realizations.end()) outcome = "ruptured";
    
    QVariantMap value;
    value["id"] = QString::fromStdString(item.identity.id);
    value["label"] = QString::fromStdString(item.label);
    value["outcome"] = QString::fromStdString(outcome);
    value["componentsCount"] = static_cast<int>(item.components.size());
    
    // Position distribution
    const bool isBase = (index == 0);
    value["x"] = isBase ? 30 : 380;
    value["y"] = isBase ? 140 : (40 + (index - 1) * 95);
    values.push_back(value);
    index++;
  }
  return values;
}

QVariantList GuiBridge::interventions() const {
  QVariantList values;
  int index = 0;
  for (const auto &item : study_.interventions) {
    QVariantMap value;
    value["id"] = QString::fromStdString(item.identity.id);
    value["kind"] = QString::fromStdString(item.kind);
    value["source"] = QString::fromStdString(item.source_realization_id);
    value["target"] = QString::fromStdString(item.target_realization_id.value_or("frontier"));
    value["status"] = QString::fromStdString(item.status);
    value["prediction"] = QString::fromStdString(item.prediction);
    value["x"] = 230;
    value["y"] = 65 + index * 95;
    values.push_back(value);
    index++;
  }
  return values;
}

QVariantList GuiBridge::runs() const {
  QVariantList values;
  for (const auto &item : study_.runs) {
    const auto count = std::count_if(study_.evidence.begin(), study_.evidence.end(),
                                     [&](const auto &e) { return e.run_id == item.identity.id; });
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

void GuiBridge::openInvestigation(const QString &id) {
  loadStudy(id.toStdString());
  current_view_ = "workbench";
  emit currentViewChanged();
  setStatus(QString("Investigação %1 aberta no workbench.").arg(id));
}

void GuiBridge::backToHome() {
  current_view_ = "home";
  emit currentViewChanged();
  refreshStudies();
  setStatus("Catálogo de investigações do Laboratório TinyKernel.");
}

void GuiBridge::openWizard() {
  current_view_ = "wizard";
  emit currentViewChanged();
  setStatus("Assistente de formulação de nova investigação causal.");
}

void GuiBridge::selectEntity(const QString &id) {
  const auto target = id.toStdString();
  const auto realization = std::find_if(study_.realizations.begin(), study_.realizations.end(),
                                        [&](const auto &item) { return item.identity.id == target; });
  if (realization != study_.realizations.end()) {
    selected_details_ = id + "\nSchema 1 • TK-O " + QString::fromStdString(realization->identity.ontology_version) +
        "\nComponentes\n• " + joined(realization->components) +
        "\n\nProveniência: " + QString::fromStdString(study_.provenance.empty() ? "preregistration" : study_.provenance.front().detail);
    emit selectedDetailsChanged();
    return;
  }

  const auto intervention = std::find_if(study_.interventions.begin(), study_.interventions.end(),
                                         [&](const auto &item) { return item.identity.id == target; });
  if (intervention != study_.interventions.end()) {
    selected_details_ = id + "\n" + QString::fromStdString(intervention->kind + " " + intervention->target) +
        "\nPredição: " + QString::fromStdString(intervention->prediction) +
        "\nStatus: " + QString::fromStdString(intervention->status) +
        "\nProveniência: preregistration / TK-O v0.2.0";
    emit selectedDetailsChanged();
    return;
  }

  const auto run = std::find_if(study_.runs.begin(), study_.runs.end(),
                                [&](const auto &item) { return item.identity.id == target; });
  if (run != study_.runs.end()) {
    const auto adjudication = std::find_if(study_.adjudications.begin(), study_.adjudications.end(),
                                           [&](const auto &item) { return item.run_id == target; });
    QStringList evidence;
    for (const auto &item : study_.evidence) {
      if (item.run_id == target) {
        evidence.push_back(QString::fromStdString(item.witness_id + "\n  sha256: " + item.sha256));
      }
    }
    selected_details_ = id + "\nResultado: " + QString::fromStdString(run->result_realization_id) +
        "\nAdjudicação: " + (adjudication == study_.adjudications.end()
            ? QString("pendente") : QString::fromStdString(adjudication->classification)) +
        "\n\nEVIDÊNCIA\n" + evidence.join("\n");
    emit selectedDetailsChanged();
    return;
  }

  const auto claim = std::find_if(study_.claims.begin(), study_.claims.end(),
                                  [&](const auto &item) { return item.identity.id == target; });
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
    if (active_id_ == "TK-0000") {
      study_ = tinykernel::experiment::execute_tk0000();
    } else if (active_id_ == "TK-0001") {
      study_ = tinykernel::experiment::execute_tk0001();
    }
    saveStudyToRepository(study_);
    selected_details_ = "Execução concluída. Selecione um run para inspecionar evidências SHA-256.";
    emit dataChanged();
    emit selectedDetailsChanged();
    setStatus(QString("%1 executado com sucesso.").arg(QString::fromStdString(active_id_)));
  } catch (const std::exception &error) {
    setStatus("Falha ao executar: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::saveStudyToRepository(const tinykernel::ontology::Study &study) {
  try {
    tinykernel::persistence::Repository repository(
        std::filesystem::path(workspace_.toStdString()) / "tinykernel.sqlite3");
    repository.initialize();
    repository.save(study);
  } catch (...) {}
}

void GuiBridge::exportCurrent() {
  exportInvestigation();
}

void GuiBridge::exportInvestigation() {
  try {
    const auto directory = std::filesystem::path(workspace_.toStdString());
    std::filesystem::create_directories(directory);
    const auto destination = directory / (active_id_ + ".json");
    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    output << tinykernel::persistence::deterministic_export(study_) << '\n';
    if (!output) throw std::runtime_error("não foi possível gravar o export");
    setStatus("Export JSON salvo em " + QString::fromStdString(destination.string()));
  } catch (const std::exception &error) {
    setStatus("Falha ao exportar: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::createInvestigation(const QVariantMap &config) {
  try {
    const std::string id = config.value("id", "TK-0002").toString().toStdString();
    const std::string name = config.value("phenomenonName", QString::fromStdString(id)).toString().toStdString();
    const std::string desc = config.value("phenomenonDesc", "").toString().toStdString();
    const std::string context = config.value("contextDesc", "Execução local determinística.").toString().toStdString();
    const std::string baselineLabel = config.value("baselineLabel", "baseline completa").toString().toStdString();
    
    const auto compList = config.value("baselineComponents").toStringList();
    std::vector<std::string> comps;
    for (const auto &c : compList) comps.push_back(c.toStdString());
    if (comps.empty()) comps = {"sensor", "filter", "integrator", "actuator"};

    tinykernel::ontology::Study newStudy;
    newStudy.investigation = {make_id(id), name, id + ":P", id + ":C", id + ":PHI",
                              "Gamma=active_causal_relations", "formulated"};
    newStudy.phenomenon = {make_id(id + ":P"), name, desc};
    newStudy.context = {make_id(id + ":C"), context};
    newStudy.constitutive_profile = {make_id(id + ":PHI"),
                                     {"estado observável"},
                                     {"componente->resultado"},
                                     {"resposta temporal"}};

    newStudy.witnesses = {
        {make_id(id + ":W:OPERATIONAL"), "operational", "O sistema produz comportamento observável."},
        {make_id(id + ":W:CAUSAL"), "causal", "A saída depende da relação causal preregistrada."},
        {make_id(id + ":W:DISCRIMINATIVE"), "discriminative", "As distinções do perfil permanecem separáveis."},
        {make_id(id + ":W:OBSERVATIONAL"), "observational", "O aparato permanece capaz de observar."},
        {make_id(id + ":W:TEMPORAL"), "temporal", "A restrição temporal preregistrada é satisfeita."}
    };

    newStudy.realizations.push_back({make_id(id + ":R:BASE"), id, baselineLabel, comps, static_cast<std::uint32_t>(comps.size())});
    newStudy.structures.push_back({make_id(id + ":R:BASE:SIGMA"), id + ":R:BASE", comps, comps});
    newStudy.provenance.push_back({make_id(id + ":PROV"), "interactive_wizard",
                                   "deterministic adapter", "2026-09-10T00:00:00-03:00",
                                   "Investigação criada interativamente pelo usuário."});

    // 1 structural integrity evidence record
    const std::string artifact = "investigation=" + id + "\nstatus=formulated\nbaseline=untested\ncomponents=" + std::to_string(comps.size()) + "\n";
    newStudy.evidence.push_back({make_id(id + ":E:STRUCTURAL_INTEGRITY"), id + ":RUN:SPECIFICATION",
                                 id + ":W:OBSERVATIONAL", {}, artifact, tinykernel::evidence::sha256(artifact),
                                 tinykernel::ontology::EvidenceType::structural_record});

    // Claims are OPEN until empirical observation is collected
    newStudy.claims.push_back({make_id(id + ":Q:SUFFICIENCY"), id + ":R:BASE",
                               "A realização baseline é suficiente sob o protocolo preregistrado.",
                               id + ":P", id + ":C", {},
                               {id + ":W:OPERATIONAL", id + ":W:CAUSAL", id + ":W:DISCRIMINATIVE", id + ":W:OBSERVATIONAL", id + ":W:TEMPORAL"},
                               {}, tinykernel::ontology::ClaimLevel::l2_relative_sufficiency,
                               tinykernel::ontology::ClaimStatus::open,
                               "Aguardando testes empíricos da baseline.", id + ":PROV"});

    newStudy.claims.push_back({make_id(id + ":Q:RELATIVE_MINIMALITY"), id + ":R:BASE",
                               "A realização é minimal na ordem Gamma declarada.",
                               id + ":P", id + ":C", {}, {}, {},
                               tinykernel::ontology::ClaimLevel::l5_relative_minimality,
                               tinykernel::ontology::ClaimStatus::open,
                               "Espaço incompleto: reduções planejadas permanecem abertas.", id + ":PROV"});

    saveStudyToRepository(newStudy);
    refreshStudies();
    openInvestigation(QString::fromStdString(id));
  } catch (const std::exception &error) {
    setStatus("Falha ao criar investigação: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::injectObservation(const QString &realizationId, const QString &dimension,
                                 bool satisfied, const QString &trace) {
  try {
    const std::string studyId = active_id_;
    const std::string rId = realizationId.isEmpty() ? study_.realizations[0].identity.id : realizationId.toStdString();
    const std::string dim = dimension.isEmpty() ? "causal" : dimension.toStdString();
    const std::string rawTrace = trace.toStdString();

    const std::string runId = studyId + ":RUN:" + (rId == (studyId + ":R:BASE") ? "BASELINE_EMPIRICAL" : rId);
    
    // Add or retrieve run
    auto runIt = std::find_if(study_.runs.begin(), study_.runs.end(), [&](const auto &r) { return r.identity.id == runId; });
    if (runIt == study_.runs.end()) {
      study_.runs.push_back({make_id(runId), studyId, std::nullopt, studyId + ":R:BASE", rId, "in_progress"});
    }

    // Find specific target witness
    auto witnessIt = std::find_if(study_.witnesses.begin(), study_.witnesses.end(), [&](const auto &w) { return w.kind == dim; });
    const std::string witnessId = (witnessIt != study_.witnesses.end()) ? witnessIt->identity.id : (studyId + ":W:" + dim);

    const std::string obsId = runId + ":O:" + dim;
    // Replace or insert observation for this dimension
    auto obsIt = std::find_if(study_.observations.begin(), study_.observations.end(), [&](const auto &o) { return o.identity.id == obsId; });
    if (obsIt != study_.observations.end()) {
      obsIt->satisfied = satisfied;
      obsIt->value = satisfied ? "satisfied" : "not_satisfied";
    } else {
      study_.observations.push_back({make_id(obsId), runId, rId, witnessId, dim,
                                     satisfied ? "satisfied" : "not_satisfied", satisfied});
    }

    const std::string artifact = "run=" + runId + "\nrealization=" + rId + "\ndimension=" + dim +
        "\nsatisfied=" + (satisfied ? "true" : "false") + "\nempirical_trace=" + (rawTrace.empty() ? "measured=true" : rawTrace) + "\n";
    const std::string evId = runId + ":E:" + dim;

    // Replace or insert empirical evidence
    auto evIt = std::find_if(study_.evidence.begin(), study_.evidence.end(), [&](const auto &e) { return e.identity.id == evId; });
    if (evIt != study_.evidence.end()) {
      evIt->artifact = artifact;
      evIt->sha256 = tinykernel::evidence::sha256(artifact);
      evIt->evidence_type = tinykernel::ontology::EvidenceType::empirical_observation;
    } else {
      study_.evidence.push_back({make_id(evId), runId, witnessId, {obsId}, artifact,
                                 tinykernel::evidence::sha256(artifact),
                                 tinykernel::ontology::EvidenceType::empirical_observation});
    }

    study_.investigation.status = "observed";
    saveStudyToRepository(study_);
    emit dataChanged();
    setStatus(QString("Observação empírica para dimensão '%1' registrada com sucesso.").arg(QString::fromStdString(dim)));
  } catch (const std::exception &error) {
    setStatus("Falha ao injetar observação: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::adjudicateWitnesses() {
  try {
    const std::string studyId = active_id_;
    int adjudicationsProduced = 0;

    // 1. Evaluate baseline empirical observations
    const std::string baseRealizationId = studyId + ":R:BASE";
    std::vector<std::string> baseWitnessKinds;
    for (const auto &w : study_.witnesses) baseWitnessKinds.push_back(w.kind);

    int satisfiedCount = 0;
    std::vector<std::string> satisfiedEvIds;
    for (const auto &dim : baseWitnessKinds) {
      const auto obs = std::find_if(study_.observations.begin(), study_.observations.end(),
                                    [&](const auto &o) { return o.realization_id == baseRealizationId && o.dimension == dim; });
      if (obs != study_.observations.end() && obs->satisfied) {
        satisfiedCount++;
        satisfiedEvIds.push_back(studyId + ":RUN:BASELINE_EMPIRICAL:E:" + dim);
      }
    }

    const bool baselinePreserved = (satisfiedCount == static_cast<int>(study_.witnesses.size()) && !study_.witnesses.empty());
    if (baselinePreserved) {
      const std::string baseRunId = studyId + ":RUN:BASELINE_EMPIRICAL";
      auto adjIt = std::find_if(study_.adjudications.begin(), study_.adjudications.end(), [&](const auto &a) { return a.run_id == baseRunId; });
      if (adjIt != study_.adjudications.end()) {
        adjIt->outcome = tinykernel::ontology::Outcome::preserving;
        adjIt->classification = "PRESERVED";
        adjIt->rationale = "Todos os witnesses constitutivos foram empiricamente satisfeitos.";
        adjIt->evidence_references = satisfiedEvIds;
      } else {
        study_.adjudications.push_back({make_id(baseRunId + ":A"), baseRunId,
                                        tinykernel::ontology::Outcome::preserving, "PRESERVED",
                                        "TK-O-0.2.0:all-constitutive-dimensions-v1",
                                        "Todos os witnesses constitutivos foram empiricamente satisfeitos.",
                                        satisfiedEvIds});
      }
      adjudicationsProduced++;
    }

    // 2. Evaluate performed interventions with strict dimensional classification
    for (auto &itv : study_.interventions) {
      if (itv.status == "performed" && itv.target_realization_id) {
        const std::string targetId = *itv.target_realization_id;
        
        // Retrieve real observation run ID
        std::string actualRunId = studyId + ":RUN:" + targetId;
        auto runFound = std::find_if(study_.runs.begin(), study_.runs.end(),
                                     [&](const auto &r) { return r.result_realization_id == targetId; });
        if (runFound != study_.runs.end()) {
          actualRunId = runFound->identity.id;
        }

        std::vector<std::string> itvEvIds;
        bool hasBrokenCausal = false;
        bool hasBrokenOperational = false;
        bool hasBrokenDiscriminative = false;
        bool hasBrokenTemporal = false;
        bool hasBrokenObservational = false;

        for (const auto &o : study_.observations) {
          if (o.realization_id == targetId) {
            itvEvIds.push_back(o.run_id + ":E:" + o.dimension);
            if (!o.satisfied) {
              if (o.dimension == "causal") hasBrokenCausal = true;
              else if (o.dimension == "operational") hasBrokenOperational = true;
              else if (o.dimension == "discriminative") hasBrokenDiscriminative = true;
              else if (o.dimension == "temporal") hasBrokenTemporal = true;
              else if (o.dimension == "observational") hasBrokenObservational = true;
            }
          }
        }

        if (!itvEvIds.empty()) {
          std::string classification = "PRESERVED";
          tinykernel::ontology::Outcome outcome = tinykernel::ontology::Outcome::preserving;
          std::string rationale = "Todos os witnesses observados foram satisfeitos.";

          if (hasBrokenObservational) {
            classification = "WITNESS_COMPROMISED";
            outcome = tinykernel::ontology::Outcome::undetermined;
            rationale = "Aparato observacional incapaz de medir determinismo.";
          } else if (hasBrokenCausal) {
            classification = "BROKEN_CAUSAL";
            outcome = tinykernel::ontology::Outcome::ruptured;
            rationale = "Ruptura causal empírica observada após intervenção.";
          } else if (hasBrokenOperational) {
            classification = "BROKEN_OPERATIONAL";
            outcome = tinykernel::ontology::Outcome::ruptured;
            rationale = "Ruptura operacional observada após intervenção.";
          } else if (hasBrokenDiscriminative) {
            classification = "BROKEN_DISCRIMINATIVE";
            outcome = tinykernel::ontology::Outcome::ruptured;
            rationale = "Ruptura discriminativa observada após intervenção.";
          } else if (hasBrokenTemporal) {
            classification = "BROKEN_TEMPORAL";
            outcome = tinykernel::ontology::Outcome::ruptured;
            rationale = "Ruptura temporal observada após intervenção.";
          }

          auto adjIt = std::find_if(study_.adjudications.begin(), study_.adjudications.end(),
                                    [&](const auto &a) { return a.run_id == actualRunId; });
          if (adjIt != study_.adjudications.end()) {
            adjIt->outcome = outcome;
            adjIt->classification = classification;
            adjIt->rationale = rationale;
            adjIt->evidence_references = itvEvIds;
          } else {
            study_.adjudications.push_back({make_id(actualRunId + ":A"), actualRunId,
                                            outcome, classification,
                                            "TK-O-0.2.0:dimensional-adjudication-v1",
                                            rationale, itvEvIds});
          }
          adjudicationsProduced++;
        }
      }
    }

    study_.investigation.status = "adjudicated";
    saveStudyToRepository(study_);
    emit dataChanged();
    setStatus(QString("Adjudicação concluída: %1 adjudicações registradas. (Claims permanecem OPEN até INFER).").arg(adjudicationsProduced));
  } catch (const std::exception &error) {
    setStatus("Falha na adjudicação: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::inferClaims() {
  try {
    const std::string studyId = active_id_;
    int supportedCount = 0;

    // Attach evidence references from adjudications to relevant claims
    const std::string baseRunId = studyId + ":RUN:BASELINE_EMPIRICAL";
    auto baseAdj = std::find_if(study_.adjudications.begin(), study_.adjudications.end(),
                                [&](const auto &a) { return a.run_id == baseRunId && a.classification == "PRESERVED"; });

    auto suffClaim = std::find_if(study_.claims.begin(), study_.claims.end(),
                                  [&](const auto &c) { return c.identity.id.find(":Q:SUFFICIENCY") != std::string::npos; });
    if (suffClaim != study_.claims.end() && baseAdj != study_.adjudications.end()) {
      suffClaim->evidence_references = baseAdj->evidence_references;
    }

    for (auto &itv : study_.interventions) {
      if (itv.status == "performed" && itv.target_realization_id && !itv.target.empty()) {
        const std::string targetId = *itv.target_realization_id;
        std::string actualRunId = studyId + ":RUN:" + targetId;
        auto runFound = std::find_if(study_.runs.begin(), study_.runs.end(),
                                     [&](const auto &r) { return r.result_realization_id == targetId; });
        if (runFound != study_.runs.end()) actualRunId = runFound->identity.id;

        auto itvAdj = std::find_if(study_.adjudications.begin(), study_.adjudications.end(),
                                   [&](const auto &a) { return a.run_id == actualRunId && a.classification == "BROKEN_CAUSAL"; });

        if (itvAdj != study_.adjudications.end() && baseAdj != study_.adjudications.end()) {
          std::vector<std::string> combinedEv = baseAdj->evidence_references;
          combinedEv.insert(combinedEv.end(), itvAdj->evidence_references.begin(), itvAdj->evidence_references.end());

          const std::string claimId = studyId + ":Q:" + itv.target + "_NECESSITY";
          auto claimIt = std::find_if(study_.claims.begin(), study_.claims.end(), [&](const auto &c) { return c.identity.id == claimId; });
          if (claimIt != study_.claims.end()) {
            claimIt->evidence_references = combinedEv;
            claimIt->intervention_scope = {itv.identity.id};
          } else {
            study_.claims.push_back({make_id(claimId), itv.target,
                                     "A relação associada possui necessidade relativa nesta realização.",
                                     study_.investigation.phenomenon_id, study_.investigation.context_id,
                                     {itv.identity.id}, {studyId + ":W:CAUSAL", studyId + ":W:TEMPORAL"}, combinedEv,
                                     tinykernel::ontology::ClaimLevel::l3_relative_necessity,
                                     tinykernel::ontology::ClaimStatus::open,
                                     "Necessidade demonstrada empiricamente por ruptura causal adjudicada.",
                                     studyId + ":PROV"});
          }
        }
      }
    }

    tinykernel::knowledge::infer_claims(study_);

    for (const auto &c : study_.claims) {
      if (c.status == tinykernel::ontology::ClaimStatus::supported) supportedCount++;
    }

    saveStudyToRepository(study_);
    emit dataChanged();
    setStatus(QString("Inferência concluída: %1 claims sustentados sob adjudicação empírica.").arg(supportedCount));
  } catch (const std::exception &error) {
    setStatus("Falha na inferência: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::addIntervention(const QString &sourceId, const QString &kind,
                               const QString &targetComp, const QString &replacementComp) {
  try {
    const std::string studyId = active_id_;
    const std::string k = kind.toStdString();
    const std::string target = targetComp.toStdString();
    const std::string repl = replacementComp.toStdString();
    const std::string src = sourceId.toStdString();

    const auto sourceIt = std::find_if(study_.realizations.begin(), study_.realizations.end(),
                                       [&](const auto &r) { return r.identity.id == src; });
    if (sourceIt == study_.realizations.end()) throw std::runtime_error("Realização de origem não encontrada");

    std::vector<std::string> newComps = sourceIt->components;
    std::string newLabel = "";

    if (k == "remove" || k == "disable") {
      newComps.erase(std::remove(newComps.begin(), newComps.end(), target), newComps.end());
      newLabel = "sem " + target;
    } else if (k == "replace") {
      for (auto &c : newComps) if (c == target) c = repl;
      newLabel = target + " -> " + repl;
    } else if (k == "perturb") {
      for (auto &c : newComps) if (c == target) c = target + "_perturbed";
      newLabel = target + " perturbado";
    }

    const int itvNum = static_cast<int>(study_.realizations.size());
    const std::string targetRealizationId = studyId + ":R:INT_" + std::to_string(itvNum);

    // Check if there is an existing planned intervention for this target
    auto plannedIt = std::find_if(study_.interventions.begin(), study_.interventions.end(),
                                  [&](const auto &itv) { return itv.kind == k && itv.target == target; });

    std::string itvId;
    if (plannedIt != study_.interventions.end()) {
      plannedIt->target_realization_id = targetRealizationId;
      plannedIt->status = "performed";
      itvId = plannedIt->identity.id;
    } else {
      itvId = studyId + ":I:" + k + "_" + (target.empty() ? std::to_string(itvNum) : target);
      study_.interventions.push_back({make_id(itvId), studyId, k, src, targetRealizationId, target, repl,
                                     "BROKEN_CAUSAL", "performed"});
    }

    study_.realizations.push_back({make_id(targetRealizationId), studyId, newLabel, newComps, static_cast<std::uint32_t>(newComps.size())});
    study_.structures.push_back({make_id(targetRealizationId + ":SIGMA"), targetRealizationId, newComps, newComps});

    // Record structural assembly record (not empirical evidence)
    const std::string structArtifact = "run=" + studyId + ":MATERIALIZED\nsource=" + src + "\nintervention=" + itvId +
        "\nrealization=" + targetRealizationId + "\ncomponents_count=" + std::to_string(newComps.size()) + "\n";
    study_.evidence.push_back({make_id(itvId + ":E:STRUCTURAL_ASSEMBLY"), studyId + ":RUN:SPECIFICATION",
                               studyId + ":W:OBSERVATIONAL", {}, structArtifact,
                               tinykernel::evidence::sha256(structArtifact),
                               tinykernel::ontology::EvidenceType::structural_record});

    if (study_.investigation.status == "formulated" || study_.investigation.status.empty()) {
      study_.investigation.status = "materialized";
    }

    saveStudyToRepository(study_);
    emit dataChanged();
    setStatus(QString("Intervenção %1 materializada. Aguardando observações empíricas.").arg(QString::fromStdString(itvId)));
  } catch (const std::exception &error) {
    setStatus("Falha ao adicionar intervenção: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::setStatus(QString message) {
  status_message_ = std::move(message);
  emit statusMessageChanged();
}
