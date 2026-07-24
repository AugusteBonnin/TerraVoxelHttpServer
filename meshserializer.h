#ifndef MESHSERIALIZER_H
#define MESHSERIALIZER_H

#include <QByteArray>
#include <QString>

class Mesh;

class MeshSerializer
{
public:
    static QByteArray serialize(const Mesh &mesh);
    static bool save(const Mesh &mesh,
                     const QString &fileName,
                     QString *errorMessage = nullptr);
    static bool save(const QByteArray &data,
                     const QString &fileName,
                     QString *errorMessage = nullptr);
};

#endif
