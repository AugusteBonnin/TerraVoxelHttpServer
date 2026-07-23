#include "codelist.h"

QStringList CodeList::fromString(const QString &value)
{
    QStringList result;
    const QStringList parts = value.split(QLatin1Char(';'), Qt::SkipEmptyParts);

    for (QString code : parts) {
        code = code.trimmed();
        if (!code.isEmpty() && !result.contains(code))
            result.append(code);
    }

    return result;
}

QJsonArray CodeList::toJson(const QStringList &values)
{
    QJsonArray array;
    for (const QString &value : values)
        array.append(value);
    return array;
}

QJsonArray CodeList::toJson(const QString &value)
{
    return toJson(fromString(value));
}
