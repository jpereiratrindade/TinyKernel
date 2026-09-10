#pragma once

#include "tinykernel/ontology/types.hpp"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class GuiBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentView READ currentView NOTIFY currentViewChanged)
  Q_PROPERTY(QString activeInvestigationId READ activeInvestigationId NOTIFY activeInvestigationIdChanged)
  Q_PROPERTY(QVariantList investigations READ investigations NOTIFY investigationsChanged)
  Q_PROPERTY(QVariantMap globalStats READ globalStats NOTIFY globalStatsChanged)

  Q_PROPERTY(QString phenomenon READ phenomenon NOTIFY dataChanged)
  Q_PROPERTY(QString context READ context NOTIFY dataChanged)
  Q_PROPERTY(QString profile READ profile NOTIFY dataChanged)
  Q_PROPERTY(QVariantList realizations READ realizations NOTIFY dataChanged)
  Q_PROPERTY(QVariantList interventions READ interventions NOTIFY dataChanged)
  Q_PROPERTY(QVariantList runs READ runs NOTIFY dataChanged)
  Q_PROPERTY(QVariantList claims READ claims NOTIFY dataChanged)
  Q_PROPERTY(QString frontier READ frontier NOTIFY dataChanged)
  Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(QString selectedDetails READ selectedDetails NOTIFY selectedDetailsChanged)

public:
  explicit GuiBridge(const QString &workspace, QObject *parent = nullptr);

  QString currentView() const;
  QString activeInvestigationId() const;
  QVariantList investigations() const;
  QVariantMap globalStats() const;

  QString phenomenon() const;
  QString context() const;
  QString profile() const;
  QVariantList realizations() const;
  QVariantList interventions() const;
  QVariantList runs() const;
  QVariantList claims() const;
  QString frontier() const;
  QString statusMessage() const;
  QString selectedDetails() const;

  Q_INVOKABLE void openInvestigation(const QString &id);
  Q_INVOKABLE void backToHome();
  Q_INVOKABLE void openWizard();
  Q_INVOKABLE void selectEntity(const QString &id);
  Q_INVOKABLE void runInvestigation();
  Q_INVOKABLE void exportInvestigation();
  Q_INVOKABLE void exportCurrent();
  Q_INVOKABLE void createInvestigation(const QVariantMap &config);
  Q_INVOKABLE void addIntervention(const QString &sourceId, const QString &kind,
                                  const QString &targetComponent, const QString &replacementComponent);
  Q_INVOKABLE void injectObservation(const QString &realizationId, const QString &dimension,
                                    bool satisfied, const QString &trace);
  Q_INVOKABLE void adjudicateWitnesses();
  Q_INVOKABLE void inferClaims();
  Q_INVOKABLE void deleteInvestigation(const QString &id);

signals:
  void dataChanged();
  void currentViewChanged();
  void activeInvestigationIdChanged();
  void investigationsChanged();
  void globalStatsChanged();
  void selectedDetailsChanged();
  void statusMessageChanged();

private:
  void setStatus(QString message);
  void refreshStudies();
  void loadStudy(const std::string &id);
  void saveStudyToRepository(const tinykernel::ontology::Study &study);

  QString workspace_;
  QString current_view_{"home"}; // "home" | "wizard" | "workbench"
  std::string active_id_{"TK-0001"};
  tinykernel::ontology::Study study_;
  std::vector<tinykernel::ontology::Study> all_studies_;
  QString status_message_;
  QString selected_details_;
};
