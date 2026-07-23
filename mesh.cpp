#include "mesh.h"

#include <QHash>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
QString pointKey(double x, double y)
{
    constexpr double quantization = 10000000.0;
    return QString::number(std::llround(x * quantization))
         + QLatin1Char(':')
         + QString::number(std::llround(y * quantization));
}
}

bool Mesh::build(const QVector<QVector<Point2D>> &triangles,
                 QString *errorMessage)
{
    if (triangles.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Aucun triangle à convertir");
        return false;
    }

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    for (const auto &triangle : triangles) {
        for (const Point2D &point : triangle) {
            minX = std::min(minX, point.x);
            minY = std::min(minY, point.y);
            maxX = std::max(maxX, point.x);
            maxY = std::max(maxY, point.y);
        }
    }

    const double side = std::max(maxX - minX, maxY - minY);
    if (!(side > 0.0) || !std::isfinite(side)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Emprise géométrique invalide");
        return false;
    }

    const double centerX = (minX + maxX) * 0.5;
    const double centerY = (minY + maxY) * 0.5;
    m_minX = centerX - side * 0.5;
    m_minY = centerY - side * 0.5;
    m_maxX = m_minX + side;
    m_maxY = m_minY + side;

    m_vertices.clear();
    m_indices.clear();

    QHash<QString, quint32> vertexByKey;
    m_indices.reserve(triangles.size() * 3);

    for (const auto &triangle : triangles) {
        if (triangle.size() != 3)
            continue;

        for (const Point2D &point : triangle) {
            const double nx = (point.x - m_minX) / side;
            const double ny = (point.y - m_minY) / side;
            const QString key = pointKey(nx, ny);
            auto it = vertexByKey.constFind(key);
            quint32 index;

            if (it == vertexByKey.cend()) {
                if (m_vertices.size() >= std::numeric_limits<quint32>::max()) {
                    if (errorMessage)
                        *errorMessage = QStringLiteral("Trop de sommets pour des indices uint32");
                    return false;
                }
                index = static_cast<quint32>(m_vertices.size());
                m_vertices.append({nx, ny});
                vertexByKey.insert(key, index);
            } else {
                index = it.value();
            }
            m_indices.append(index);
        }
    }

    if (m_indices.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Le maillage indexé est vide");
        return false;
    }
    return true;
}

const QVector<Point2D> &Mesh::vertices() const { return m_vertices; }
const QVector<quint32> &Mesh::indices() const { return m_indices; }
double Mesh::minX() const { return m_minX; }
double Mesh::minY() const { return m_minY; }
double Mesh::maxX() const { return m_maxX; }
double Mesh::maxY() const { return m_maxY; }
bool Mesh::isEmpty() const { return m_indices.isEmpty(); }
