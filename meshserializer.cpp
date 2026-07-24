#include "meshserializer.h"
#include "mesh.h"

#include <QDataStream>
#include <QSaveFile>

QByteArray MeshSerializer::serialize(const Mesh &mesh)
{
    if (mesh.isEmpty())
        return {};

    QByteArray output;
    QDataStream stream(&output, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    stream.writeRawData("TVM1", 4);
    stream << quint32(1)
           << quint32(mesh.vertices().size())
           << quint32(mesh.indices().size());

    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    stream << mesh.minX() << mesh.minY() << mesh.maxX() << mesh.maxY();

    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    for (const Point2D &vertex : mesh.vertices())
        stream << float(vertex.x) << float(vertex.y) << float(0.0f);
    for (quint32 index : mesh.indices())
        stream << index;

    return output;
}

bool MeshSerializer::save(const Mesh &mesh,
                          const QString &fileName,
                          QString *errorMessage)
{
    const QByteArray data = serialize(mesh);
    return save(data, fileName, errorMessage);
}

bool MeshSerializer::save(const QByteArray &data,
                          const QString &fileName,
                          QString *errorMessage)
{
    if (data.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("La sérialisation du mesh a échoué");
        return false;
    }

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    if (file.write(data) != data.size()) {
        if (errorMessage)
            *errorMessage = file.errorString();
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    return true;
}
