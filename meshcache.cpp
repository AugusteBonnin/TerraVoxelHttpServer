#include "meshcache.h"
#include "entity.h"
#include "mesh.h"
#include "meshserializer.h"
#include "wkbreader.h"

#include <QDir>
#include <QFileInfo>

#include <utility>

MeshCache::MeshCache(QString cacheRoot)
    : m_cacheRoot(std::move(cacheRoot))
{
}

QString MeshCache::directory(const Entity &entity) const
{
    return QDir(m_cacheRoot).filePath(entity.cacheType() + QLatin1Char('/') + entity.code());
}

QString MeshCache::url(const Entity &entity) const
{
    return QStringLiteral("/cache/%1/%2/mesh.bin")
        .arg(entity.cacheType(), entity.code());
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
