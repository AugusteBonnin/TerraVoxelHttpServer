#ifndef TILECACHE_H
#define TILECACHE_H

#include "tuile.h"

#include <QString>

class TileCache
{
public:
    explicit TileCache(QString cacheRoot = QStringLiteral("/var/www/terravoxel/cache"));

    QString directory(const Tuile &tile) const;
    QString assetPath(const Tuile &tile, const QString &assetName) const;

    bool ensureFlatMesh(const Tuile &tile,
                        QString *path,
                        QString *errorMessage = nullptr) const;

    bool ensureRaster(const Tuile &tile,
                      const QString &assetName,
                      QString *path,
                      QString *errorMessage = nullptr) const;

private:
    bool downloadRaster(const Tuile &tile,
                        const QString &assetName,
                        const QString &path,
                        QString *errorMessage) const;
    static bool validAssetName(const QString &assetName);

    QString m_cacheRoot;
};

#endif
