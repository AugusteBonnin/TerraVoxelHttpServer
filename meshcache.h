#ifndef MESHCACHE_H
#define MESHCACHE_H

#include <QByteArray>
#include <QString>

class Entity;

class MeshCache
{
public:
    explicit MeshCache(QString cacheRoot = QStringLiteral("/var/www/terravoxel/cache"));

    bool ensure(Entity &entity, QString *errorMessage = nullptr) const;

    bool meshData(const QString &type,
                  const QString &code,
                  const QByteArray &trianglesWkb,
                  QByteArray *data,
                  QString *errorMessage = nullptr) const;

    QString meshPath(const QString &type, const QString &code) const;

private:
    QString directory(const QString &type, const QString &code) const;
    QString directory(const Entity &entity) const;
    QString url(const Entity &entity) const;

    static bool validPathPart(const QString &value);

    QString m_cacheRoot;
};

#endif
