#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "france.h"
#include "region.h"
#include "departement.h"
#include "epci.h"
#include "commune.h"

#include <QByteArray>
#include <QSqlDatabase>
#include <QVector>

class Repository
{
public:
    Repository();
    ~Repository();

    Repository(const Repository &) = delete;
    Repository &operator=(const Repository &) = delete;

    bool open(QString *error = nullptr);
    void close();
    bool isOpen() const;

    bool loadFrance(France *, QString *error = nullptr) const;

    QVector<Region> regions(QString *error = nullptr) const;
    bool region(const QString &, Region *, QString *error = nullptr) const;

    QVector<Departement> departements(const QString &, QString *error = nullptr) const;
    bool departement(const QString &, Departement *, QString *error = nullptr) const;

    QVector<Epci> epcis(const QString &, QString *error = nullptr) const;
    bool epci(const QString &, Epci *, QString *error = nullptr) const;

    QVector<Commune> communesForEpci(const QString &, QString *error = nullptr) const;
    bool commune(const QString &, Commune *, QString *error = nullptr) const;

    // Types acceptés : regions, departements, epcis, communes.
    // found vaut false si aucun enregistrement ne correspond au code.
    bool trianglesWkb(const QString &type,
                      const QString &code,
                      QByteArray *wkb,
                      bool *found,
                      QString *error = nullptr) const;

private:
    static QString commonColumns(const QString &);
    static QString meshUrl(const Entity &);
    static QJsonObject childJson(const Entity &);
    static bool meshTable(const QString &type,
                          QString *tableName,
                          QString *codeColumn);

    bool ensureOpen(QString *) const;
    static void setError(QString *, const QString &);

    QString m_connectionName;
    QSqlDatabase m_database;
};

#endif
