#include "tilecache.h"

#include "mesh.h"
#include "meshserializer.h"
#include "tilepyramid.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
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

QUrl tileWmsUrl(const Tuile &tile, const QString &assetName)
{
    QString layer;
    QString format;
    int imageSize = 256;

    if (assetName == QStringLiteral("ortho.jpg")) {
        layer = QStringLiteral("ORTHOIMAGERY.ORTHOPHOTOS");
        format = QStringLiteral("image/jpeg");
    } else {
        layer = QStringLiteral("ELEVATION.ELEVATIONGRIDCOVERAGE.HIGHRES");
        format = QStringLiteral("image/x-bil;bits=32");
        imageSize = 101;
    }

    QUrl url(QStringLiteral("https://data.geopf.fr/wms-r/wms"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("SERVICE"), QStringLiteral("WMS"));
    query.addQueryItem(QStringLiteral("VERSION"), QStringLiteral("1.3.0"));
    query.addQueryItem(QStringLiteral("REQUEST"), QStringLiteral("GetMap"));
    query.addQueryItem(QStringLiteral("LAYERS"), layer);
    query.addQueryItem(QStringLiteral("STYLES"), QStringLiteral("normal"));
    query.addQueryItem(QStringLiteral("FORMAT"), format);
    query.addQueryItem(QStringLiteral("CRS"), QStringLiteral("EPSG:2154"));
    query.addQueryItem(QStringLiteral("BBOX"),
                       QStringLiteral("%1,%2,%3,%4")
                           .arg(tile.xmin())
                           .arg(tile.ymin())
                           .arg(tile.xmax())
                           .arg(tile.ymax()));
    query.addQueryItem(QStringLiteral("WIDTH"), QString::number(imageSize));
    query.addQueryItem(QStringLiteral("HEIGHT"), QString::number(imageSize));
    url.setQuery(query);
    return url;
}

} // namespace

TileCache::TileCache(QString cacheRoot)
    : m_cacheRoot(std::move(cacheRoot))
{
}

bool TileCache::validAssetName(const QString &assetName)
{
    return assetName == QStringLiteral("mesh.bin")
        || assetName == QStringLiteral("ortho.jpg")
        || assetName == QStringLiteral("mnt.bin")
        || assetName == QStringLiteral("mnt.bil");
}

QString TileCache::directory(const Tuile &tile) const
{
    return QDir(m_cacheRoot).filePath(TilePyramid::cacheDirectory(tile));
}

QString TileCache::assetPath(const Tuile &tile, const QString &assetName) const
{
    if (!tile.isValid() || !validAssetName(assetName))
        return {};

    QString canonicalName = assetName;
    if (canonicalName == QStringLiteral("mnt.bil"))
        canonicalName = QStringLiteral("mnt.bin");
    return QDir(directory(tile)).filePath(canonicalName);
}

bool TileCache::readAsset(const Tuile &tile,
                          const QString &assetName,
                          QByteArray *data,
                          QString *errorMessage) const
{
    if (!data) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Paramètre de sortie absent");
        return false;
    }
    data->clear();

    const QString path = assetPath(tile, assetName);
    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Tuile ou ressource invalide");
        return false;
    }

    QFile file(path);
    if (!file.exists())
        return false;
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    *data = file.readAll();
    return !data->isEmpty();
}

bool TileCache::ensureFlatMesh(const Tuile &tile,
                               QByteArray *data,
                               QString *errorMessage) const
{
    if (readAsset(tile, QStringLiteral("mesh.bin"), data, errorMessage))
        return true;

    QVector<QVector<Point2D>> triangles{
        {{double(tile.xmin()), double(tile.ymin())},
         {double(tile.xmax()), double(tile.ymin())},
         {double(tile.xmax()), double(tile.ymax())}},
        {{double(tile.xmin()), double(tile.ymin())},
         {double(tile.xmax()), double(tile.ymax())},
         {double(tile.xmin()), double(tile.ymax())}}
    };

    Mesh mesh;
    if (!mesh.build(triangles, errorMessage))
        return false;

    const QString dir = directory(tile);
    if (!QDir().mkpath(dir)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Impossible de créer %1").arg(dir);
        return false;
    }

    const QString path = assetPath(tile, QStringLiteral("mesh.bin"));
    if (!MeshSerializer::save(mesh, path, errorMessage))
        return false;

    *data = MeshSerializer::serialize(mesh);
    return !data->isEmpty();
}

bool TileCache::downloadRaster(const Tuile &tile,
                               const QString &assetName,
                               const QString &path,
                               QString *errorMessage) const
{
    QNetworkAccessManager manager;
    QNetworkRequest request(tileWmsUrl(tile, assetName));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("TerraVoxelHttpServer/1.0"));

    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, [&] {
        reply->abort();
        loop.quit();
    });
    timeout.start(30000);
    loop.exec();

    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkMessage = reply->errorString();
    const QByteArray payload = reply->readAll();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Téléchargement WMS impossible : %1")
                                .arg(networkMessage);
        return false;
    }

    const QByteArray trimmed = payload.trimmed();
    if (payload.isEmpty() || trimmed.startsWith('<')) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Réponse WMS vide ou invalide");
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(payload) != payload.size()
        || !file.commit()) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    return true;
}

bool TileCache::ensureRaster(const Tuile &tile,
                             const QString &assetName,
                             QByteArray *data,
                             QString *errorMessage) const
{
    if (assetName != QStringLiteral("ortho.jpg")
        && assetName != QStringLiteral("mnt.bin")
        && assetName != QStringLiteral("mnt.bil")) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Ressource raster inconnue");
        return false;
    }

    if (readAsset(tile, assetName, data, errorMessage))
        return true;

    const QString dir = directory(tile);
    if (!QDir().mkpath(dir)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Impossible de créer %1").arg(dir);
        return false;
    }

    const QString path = assetPath(tile, assetName);
    if (!downloadRaster(tile, assetName, path, errorMessage))
        return false;
    return readAsset(tile, assetName, data, errorMessage);
}
