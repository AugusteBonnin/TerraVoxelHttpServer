#include "epci.h"

#include "codelist.h"

QString Epci::cacheType() const
{
    return QStringLiteral("epci");
}

QString Epci::cleabs() const
{
    return m_cleabs;
}

void Epci::setCleabs(const QString &value)
{
    m_cleabs = value;
}

QString Epci::nature() const
{
    return m_nature;
}

void Epci::setNature(const QString &value)
{
    m_nature = value;
}

QStringList Epci::memberCommuneCodes() const
{
    return m_memberCommuneCodes;
}

void Epci::setMemberCommuneCodes(const QStringList &codes)
{
    m_memberCommuneCodes = codes;
}

void Epci::setMemberCommuneCodes(const QString &encodedCodes)
{
    m_memberCommuneCodes = CodeList::fromString(encodedCodes);
}

QStringList Epci::memberDepartementCodes() const
{
    return m_memberDepartementCodes;
}

void Epci::setMemberDepartementCodes(const QStringList &codes)
{
    m_memberDepartementCodes = codes;
}

void Epci::setMemberDepartementCodes(const QString &encodedCodes)
{
    m_memberDepartementCodes = CodeList::fromString(encodedCodes);
}

QJsonObject Epci::toJson(bool includeChildren) const
{
    QJsonObject object = Entity::toJson(includeChildren);

    if (!m_cleabs.isEmpty())
        object.insert(QStringLiteral("cleabs"), m_cleabs);

    if (!m_nature.isEmpty())
        object.insert(QStringLiteral("nature"), m_nature);

    object.insert(QStringLiteral("codes_departements_membres"),
                  CodeList::toJson(m_memberDepartementCodes));
    object.insert(QStringLiteral("codes_communes_membres"),
                  CodeList::toJson(m_memberCommuneCodes));
    object.insert(QStringLiteral("interdepartemental"),
                  m_memberDepartementCodes.size() > 1);

    return object;
}
