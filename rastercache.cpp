#include "rastercache.h"
#include "entity.h"

#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <utility>

namespace {
QUrl wmsUrl(const QString &layer, const QString &format,
            const EntityBounds &b, int size)
{
    QUrl url(QStringLiteral("https://data.geopf.fr/wms-r/wms"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("SERVICE"), QStringLiteral("WMS"));
    q.addQueryItem(QStringLiteral("VERSION"), QStringLiteral("1.3.0"));
    q.addQueryItem(QStringLiteral("REQUEST"), QStringLiteral("GetMap"));
    q.addQueryItem(QStringLiteral("LAYERS"), layer);
    q.addQueryItem(QStringLiteral("STYLES"), QStringLiteral("normal"));
    q.addQueryItem(QStringLiteral("FORMAT"), format);
    q.addQueryItem(QStringLiteral("CRS"), QStringLiteral("EPSG:2154"));
    q.addQueryItem(QStringLiteral("BBOX"), QStringLiteral("%1,%2,%3,%4")
                   .arg(int(b.minX)).arg(int(b.minY)).arg(int(b.maxX)).arg(int(b.maxY)));
    q.addQueryItem(QStringLiteral("WIDTH"), QString::number(size));
    q.addQueryItem(QStringLiteral("HEIGHT"), QString::number(size));
    url.setQuery(q);
    return url;
}
}

RasterCache::RasterCache(QString cacheRoot)
    : m_cacheRoot(std::move(cacheRoot))
{
}

QString RasterCache::directory(const Entity &entity) const
{
    if (entity.code().isEmpty() || entity.cacheType() == QStringLiteral("france"))
        return QDir(m_cacheRoot).filePath(entity.cacheType());
    return QDir(m_cacheRoot).filePath(entity.cacheType() + QLatin1Char('/') + entity.code());
}

QString RasterCache::url(const Entity &entity, const QString &fileName) const
{
    if (entity.cacheType() == QStringLiteral("france"))
        return QStringLiteral("/cache/%1/%2").arg(entity.cacheType(), fileName);
    return QStringLiteral("/cache/%1/%2/%3").arg(entity.cacheType(), entity.code(), fileName);
}

bool RasterCache::download(const QUrl &url, const QString &path, QString *errorMessage) const
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("TerraVoxel/1.0"));
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, [&] { reply->abort(); loop.quit(); });
    timeout.start(120000);
    loop.exec();
    const bool timedOut = !timeout.isActive();
    timeout.stop();

    if (timedOut || reply->error() != QNetworkReply::NoError) {
        if (errorMessage)
            *errorMessage = timedOut ? QStringLiteral("Délai dépassé pour %1").arg(url.toString())
                                     : QStringLiteral("Erreur HTTP pour %1 : %2").arg(url.toString(), reply->errorString());
        reply->deleteLater();
        return false;
    }

    const QByteArray payload = reply->readAll();
    const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    reply->deleteLater();
    const QByteArray trimmed = payload.trimmed();
    if (payload.isEmpty() || contentType.contains(QStringLiteral("xml"), Qt::CaseInsensitive)
        || trimmed.startsWith("<?xml") || trimmed.startsWith("<ServiceException")) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Réponse WMS invalide : %1").arg(QString::fromUtf8(trimmed.left(1000)));
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(payload) != payload.size() || !file.commit()) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    return true;
}

bool RasterCache::ensure(Entity &entity, QString *errorMessage) const
{
    constexpr int size = 2048;
    const EntityBounds bounds = entity.square();
    if (!bounds.valid || !(bounds.maxX > bounds.minX) || !(bounds.maxY > bounds.minY)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Emprise raster invalide");
        return false;
    }

    const QString dir = directory(entity);
    if (!QDir().mkpath(dir)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Impossible de créer %1").arg(dir);
        return false;
    }

    const QString orthoPath = QDir(dir).filePath(QStringLiteral("ortho.jpg"));
    const QString mntPath = QDir(dir).filePath(QStringLiteral("mnt.bil"));
    if (!QFileInfo::exists(orthoPath)
        && !download(wmsUrl(QStringLiteral("ORTHOIMAGERY.ORTHOPHOTOS"), QStringLiteral("image/jpeg"), bounds, size), orthoPath, errorMessage))
        return false;
    if (!QFileInfo::exists(mntPath)
        && !download(wmsUrl(QStringLiteral("ELEVATION.ELEVATIONGRIDCOVERAGE.HIGHRES"), QStringLiteral("image/x-bil;bits=32"), bounds, size), mntPath, errorMessage))
        return false;

    entity.setOrthoUrl(url(entity, QStringLiteral("ortho.jpg")));
    entity.setMntUrl(url(entity, QStringLiteral("mnt.bil")));
    return true;
}
