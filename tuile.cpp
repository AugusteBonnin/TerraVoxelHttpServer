#include "tuile.h"
#include "tilepyramid.h"

#include <QJsonArray>

Tuile::Tuile(int level, qint64 x, qint64 y)
    : m_level(level), m_x(x), m_y(y)
{
}

bool Tuile::isValid() const
{
    if (m_level < MinimumLevel || m_level > MaximumLevel)
        return false;
    const qint64 tileSize = size();
    return tileSize > 0 && m_x % tileSize == 0 && m_y % tileSize == 0;
}

int Tuile::level() const { return m_level; }
qint64 Tuile::x() const { return m_x; }
qint64 Tuile::y() const { return m_y; }
void Tuile::setLevel(int level) { m_level = level; }
void Tuile::setX(qint64 x) { m_x = x; }
void Tuile::setY(qint64 y) { m_y = y; }

qint64 Tuile::size() const { return TilePyramid::tileSize(m_level); }
qint64 Tuile::xmin() const { return m_x; }
qint64 Tuile::ymin() const { return m_y; }
qint64 Tuile::xmax() const { return m_x + size(); }
qint64 Tuile::ymax() const { return m_y + size(); }
qint64 Tuile::centerX() const { return m_x + size() / 2; }
qint64 Tuile::centerY() const { return m_y + size() / 2; }
QString Tuile::key() const { return TilePyramid::tileKey(*this); }
bool Tuile::canHaveChildren() const { return m_level > MinimumLevel; }
Tuile Tuile::child(ChildPosition position) const { return TilePyramid::child(*this, position); }

QJsonObject Tuile::toJson(bool includeChildren) const
{
    QJsonObject json;
    json.insert(QStringLiteral("cle"), key());
    json.insert(QStringLiteral("niveau"), TilePyramid::sizeKey(size()));
    json.insert(QStringLiteral("niveauM"), size());
    json.insert(QStringLiteral("niveauInterne"), m_level);
    json.insert(QStringLiteral("minX"), xmin());
    json.insert(QStringLiteral("minY"), ymin());
    json.insert(QStringLiteral("est"), xmin());
    json.insert(QStringLiteral("nord"), ymin());
    json.insert(QStringLiteral("taille"), size());
    json.insert(QStringLiteral("xmin"), xmin());
    json.insert(QStringLiteral("ymin"), ymin());
    json.insert(QStringLiteral("xmax"), xmax());
    json.insert(QStringLiteral("ymax"), ymax());
    json.insert(QStringLiteral("centreX"), centerX());
    json.insert(QStringLiteral("centreY"), centerY());
    json.insert(QStringLiteral("cache"), TilePyramid::cacheDirectory(*this));
    json.insert(QStringLiteral("mesh"), TilePyramid::assetUrl(*this, QStringLiteral("mesh.bin")));
    json.insert(QStringLiteral("ortho"), TilePyramid::assetUrl(*this, QStringLiteral("ortho.jpg")));
    json.insert(QStringLiteral("mnt"), TilePyramid::assetUrl(*this, QStringLiteral("mnt.bin")));

    if (includeChildren && canHaveChildren()) {
        QJsonArray array;
        for (const Tuile &childTile : TilePyramid::children(*this))
            array.append(childTile.toJson(false));
        json.insert(QStringLiteral("enfants"), array);
    }
    return json;
}

bool Tuile::operator==(const Tuile &other) const
{
    return m_level == other.m_level && m_x == other.m_x && m_y == other.m_y;
}
