#ifndef TUILE_H
#define TUILE_H

#include <QJsonObject>
#include <QString>
#include <QtGlobal>

class Tuile
{
public:
    static constexpr qint64 MinimumLevel = 125;
    static constexpr qint64 MaximumLevel = 1024000;

    enum class ChildPosition
    {
        SouthWest,
        SouthEast,
        NorthWest,
        NorthEast,
        Center
    };

    Tuile() = default;
    Tuile(qint64 level, qint64 x, qint64 y);

    bool isValid() const;
    qint64 level() const;
    qint64 x() const;
    qint64 y() const;
    void setLevel(qint64 level);
    void setX(qint64 x);
    void setY(qint64 y);

    qint64 size() const;
    qint64 xmin() const;
    qint64 ymin() const;
    qint64 xmax() const;
    qint64 ymax() const;
    qint64 centerX() const;
    qint64 centerY() const;

    QString key() const;
    bool canHaveChildren() const;
    Tuile child(ChildPosition position) const;
    QJsonObject toJson(bool includeChildren = false) const;

    bool operator==(const Tuile &other) const;

private:
    qint64 m_level = 0;
    qint64 m_x = 0;
    qint64 m_y = 0;
};

inline size_t qHash(const Tuile &tile, size_t seed = 0)
{
    seed = qHash(tile.level(), seed);
    seed = qHash(tile.x(), seed);
    return qHash(tile.y(), seed);
}

#endif
