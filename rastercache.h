#ifndef RASTERCACHE_H
#define RASTERCACHE_H

#include <QString>

class Entity;
class QUrl;

class RasterCache
{
public:
    explicit RasterCache(QString cacheRoot = QStringLiteral("/var/www/terravoxel/cache"));

    bool ensure(Entity &entity, QString *errorMessage = nullptr) const;

private:
    QString directory(const Entity &entity) const;
    QString url(const Entity &entity, const QString &fileName) const;
    bool download(const QUrl &url, const QString &path, QString *errorMessage) const;

    QString m_cacheRoot;
};

#endif
