#ifndef MESH_H
#define MESH_H

#include "wkbreader.h"

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

class Mesh
{
public:
    bool build(const QVector<QVector<Point2D>> &triangles,
               QString *errorMessage = nullptr);

    const QVector<Point2D> &vertices() const;
    const QVector<quint32> &indices() const;

    double minX() const;
    double minY() const;
    double maxX() const;
    double maxY() const;

    bool isEmpty() const;

private:
    QVector<Point2D> m_vertices;
    QVector<quint32> m_indices;
    double m_minX = 0.0;
    double m_minY = 0.0;
    double m_maxX = 0.0;
    double m_maxY = 0.0;
};

#endif
