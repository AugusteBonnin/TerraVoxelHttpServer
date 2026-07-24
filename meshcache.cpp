#include "meshcache.h"
#include "mesh.h"
#include "meshserializer.h"
#include "wkbreader.h"

#include <QDir>
#include <QFile>

#include <utility>

MeshCache::MeshCache(QString cacheRoot)
    : m_cacheRoot(std::move(cacheRoot))
{
}

bool MeshCache::validPathPart(const QString &value)
{
    if (value.isEmpty())
        return false;

    for (const QChar character : value) {
        if (!character.isLetterOrNumber()
            && character != QLatin1Char('-')
            && character != QLatin1Char('_')) {
            return false;
        }
    }
    return true;
}

QString MeshCache::directory(const QString &type, const QString &code) const
{
    if (!validPathPart(type) || !validPathPart(code))
        return {};

    return QDir(m_cacheRoot).filePath(type + QLatin1Char('/') + code);
}

QString MeshCache::meshPath(const QString &type, const QString &code) const
{
    const QString dir = directory(type, code);
    return dir.isEmpty() ? QString() : QDir(dir).filePath(QStringLiteral("mesh.bin"));
}

bool MeshCache::loadOrCreate(const QString &type,
                             const QString &code,
                             const QByteArray &trianglesWkb,
                             QByteArray *data,
                             QString *errorMessage) const
{
    if (!data) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Paramètre de sortie absent");
        return false;
    }
    data->clear();

    const QString path = meshPath(type, code);
    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Type ou code de mesh invalide");
        return false;
    }

    QFile cachedFile(path);
    if (cachedFile.exists()) {
        if (!cachedFile.open(QIODevice::ReadOnly)) {
            if (errorMessage)
                *errorMessage = cachedFile.errorString();
            return false;
        }
        *data = cachedFile.readAll();
        if (!data->isEmpty())
            return true;
        data->clear();
        cachedFile.close();
    }

    if (trianglesWkb.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Le WKB du mesh %1/%2 est vide").arg(type, code);
        return false;
    }

    QVector<QVector<Point2D>> triangles;
    WkbReader reader(trianglesWkb);
    if (!reader.readTriangles(&triangles, errorMessage))
        return false;

    Mesh mesh;
    if (!mesh.build(triangles, errorMessage))
        return false;

    const QString dir = directory(type, code);
    if (!QDir().mkpath(dir)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Impossible de créer %1").arg(dir);
        return false;
    }

    *data = MeshSerializer::serialize(mesh);
    if (data->isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("La sérialisation du mesh a échoué");
        return false;
    }
    return MeshSerializer::save(*data, path, errorMessage);
}
