#ifndef TILEPYRAMID_H
#define TILEPYRAMID_H

#include "tuile.h"
#include <QList>
#include <QString>

class TilePyramid
{
public:
    static bool isValidLevel(qint64 levelMeters);
    static QString sizeKey(qint64 sizeMeters);
    static QString tileKey(const Tuile &tile);
    static QString tileKey(qint64 level, qint64 x, qint64 y);
    static QString cacheDirectory(const Tuile &tile);
    static QString relativeAssetPath(const Tuile &tile, const QString &assetName);
    static QString assetUrl(const Tuile &tile, const QString &assetName);
    static Tuile child(const Tuile &parent, Tuile::ChildPosition position);
    static QList<Tuile> children(const Tuile &parent);
    static bool hasParent(const Tuile &tile);
    static Tuile gridParent(const Tuile &tile);

};

#endif
