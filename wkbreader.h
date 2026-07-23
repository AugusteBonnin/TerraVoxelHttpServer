#ifndef WKBREADER_H
#define WKBREADER_H

#include <QByteArray>
#include <QString>
#include <QVector>

struct Point2D
{
    double x = 0.0;
    double y = 0.0;
};

class WkbReader
{
public:
    explicit WkbReader(const QByteArray &data);

    bool readTriangles(QVector<QVector<Point2D>> *triangles,
                       QString *errorMessage = nullptr);

private:
    static constexpr quint32 WkbPoint = 1;
    static constexpr quint32 WkbLineString = 2;
    static constexpr quint32 WkbPolygon = 3;
    static constexpr quint32 WkbMultiPoint = 4;
    static constexpr quint32 WkbMultiLineString = 5;
    static constexpr quint32 WkbMultiPolygon = 6;
    static constexpr quint32 WkbGeometryCollection = 7;

    bool readGeometry(QVector<QVector<Point2D>> *triangles,
                      QString *errorMessage);

    bool readPolygon(bool littleEndian,
                     bool hasZ,
                     bool hasM,
                     QVector<QVector<Point2D>> *triangles,
                     QString *errorMessage);

    bool readGeometryCollection(bool littleEndian,
                                QVector<QVector<Point2D>> *triangles,
                                QString *errorMessage);

    bool skipPoint(bool littleEndian,
                   bool hasZ,
                   bool hasM,
                   QString *errorMessage);

    bool skipLineString(bool littleEndian,
                        bool hasZ,
                        bool hasM,
                        QString *errorMessage);

    bool readPoint(bool littleEndian,
                   bool hasZ,
                   bool hasM,
                   Point2D *point);

    bool readUInt8(quint8 *value);
    bool readUInt32(bool littleEndian, quint32 *value);
    bool readDouble(bool littleEndian, double *value);

    static bool fail(const QString &message, QString *errorMessage);

    const QByteArray &m_data;
    qsizetype m_offset = 0;
};

#endif // WKBREADER_H
