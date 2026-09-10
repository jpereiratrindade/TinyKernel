#pragma once

#include "tinykernel/ontology/types.hpp"

#include <QObject>
#include <QString>
#include <QVariantList>

class GuiBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString phenomenon READ phenomenon CONSTANT)
  Q_PROPERTY(QString context READ context CONSTANT)
  Q_PROPERTY(QString profile READ profile CONSTANT)
  Q_PROPERTY(QVariantList realizations READ realizations CONSTANT)
  Q_PROPERTY(QVariantList interventions READ interventions CONSTANT)
  Q_PROPERTY(QVariantList runs READ runs CONSTANT)
  Q_PROPERTY(QVariantList claims READ claims CONSTANT)
  Q_PROPERTY(QString frontier READ frontier CONSTANT)
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
  QString selectedDetails() const;
  Q_INVOKABLE void selectEntity(const QString &id);

signals:
  void selectedDetailsChanged();

private:
  tinykernel::ontology::Study study_;
  QString selected_details_;
};
