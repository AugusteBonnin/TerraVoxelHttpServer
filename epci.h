#ifndef EPCI_H
#define EPCI_H

#include "entity.h"

#include <QStringList>

class Epci final : public Entity
{
public:
    QString cacheType() const override;

    QString cleabs() const;
    void setCleabs(const QString &cleabs);

    QString nature() const;
    void setNature(const QString &nature);

    QStringList memberCommuneCodes() const;
    void setMemberCommuneCodes(const QStringList &codes);
    void setMemberCommuneCodes(const QString &encodedCodes);

    QStringList memberDepartementCodes() const;
    void setMemberDepartementCodes(const QStringList &codes);
    void setMemberDepartementCodes(const QString &encodedCodes);

    QJsonObject toJson(bool includeChildren = true) const override;

private:
    QString m_cleabs;
    QString m_nature;
    QStringList m_memberCommuneCodes;
    QStringList m_memberDepartementCodes;
};

#endif
