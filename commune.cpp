#include "commune.h"

#include "codelist.h"

QString Commune::cacheType() const
{
    return QStringLiteral("communes");
}

QString Commune::cleabs() const { return m_cleabs; }
void Commune::setCleabs(const QString &value) { m_cleabs = value; }
QString Commune::statut() const { return m_statut; }
void Commune::setStatut(const QString &value) { m_statut = value; }
int Commune::population() const { return m_population; }
void Commune::setPopulation(int value) { m_population = value; }
QDate Commune::censusDate() const { return m_censusDate; }
void Commune::setCensusDate(const QDate &value) { m_censusDate = value; }
QString Commune::censusOrganisation() const { return m_censusOrganisation; }
void Commune::setCensusOrganisation(const QString &value) { m_censusOrganisation = value; }
QString Commune::cantonCode() const { return m_cantonCode; }
void Commune::setCantonCode(const QString &value) { m_cantonCode = value; }
QString Commune::arrondissementCode() const { return m_arrondissementCode; }
void Commune::setArrondissementCode(const QString &value) { m_arrondissementCode = value; }
QString Commune::departementCode() const { return m_departementCode; }
void Commune::setDepartementCode(const QString &value) { m_departementCode = value; }
QString Commune::regionCode() const { return m_regionCode; }
void Commune::setRegionCode(const QString &value) { m_regionCode = value; }
QString Commune::sirenCode() const { return m_sirenCode; }
void Commune::setSirenCode(const QString &value) { m_sirenCode = value; }
QStringList Commune::epciSirenCodes() const { return m_epciSirenCodes; }
void Commune::setEpciSirenCodes(const QStringList &values) { m_epciSirenCodes = values; }
void Commune::setEpciSirenCodes(const QString &value)
{
    m_epciSirenCodes = CodeList::fromString(value);
}
QString Commune::postalCode() const { return m_postalCode; }
void Commune::setPostalCode(const QString &value) { m_postalCode = value; }
int Commune::cadastralArea() const { return m_cadastralArea; }
void Commune::setCadastralArea(int value) { m_cadastralArea = value; }

QJsonObject Commune::toJson(bool includeChildren) const
{
    QJsonObject object = Entity::toJson(includeChildren);

    if (!m_cleabs.isEmpty())
        object.insert(QStringLiteral("cleabs"), m_cleabs);
    if (!m_statut.isEmpty())
        object.insert(QStringLiteral("statut"), m_statut);

    object.insert(QStringLiteral("population"), m_population);

    if (m_censusDate.isValid())
        object.insert(QStringLiteral("date_recensement"),
                      m_censusDate.toString(Qt::ISODate));
    if (!m_censusOrganisation.isEmpty())
        object.insert(QStringLiteral("organisme_recenseur"), m_censusOrganisation);
    if (!m_cantonCode.isEmpty())
        object.insert(QStringLiteral("code_canton"), m_cantonCode);
    if (!m_arrondissementCode.isEmpty())
        object.insert(QStringLiteral("code_arrondissement"), m_arrondissementCode);
    if (!m_departementCode.isEmpty())
        object.insert(QStringLiteral("code_departement"), m_departementCode);
    if (!m_regionCode.isEmpty())
        object.insert(QStringLiteral("code_region"), m_regionCode);
    if (!m_sirenCode.isEmpty())
        object.insert(QStringLiteral("code_siren"), m_sirenCode);

    object.insert(QStringLiteral("codes_siren_epci"),
                  CodeList::toJson(m_epciSirenCodes));

    if (!m_postalCode.isEmpty())
        object.insert(QStringLiteral("code_postal"), m_postalCode);

    object.insert(QStringLiteral("superficie_cadastrale"), m_cadastralArea);

    return object;
}
