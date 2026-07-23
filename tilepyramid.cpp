#include "tilepyramid.h"

#include <stdexcept>

void TilePyramid::validateLevel(int level)
{
    if (level < Tuile::MinimumLevel || level > Tuile::MaximumLevel)
        throw std::out_of_range("Tile level must be between -3 and 10");
}

qint64 TilePyramid::tileSize(int level)
{
    validateLevel(level);
    if (level >= 0)
        return Tuile::BaseSizeMeters << level;
    return Tuile::BaseSizeMeters >> (-level);
}

bool TilePyramid::levelForTileSize(qint64 sizeMeters, int *level)
{
    for (int candidate = Tuile::MinimumLevel;
         candidate <= Tuile::MaximumLevel;
         ++candidate) {
        if (tileSize(candidate) == sizeMeters) {
            if (level)
                *level = candidate;
            return true;
        }
    }
    return false;
}

QString TilePyramid::sizeKey(qint64 sizeMeters)
{
    return QStringLiteral("%1").arg(sizeMeters, 7, 10, QLatin1Char('0'));
}

QString TilePyramid::tileKey(const Tuile &tile)
{
    return QStringLiteral("%1/%2/%3")
        .arg(sizeKey(tile.size()))
        .arg(tile.xmin())
        .arg(tile.ymin());
}

QString TilePyramid::tileKey(int level, qint64 x, qint64 y)
{
    return tileKey(Tuile(level, x, y));
}

QString TilePyramid::cacheDirectory(const Tuile &tile)
{
    return QStringLiteral("tiles/%1").arg(tile.key());
}

QString TilePyramid::relativeAssetPath(const Tuile &tile, const QString &assetName)
{
    QString safeName = assetName;
    safeName.remove(QStringLiteral(".."));
    safeName.remove(QLatin1Char('/'));
    safeName.remove(QLatin1Char('\\'));
    return QStringLiteral("%1/%2").arg(cacheDirectory(tile), safeName);
}

QString TilePyramid::assetUrl(const Tuile &tile, const QString &assetName)
{
    return QStringLiteral("/%1").arg(relativeAssetPath(tile, assetName));
}

Tuile TilePyramid::child(const Tuile &parent, Tuile::ChildPosition position)
{
    if (!parent.canHaveChildren())
        return {};

    const int childLevel = parent.level() - 1;
    const qint64 parentSize = parent.size();
    const qint64 childSize = parentSize / 2;
    qint64 childX = parent.x();
    qint64 childY = parent.y();

    switch (position) {
    case Tuile::ChildPosition::SouthWest: break;
    case Tuile::ChildPosition::SouthEast: childX += childSize; break;
    case Tuile::ChildPosition::NorthWest: childY += childSize; break;
    case Tuile::ChildPosition::NorthEast: childX += childSize; childY += childSize; break;
    case Tuile::ChildPosition::Center: childX += parentSize / 4; childY += parentSize / 4; break;
    }

    return Tuile(childLevel, childX, childY);
}

QList<Tuile> TilePyramid::children(const Tuile &parent)
{
    if (!parent.canHaveChildren())
        return {};

    return {
        child(parent, Tuile::ChildPosition::SouthWest),
        child(parent, Tuile::ChildPosition::SouthEast),
        child(parent, Tuile::ChildPosition::NorthWest),
        child(parent, Tuile::ChildPosition::NorthEast),
        child(parent, Tuile::ChildPosition::Center)
    };
}

bool TilePyramid::hasParent(const Tuile &tile)
{
    return tile.level() < Tuile::MaximumLevel;
}

Tuile TilePyramid::gridParent(const Tuile &tile)
{
    if (!hasParent(tile))
        return {};

    const int parentLevel = tile.level() + 1;
    const qint64 parentSize = tileSize(parentLevel);
    auto floorMultiple = [](qint64 value, qint64 divisor) {
        qint64 quotient = value / divisor;
        qint64 remainder = value % divisor;
        if (remainder != 0 && value < 0)
            --quotient;
        return quotient * divisor;
    };

    return Tuile(parentLevel,
                 floorMultiple(tile.x(), parentSize),
                 floorMultiple(tile.y(), parentSize));
}
