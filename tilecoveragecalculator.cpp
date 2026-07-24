#include "tilecoveragecalculator.h"

#include "tilepyramid.h"

#include <cmath>

qint64 TileCoverageCalculator::floorToGrid(double coordinate, qint64 tileSize)
{
    return static_cast<qint64>(std::floor(coordinate / double(tileSize))) * tileSize;
}

QVector<Tuile> TileCoverageCalculator::rectangle(const EntityBounds &bounds, qint64 level)
{
    QVector<Tuile> result;
    if (!bounds.valid || !(bounds.maxX > bounds.minX) || !(bounds.maxY > bounds.minY))
        return result;

    if (!TilePyramid::isValidLevel(level))
        return result;
    const qint64 size = level;
    const qint64 firstX = floorToGrid(bounds.minX, size);
    const qint64 firstY = floorToGrid(bounds.minY, size);
    const qint64 lastX = floorToGrid(std::nextafter(bounds.maxX, bounds.minX), size);
    const qint64 lastY = floorToGrid(std::nextafter(bounds.maxY, bounds.minY), size);

    for (qint64 y = firstY; y <= lastY; y += size) {
        for (qint64 x = firstX; x <= lastX; x += size)
            result.append(Tuile(level, x, y));
    }
    return result;
}
