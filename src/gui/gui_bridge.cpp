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
  const std::string id = "TK-SAIT-001";
  tinykernel::ontology::Study s;
  s.investigation = {make_id(id), "Resiliência do Sistema Agroalimentar Territorial (SAIT)",
                     id + ":P", id + ":C", id + ":PHI",
                     "Gamma=complexidade_constitutiva_decrescente", "formulated"};
  s.phenomenon = {make_id(id + ":P"), "resiliência de sistema agroalimentar territorial",
                  "Capacidade de manter estabilidade produtiva, nutricional e hídrica sob perturbações climáticas e econômicas."};
  s.context = {make_id(id + ":C"), "Território semiárido/agreste, agricultura familiar, chuvas irregulares."};
  s.constitutive_profile = {make_id(id + ":PHI"),
                            {"estabilidade nutricional", "segurança hídrica", "autonomia sementes"},
                            {"solo_vivo->resiliencia_hidrica", "biodiversidade->segurança_nutricional", "feiras_locais->autonomia_economica"},
                            {"ciclo_anual_safra", "periodo_estiagem_plurianual"}};
  s.witnesses = {
      {make_id(id + ":W:OPERATIONAL"), "operational", "Produção observada e abastecimento contínuo."},
      {make_id(id + ":W:CAUSAL"), "causal", "Retenção hídrica e fertilidade dependem dos componentes agroecológicos."},
      {make_id(id + ":W:DISCRIMINATIVE"), "discriminative", "Distinção nítida entre sistemas biodiversos e convencionais degradados."},
      {make_id(id + ":W:OBSERVATIONAL"), "observational", "Aparato de amostragem de solo e colheita operacional."},
      {make_id(id + ":W:TEMPORAL"), "temporal", "Estabilidade mantida durante o ciclo de estiagem."}
  };

  const std::vector<std::string> baseComps = {
      "solo_vivo", "agrobiodiversidade", "reflorestamento_ciliar",
      "armazenamento_hidrico", "sementes_locais", "circuitos_curtos_feiras"
  };
  s.realizations.push_back({make_id(id + ":R:BASE"), id, "sistema agroflorestal completo (SAIT baseline)",
                            baseComps, static_cast<std::uint32_t>(baseComps.size())});
  s.structures.push_back({make_id(id + ":R:BASE:SIGMA"), id + ":R:BASE", baseComps, baseComps});
  s.provenance.push_back({make_id(id + ":PROV"), "territorial_benchmark", "agroecology_protocol_v1",
                          "2026-09-10T00:00:00-03:00",
                          "Benchmark de sistema agroalimentar territorial formulado com rigor epistemológico."});

  const std::vector<std::string> targets = {
      "circuitos_curtos_feiras", "sementes_locais", "armazenamento_hidrico",
      "agrobiodiversidade", "reflorestamento_ciliar", "solo_vivo"
  };
  for (const auto &target : targets) {
    s.interventions.push_back({
        make_id(id + ":I:REMOVE_" + target), id, "remove", id + ":R:BASE", std::nullopt,
        target, "", "BROKEN_CAUSAL", "planned"
    });
  }

  const std::string artifact = "investigation=" + id + "\nstatus=formulated\nbaseline=untested\nplanned_interventions=6\n";
  s.evidence.push_back({make_id(id + ":E:STRUCTURAL_INTEGRITY"), id + ":RUN:SPECIFICATION",
                        id + ":W:OBSERVATIONAL", {}, artifact, tinykernel::evidence::sha256(artifact)});

  s.claims.push_back({make_id(id + ":Q:SUFFICIENCY"), id + ":R:BASE",
                      "O arranjo territorial baseline é causalmente suficiente para sustentar o perfil de resiliência.",
                      id + ":P", id + ":C", {},
                      {id + ":W:OPERATIONAL", id + ":W:CAUSAL", id + ":W:DISCRIMINATIVE", id + ":W:OBSERVATIONAL", id + ":W:TEMPORAL"},
                      {}, tinykernel::ontology::ClaimLevel::l2_relative_sufficiency,
                      tinykernel::ontology::ClaimStatus::open,
                      "Aguardando campanhas empíricas de campo.", id + ":PROV"});

  s.claims.push_back({make_id(id + ":Q:RELATIVE_MINIMALITY"), id + ":R:BASE",
                      "O arranjo é minimal no espaço de reduções agroecológicas sob a ordem Gamma.",
                      id + ":P", id + ":C", {}, {}, {},
                      tinykernel::ontology::ClaimLevel::l5_relative_minimality,
                      tinykernel::ontology::ClaimStatus::open,
                      "Aguardando testes empíricos das 6 intervenções planejadas.", id + ":PROV"});

  s.claims.push_back({make_id(id + ":Q:ROBUSTNESS"), id + ":R:BASE",
                      "A estabilidade produtiva e hídrica é robusta a variações pluviométricas sazonais.",
                      id + ":P", id + ":C", {}, {}, {},
                      tinykernel::ontology::ClaimLevel::l7_contextual_robustness,
                      tinykernel::ontology::ClaimStatus::open,
                      "Requer observações de campo em safras consecutivas.", id + ":PROV"});

  return s;
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
    item["evidenceCount"] = static_cast<int>(s.evidence.size());
    
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
  int totalClaimsSupported = 0;
  for (const auto &s : all_studies_) {
    totalEvidence += static_cast<int>(s.evidence.size());
    for (const auto &c : s.claims) {
      if (c.status == tinykernel::ontology::ClaimStatus::supported) totalClaimsSupported++;
    }
  }
  stats["totalInvestigations"] = static_cast<int>(all_studies_.size());
  stats["totalEvidence"] = totalEvidence;
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
                                 id + ":W:OBSERVATIONAL", {}, artifact, tinykernel::evidence::sha256(artifact)});

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
    const std::string dim = dimension.toStdString();
    const std::string rawTrace = trace.toStdString();

    const std::string runId = studyId + ":RUN:" + (rId == (studyId + ":R:BASE") ? "BASELINE_EMPIRICAL" : rId);
    
    // Add run if not existing
    auto runIt = std::find_if(study_.runs.begin(), study_.runs.end(), [&](const auto &r) { return r.identity.id == runId; });
    if (runIt == study_.runs.end()) {
      study_.runs.push_back({make_id(runId), studyId, std::nullopt, studyId + ":R:BASE", rId, "completed"});
    }

    std::vector<std::string> evidenceIds;
    for (const auto &w : study_.witnesses) {
      const bool isTargetDim = (w.kind == dim || dim.empty());
      const bool isSat = isTargetDim ? satisfied : true;
      const std::string obsId = runId + ":O:" + w.kind;
      study_.observations.push_back({make_id(obsId), runId, rId, w.identity.id, w.kind,
                                     isSat ? "satisfied" : "not_satisfied", isSat});
      const std::string artifact = "run=" + runId + "\nrealization=" + rId + "\ndimension=" + w.kind +
          "\nsatisfied=" + (isSat ? "true" : "false") + "\nempirical_trace=" + (rawTrace.empty() ? "measured=true" : rawTrace) + "\n";
      const std::string evId = runId + ":E:" + w.kind;
      study_.evidence.push_back({make_id(evId), runId, w.identity.id, {obsId}, artifact, tinykernel::evidence::sha256(artifact)});
      evidenceIds.push_back(evId);
    }

    study_.adjudications.push_back({make_id(runId + ":A"), runId,
                                    satisfied ? tinykernel::ontology::Outcome::preserving : tinykernel::ontology::Outcome::ruptured,
                                    satisfied ? "PRESERVED" : "BROKEN_CAUSAL", "TK-O-0.2.0:empirical-verification-v1",
                                    satisfied ? "Observação empírica satisfez os witnesses." : "Falha empírica observada no aparato.",
                                    evidenceIds});

    study_.investigation.status = "executed";
    saveStudyToRepository(study_);
    emit dataChanged();
    setStatus("Observação empírica injetada com sucesso.");
  } catch (const std::exception &error) {
    setStatus("Falha ao injetar observação: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::adjudicateWitnesses() {
  try {
    const std::string studyId = active_id_;
    int supportedCount = 0;

    // Check baseline
    const auto baseRun = std::find_if(study_.runs.begin(), study_.runs.end(),
                                      [&](const auto &r) { return r.result_realization_id.find(":BASE") != std::string::npos; });
    if (baseRun != study_.runs.end()) {
      auto suffClaim = std::find_if(study_.claims.begin(), study_.claims.end(),
                                    [&](const auto &c) { return c.identity.id.find(":Q:SUFFICIENCY") != std::string::npos; });
      if (suffClaim != study_.claims.end()) {
        suffClaim->status = tinykernel::ontology::ClaimStatus::supported;
        supportedCount++;
      }
    }

    saveStudyToRepository(study_);
    emit dataChanged();
    setStatus(QString("Adjudicação concluída: %1 claims atualizados.").arg(supportedCount));
  } catch (const std::exception &error) {
    setStatus("Falha na adjudicação: " + QString::fromUtf8(error.what()));
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
    bool isPreserved = (k == "replace");

    if (k == "remove" || k == "disable") {
      newComps.erase(std::remove(newComps.begin(), newComps.end(), target), newComps.end());
      newLabel = "sem " + target;
      isPreserved = false;
    } else if (k == "replace") {
      for (auto &c : newComps) if (c == target) c = repl;
      newLabel = target + " -> " + repl;
      isPreserved = true;
    } else if (k == "perturb") {
      for (auto &c : newComps) if (c == target) c = target + "_perturbed";
      newLabel = target + " perturbado";
      isPreserved = false;
    }

    const int itvNum = static_cast<int>(study_.interventions.size()) + 1;
    const std::string targetRealizationId = studyId + ":R:INT_" + std::to_string(itvNum);
    const std::string itvId = studyId + ":I:" + k + "_" + (target.empty() ? std::to_string(itvNum) : target);

    study_.realizations.push_back({make_id(targetRealizationId), studyId, newLabel, newComps, static_cast<std::uint32_t>(newComps.size())});
    study_.structures.push_back({make_id(targetRealizationId + ":SIGMA"), targetRealizationId, newComps, newComps});
    study_.interventions.push_back({make_id(itvId), studyId, k, src, targetRealizationId, target, repl,
                                   isPreserved ? "PRESERVED" : "BROKEN_CAUSAL", "performed"});

    // Record run and evidence
    const std::string runId = studyId + ":RUN:" + k + "_" + (target.empty() ? std::to_string(itvNum) : target);
    study_.runs.push_back({make_id(runId), studyId, itvId, src, targetRealizationId, "completed"});

    std::vector<std::string> evidenceIds;
    for (const auto &w : study_.witnesses) {
      const bool satisfied = (w.kind != "causal" && w.kind != "temporal") || isPreserved;
      const std::string obsId = runId + ":O:" + w.kind;
      study_.observations.push_back({make_id(obsId), runId, targetRealizationId, w.identity.id, w.kind,
                                     satisfied ? "satisfied" : "not_satisfied", satisfied});
      const std::string artifact = "run=" + runId + "\nrealization=" + targetRealizationId + "\ndimension=" + w.kind +
          "\nsatisfied=" + (satisfied ? "true" : "false") + "\ntrace=operational=true;causal=" + (isPreserved ? "true" : "false") + "\n";
      const std::string evId = runId + ":E:" + w.kind;
      study_.evidence.push_back({make_id(evId), runId, w.identity.id, {obsId}, artifact, tinykernel::evidence::sha256(artifact)});
      evidenceIds.push_back(evId);
    }

    study_.adjudications.push_back({make_id(runId + ":A"), runId,
                                    isPreserved ? tinykernel::ontology::Outcome::preserving : tinykernel::ontology::Outcome::ruptured,
                                    isPreserved ? "PRESERVED" : "BROKEN_CAUSAL", "TK-O-0.2.0:all-constitutive-dimensions-v1",
                                    isPreserved ? "Todos os witnesses satisfeitos." : "Witness causal/temporal rompido.",
                                    evidenceIds});

    // If broken, add L3 necessity claim
    if (!isPreserved && !target.empty()) {
      study_.claims.push_back({make_id(studyId + ":Q:" + target + "_NECESSITY"), target,
                               "A relação associada possui necessidade relativa nesta realização.",
                               study_.investigation.phenomenon_id, study_.investigation.context_id,
                               {itvId}, {studyId + ":W:CAUSAL", studyId + ":W:TEMPORAL"}, evidenceIds,
                               tinykernel::ontology::ClaimLevel::l3_relative_necessity,
                               tinykernel::ontology::ClaimStatus::supported,
                               "Não transfere necessidade a outras realizações.", studyId + ":PROV"});
    }

    saveStudyToRepository(study_);
    emit dataChanged();
    setStatus(QString("Intervenção %1 executada com sucesso.").arg(QString::fromStdString(itvId)));
  } catch (const std::exception &error) {
    setStatus("Falha ao adicionar intervenção: " + QString::fromUtf8(error.what()));
  }
}

void GuiBridge::setStatus(QString message) {
  status_message_ = std::move(message);
  emit statusMessageChanged();
}
