#ifndef COMMUNE_H
#define COMMUNE_H

#include "entity.h"

#include <QDate>
#include <QStringList>

class Commune final : public Entity
{
public:
    QString cacheType() const override;

    QString cleabs() const;
    void setCleabs(const QString &value);

    QString statut() const;
    void setStatut(const QString &value);

    int population() const;
    void setPopulation(int value);

    QDate censusDate() const;
    void setCensusDate(const QDate &value);

    QString censusOrganisation() const;
    void setCensusOrganisation(const QString &value);

    QString cantonCode() const;
    void setCantonCode(const QString &value);

    QString arrondissementCode() const;
    void setArrondissementCode(const QString &value);

    QString departementCode() const;
    void setDepartementCode(const QString &value);

    QString regionCode() const;
    void setRegionCode(const QString &value);

    QString sirenCode() const;
    void setSirenCode(const QString &value);

    QStringList epciSirenCodes() const;
    void setEpciSirenCodes(const QStringList &values);
    void setEpciSirenCodes(const QString &encodedValues);

    QString postalCode() const;
    void setPostalCode(const QString &value);

    int cadastralArea() const;
    void setCadastralArea(int value);

    QJsonObject toJson(bool includeChildren = true) const override;

private:
    QString m_cleabs;
    QString m_statut;
    int m_population = 0;
    QDate m_censusDate;
    QString m_censusOrganisation;
    QString m_cantonCode;
    QString m_arrondissementCode;
    QString m_departementCode;
    QString m_regionCode;
    QString m_sirenCode;
    QStringList m_epciSirenCodes;
    QString m_postalCode;
    int m_cadastralArea = 0;
};

#endif
