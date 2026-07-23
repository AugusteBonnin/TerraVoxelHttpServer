#ifndef TILECOVERAGE_H
#define TILECOVERAGE_H

#include <QString>
#include <optional>

enum class TileCoverage
{
    Square,
    Rectangle,
    Contour,
    UserRectangle,
    UserPolygon
};

inline QString tileCoverageToString(TileCoverage coverage)
{
    switch (coverage) {
    case TileCoverage::Square: return QStringLiteral("carre");
    case TileCoverage::Rectangle: return QStringLiteral("rectangle");
    case TileCoverage::Contour: return QStringLiteral("contour");
    case TileCoverage::UserRectangle: return QStringLiteral("user_rectangle");
    case TileCoverage::UserPolygon: return QStringLiteral("user_polygon");
    }
    return {};
}

inline std::optional<TileCoverage> tileCoverageFromString(const QString &value)
{
    if (value == QStringLiteral("carre")) return TileCoverage::Square;
    if (value == QStringLiteral("rectangle")) return TileCoverage::Rectangle;
    if (value == QStringLiteral("contour")) return TileCoverage::Contour;
    if (value == QStringLiteral("user_rectangle")) return TileCoverage::UserRectangle;
    if (value == QStringLiteral("user_polygon")) return TileCoverage::UserPolygon;
    return std::nullopt;
}

#endif
