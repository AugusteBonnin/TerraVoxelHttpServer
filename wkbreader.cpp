#include "wkbreader.h"

#include <QtEndian>

#include <cstring>

WkbReader::WkbReader(const QByteArray &data)
    : m_data(data)
{
}

bool WkbReader::readTriangles(QVector<QVector<Point2D>> *triangles,
                              QString *errorMessage)
{
    if (!triangles)
        return fail(QStringLiteral("Destination WKB absente"), errorMessage);

    triangles->clear();
    m_offset = 0;

    if (!readGeometry(triangles, errorMessage))
        return false;

    return !triangles->isEmpty()
           || fail(QStringLiteral("La géométrie ne contient aucun triangle"),
                   errorMessage);
}

bool WkbReader::readGeometry(QVector<QVector<Point2D>> *triangles,
                             QString *errorMessage)
{
    quint8 byteOrder = 0;
    if (!readUInt8(&byteOrder))
        return fail(QStringLiteral("WKB tronqué avant l'ordre des octets"),
                    errorMessage);

    if (byteOrder != 0 && byteOrder != 1)
        return fail(QStringLiteral("Ordre des octets WKB invalide"),
                    errorMessage);

    const bool littleEndian = byteOrder == 1;

    quint32 rawType = 0;
    if (!readUInt32(littleEndian, &rawType))
        return fail(QStringLiteral("WKB tronqué avant le type"), errorMessage);

    // EWKB PostGIS : Z=0x80000000, M=0x40000000, SRID=0x20000000.
    const bool hasZ = (rawType & 0x80000000u) != 0;
    const bool hasM = (rawType & 0x40000000u) != 0;
    const bool hasSrid = (rawType & 0x20000000u) != 0;
    quint32 type = rawType & 0x000000ffu;

    // ISO WKB : 1000/2000/3000 ajoutés au type pour Z/M/ZM.
    if (type == 0 || type > WkbGeometryCollection) {
        const quint32 isoType = rawType & 0x0fffffffu;
        type = isoType % 1000u;
    }

    if (hasSrid) {
        quint32 ignoredSrid = 0;
        if (!readUInt32(littleEndian, &ignoredSrid))
            return fail(QStringLiteral("WKB tronqué avant le SRID"),
                        errorMessage);
    }

    switch (type) {
    case WkbPolygon:
        return readPolygon(littleEndian, hasZ, hasM, triangles, errorMessage);

    case WkbMultiPolygon:
    case WkbGeometryCollection:
        return readGeometryCollection(littleEndian, triangles, errorMessage);

    case WkbPoint:
        return skipPoint(littleEndian, hasZ, hasM, errorMessage);

    case WkbLineString:
        return skipLineString(littleEndian, hasZ, hasM, errorMessage);

    case WkbMultiPoint:
    case WkbMultiLineString:
        return readGeometryCollection(littleEndian, triangles, errorMessage);

    default:
        return fail(QStringLiteral("Type WKB non pris en charge : %1").arg(type),
                    errorMessage);
    }
}

bool WkbReader::readPolygon(bool littleEndian,
                            bool hasZ,
                            bool hasM,
                            QVector<QVector<Point2D>> *triangles,
                            QString *errorMessage)
{
    quint32 ringCount = 0;
    if (!readUInt32(littleEndian, &ringCount))
        return fail(QStringLiteral("Polygone WKB tronqué"), errorMessage);

    if (ringCount == 0)
        return true;

    for (quint32 ringIndex = 0; ringIndex < ringCount; ++ringIndex) {
        quint32 pointCount = 0;
        if (!readUInt32(littleEndian, &pointCount))
            return fail(QStringLiteral("Anneau WKB tronqué"), errorMessage);

        QVector<Point2D> ring;
        ring.reserve(static_cast<qsizetype>(pointCount));

        for (quint32 i = 0; i < pointCount; ++i) {
            Point2D point;
            if (!readPoint(littleEndian, hasZ, hasM, &point))
                return fail(QStringLiteral("Coordonnée WKB tronquée"),
                            errorMessage);
            ring.append(point);
        }

        // Les trous ne sont pas utilisés : la colonne triangles est supposée
        // contenir les polygones élémentaires du maillage.
        if (ringIndex != 0)
            continue;

        if (ring.size() >= 2
            && ring.first().x == ring.last().x
            && ring.first().y == ring.last().y) {
            ring.removeLast();
        }

        if (ring.size() < 3)
            continue;

        // Un triangle contient normalement trois sommets. Un polygone plus
        // grand est transformé en éventail pour conserver le comportement initial.
        for (qsizetype i = 1; i + 1 < ring.size(); ++i)
            triangles->append({ring.at(0), ring.at(i), ring.at(i + 1)});
    }

    return true;
}

