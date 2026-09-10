#pragma once

#include "tinykernel/ontology/types.hpp"

#include <QObject>
#include <QString>
#include <QVariantList>

class GuiBridge final : public QObject {
  Q_OBJECT
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
  Q_INVOKABLE void selectEntity(const QString &id);
  Q_INVOKABLE void runInvestigation();
  Q_INVOKABLE void exportInvestigation();

signals:
  void dataChanged();
  void selectedDetailsChanged();
  void statusMessageChanged();

private:
  void setStatus(QString message);
  tinykernel::ontology::Study study_;
  QString workspace_;
  QString status_message_;
  QString selected_details_;
};
