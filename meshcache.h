#ifndef MESHCACHE_H
#define MESHCACHE_H

#include <QByteArray>
#include <QString>

class MeshCache
{
public:
    explicit MeshCache(QString cacheRoot = QStringLiteral("/var/www/terravoxel/cache"));

    bool load(const QString &type,
              const QString &code,
              QByteArray *data,
              bool *found,
              QString *errorMessage = nullptr) const;
    bool create(const QString &type,
                const QString &code,
                const QByteArray &trianglesWkb,
                QByteArray *data,
                QString *errorMessage = nullptr) const;

    QString meshPath(const QString &type, const QString &code) const;

private:
    QString directory(const QString &type, const QString &code) const;

    static bool validPathPart(const QString &value);

    QString m_cacheRoot;
};

#endif
