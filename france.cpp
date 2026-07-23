#include "france.h"

France::France()
{
    setCode(QStringLiteral("FR"));
    setName(QStringLiteral("France"));
    setUpperName(QStringLiteral("FRANCE"));
}

QString France::cacheType() const
{
    return QStringLiteral("france");
}

bool France::hasMesh() const
{
    return false;
}
