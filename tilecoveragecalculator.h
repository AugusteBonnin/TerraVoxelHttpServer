#ifndef TILECOVERAGECALCULATOR_H
#define TILECOVERAGECALCULATOR_H

#include "entity.h"
#include "tuile.h"

#include <QVector>

class TileCoverageCalculator
{
public:
    static QVector<Tuile> rectangle(const EntityBounds &bounds, qint64 level);
    static qint64 floorToGrid(double coordinate, qint64 tileSize);
};

#endif
