#include "tileset.h"
#include "tilepyramid.h"
#include <QJsonArray>

QString TileSet::id() const { return m_id; }
void TileSet::setId(const QString &id) { m_id = id; }
QString TileSet::name() const { return m_name; }
void TileSet::setName(const QString &name) { m_name = name; }
TileCoverage TileSet::coverage() const { return m_coverage; }
void TileSet::setCoverage(TileCoverage coverage) { m_coverage = coverage; }
int TileSet::level() const { return m_level; }
void TileSet::setLevel(int level) { m_level = level; }
qint64 TileSet::tileSize() const { return TilePyramid::tileSize(m_level); }
const QVector<Tuile> &TileSet::tiles() const { return m_tiles; }
QVector<Tuile> &TileSet::tiles() { return m_tiles; }

void TileSet::setTiles(const QVector<Tuile> &tiles)
{
    m_tiles.clear();
    for (const Tuile &tile : tiles)
        addTile(tile);
}

bool TileSet::addTile(const Tuile &tile)
{
    if (!tile.isValid() || tile.level() != m_level || m_tiles.contains(tile))
        return false;
    m_tiles.append(tile);
    return true;
}

QByteArray TileSet::geometryWkb() const { return m_geometryWkb; }
void TileSet::setGeometryWkb(const QByteArray &geometryWkb) { m_geometryWkb = geometryWkb; }
int TileSet::srid() const { return m_srid; }
void TileSet::setSrid(int srid) { m_srid = srid; }

bool TileSet::usesCustomGeometry() const
{
    return m_coverage == TileCoverage::UserRectangle || m_coverage == TileCoverage::UserPolygon;
}

bool TileSet::isValid() const
{
    if (m_level < Tuile::MinimumLevel || m_level > Tuile::MaximumLevel)
        return false;
    if (usesCustomGeometry() && m_geometryWkb.isEmpty())
        return false;
    for (const Tuile &tile : m_tiles)
        if (tile.level() != m_level)
            return false;
    return true;
}

QJsonObject TileSet::toJson() const
{
    QJsonObject json;
    json.insert(QStringLiteral("id"), m_id);
    json.insert(QStringLiteral("nom"), m_name);
    json.insert(QStringLiteral("couverture"), tileCoverageToString(m_coverage));
    json.insert(QStringLiteral("niveau"), TilePyramid::sizeKey(tileSize()));
    json.insert(QStringLiteral("niveauM"), tileSize());
    json.insert(QStringLiteral("niveauInterne"), m_level);
    json.insert(QStringLiteral("taille"), tileSize());
    json.insert(QStringLiteral("srid"), m_srid);

    QJsonArray array;
    for (const Tuile &tile : m_tiles)
        array.append(tile.toJson(false));
    json.insert(QStringLiteral("elements"), array);
    return json;
}