bool WkbReader::readGeometryCollection(bool littleEndian,
                                       QVector<QVector<Point2D>> *triangles,
                                       QString *errorMessage)
{
    quint32 geometryCount = 0;
    // Le compteur de la collection utilise l'ordre des octets de son en-tête.
    if (!readUInt32(littleEndian, &geometryCount))
        return fail(QStringLiteral("Collection WKB tronquée"), errorMessage);

    for (quint32 i = 0; i < geometryCount; ++i) {
        if (!readGeometry(triangles, errorMessage))
            return false;
    }

    return true;
}

bool WkbReader::skipPoint(bool littleEndian,
                          bool hasZ,
                          bool hasM,
                          QString *errorMessage)
{
    Point2D ignored;
    return readPoint(littleEndian, hasZ, hasM, &ignored)
           || fail(QStringLiteral("Point WKB tronqué"), errorMessage);
}

bool WkbReader::skipLineString(bool littleEndian,
                               bool hasZ,
                               bool hasM,
                               QString *errorMessage)
{
    quint32 pointCount = 0;
    if (!readUInt32(littleEndian, &pointCount))
        return fail(QStringLiteral("Ligne WKB tronquée"), errorMessage);

    for (quint32 i = 0; i < pointCount; ++i) {
        Point2D ignored;
        if (!readPoint(littleEndian, hasZ, hasM, &ignored))
            return fail(QStringLiteral("Ligne WKB tronquée"), errorMessage);
    }

    return true;
}

bool WkbReader::readPoint(bool littleEndian,
                          bool hasZ,
                          bool hasM,
                          Point2D *point)
{
    if (!point
        || !readDouble(littleEndian, &point->x)
        || !readDouble(littleEndian, &point->y)) {
        return false;
    }

    double ignored = 0.0;
    if (hasZ && !readDouble(littleEndian, &ignored))
        return false;
    if (hasM && !readDouble(littleEndian, &ignored))
        return false;

    return true;
}

bool WkbReader::readUInt8(quint8 *value)
{
    if (!value || m_offset + 1 > m_data.size())
        return false;

    *value = static_cast<quint8>(m_data.at(m_offset));
    ++m_offset;
    return true;
}

bool WkbReader::readUInt32(bool littleEndian, quint32 *value)
{
    if (!value || m_offset + 4 > m_data.size())
        return false;

    const auto *source = reinterpret_cast<const uchar *>(m_data.constData() + m_offset);
    *value = littleEndian ? qFromLittleEndian<quint32>(source)
                          : qFromBigEndian<quint32>(source);
    m_offset += 4;
    return true;
}

bool WkbReader::readDouble(bool littleEndian, double *value)
{
    if (!value || m_offset + 8 > m_data.size())
        return false;

    const auto *source = reinterpret_cast<const uchar *>(m_data.constData() + m_offset);
    const quint64 bits = littleEndian ? qFromLittleEndian<quint64>(source)
                                      : qFromBigEndian<quint64>(source);
    std::memcpy(value, &bits, sizeof(double));
    m_offset += 8;
    return true;
}

bool WkbReader::fail(const QString &message, QString *errorMessage)
{
    if (errorMessage)
        *errorMessage = message;
    return false;
}
