#ifndef TILESET_H
#define TILESET_H

#include "tilecoverage.h"
#include "tuile.h"
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

class TileSet
{
public:
    QString id() const;
    void setId(const QString &id);
    QString name() const;
    void setName(const QString &name);
    TileCoverage coverage() const;
    void setCoverage(TileCoverage coverage);
    int level() const;
    void setLevel(int level);
    qint64 tileSize() const;
    const QVector<Tuile> &tiles() const;
    QVector<Tuile> &tiles();
    void setTiles(const QVector<Tuile> &tiles);
    bool addTile(const Tuile &tile);
    QByteArray geometryWkb() const;
    void setGeometryWkb(const QByteArray &geometryWkb);
    int srid() const;
    void setSrid(int srid);
    bool usesCustomGeometry() const;
    bool isValid() const;
    QJsonObject toJson() const;

private:
    QString m_id;
    QString m_name;
    TileCoverage m_coverage = TileCoverage::Contour;
    int m_level = 0;
    QVector<Tuile> m_tiles;
    QByteArray m_geometryWkb;
    int m_srid = 2154;
};

#endif
