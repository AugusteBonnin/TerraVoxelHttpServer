#ifndef CODELIST_H
#define CODELIST_H

#include <QJsonArray>
#include <QString>
#include <QStringList>

class CodeList
{
public:
    static QStringList fromString(const QString &value);
    static QJsonArray toJson(const QStringList &values);
    static QJsonArray toJson(const QString &value);
};

#endif // CODELIST_H
