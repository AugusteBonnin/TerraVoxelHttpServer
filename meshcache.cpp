#include "meshcache.h"
#include "entity.h"
#include "mesh.h"
#include "meshserializer.h"
#include "wkbreader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

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

QString MeshCache::directory(const Entity &entity) const
{
    return QDir(m_cacheRoot).filePath(entity.cacheType() + QLatin1Char('/') + entity.code());
}

QString MeshCache::meshPath(const QString &type, const QString &code) const
{
    const QString dir = directory(type, code);
    return dir.isEmpty() ? QString() : QDir(dir).filePath(QStringLiteral("mesh.bin"));
}

QString MeshCache::url(const Entity &entity) const
{
    return QStringLiteral("/cache/%1/%2/mesh.bin")
        .arg(entity.cacheType(), entity.code());
}

bool MeshCache::meshData(const QString &type,
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

    if (!MeshSerializer::save(mesh, path, errorMessage))
        return false;

    *data = MeshSerializer::serialize(mesh);
    if (data->isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("La sérialisation du mesh a échoué");
        return false;
    }
    return true;
}

bool MeshCache::ensure(Entity &entity, QString *errorMessage) const
{
    if (!entity.hasMesh())
        return true;

    entity.setMeshUrl(url(entity));
    const QString dir = directory(entity);
    const QString path = QDir(dir).filePath(QStringLiteral("mesh.bin"));

    if (QFileInfo::exists(path))
        return true;
    if (entity.triangles().isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Le WKB du mesh %1/%2 est vide")
                                .arg(entity.cacheType(), entity.code());
        return false;
    }
    if (!QDir().mkpath(dir)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Impossible de créer %1").arg(dir);
        return false;
    }

    QVector<QVector<Point2D>> triangles;
    WkbReader reader(entity.triangles());
    if (!reader.readTriangles(&triangles, errorMessage))
        return false;

    Mesh mesh;
    if (!mesh.build(triangles, errorMessage))
        return false;

    return MeshSerializer::save(mesh, path, errorMessage);
}
