#include "httpserver.h"

#include "tilecoveragecalculator.h"
#include "tilepyramid.h"
#include "tileset.h"

#include <QDateTime>
#include <QHostAddress>
#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

namespace {

QString cacheRoot()
{
    const QString configured = qEnvironmentVariable("TERRAVOXEL_CACHE_ROOT");
    return configured.isEmpty()
        ? QStringLiteral("/var/www/terravoxel/cache")
        : configured;
}

bool isKnownMeshType(const QString &type)
{
    return type == QStringLiteral("regions")
        || type == QStringLiteral("departements")
        || type == QStringLiteral("epci")
        || type == QStringLiteral("epcis")
        || type == QStringLiteral("communes");
}

QString repositoryMeshType(const QString &type)
{
    return type == QStringLiteral("epci") ? QStringLiteral("epcis") : type;
}

QByteArray mimeType(const QString &assetName)
{
    if (assetName == QStringLiteral("ortho.jpg"))
        return QByteArrayLiteral("image/jpeg");
    return QByteArrayLiteral("application/octet-stream");
}

} // namespace

HttpServer::HttpServer(QObject *parent)
    : QObject(parent),
      m_meshCache(cacheRoot()),
      m_tileCache(cacheRoot())
{
    configureRoutes();
}

bool HttpServer::start(quint16 port, QString *error)
{
    if (!m_repository.open(error))
        return false;

    if (!m_tcpServer.listen(QHostAddress::Any, port)) {
        if (error)
            *error = m_tcpServer.errorString();
        return false;
    }

    if (!m_httpServer.bind(&m_tcpServer)) {
        if (error)
            *error = QStringLiteral("Impossible d'associer QHttpServer au serveur TCP");
        m_tcpServer.close();
        return false;
    }
    return true;
}

quint16 HttpServer::port() const
{
    return m_tcpServer.serverPort();
}

void HttpServer::configureRoutes()
{
    m_httpServer.route(QStringLiteral("/health"), QHttpServerRequest::Method::Get,
                       [this](const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           if (!checkRateLimit(request, responder))
                               return;
                           responder.sendResponse(QHttpServerResponse(
                               QByteArrayLiteral("text/plain; charset=utf-8"),
                               QByteArrayLiteral("OK\n")));
                       });

    m_httpServer.route(QStringLiteral("/api/france"), QHttpServerRequest::Method::Get,
                       [this](const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleFranceRequest(request, responder);
                       });

    m_httpServer.route(QStringLiteral("/api/r/<arg>"), QHttpServerRequest::Method::Get,
                       [this](const QString &code, const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleRegionRequest(request, responder, code);
                       });

    m_httpServer.route(QStringLiteral("/api/d/<arg>"), QHttpServerRequest::Method::Get,
                       [this](const QString &code, const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleDepartementRequest(request, responder, code);
                       });

    m_httpServer.route(QStringLiteral("/api/e/<arg>"), QHttpServerRequest::Method::Get,
                       [this](const QString &code, const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleEpciRequest(request, responder, code);
                       });

    m_httpServer.route(QStringLiteral("/api/c/<arg>"), QHttpServerRequest::Method::Get,
                       [this](const QString &code, const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleCommuneRequest(request, responder, code);
                       });

    m_httpServer.route(QStringLiteral("/cache/<arg>/<arg>/mesh.bin"),
                       QHttpServerRequest::Method::Get,
                       [this](const QString &type, const QString &code, const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleMeshRequest(request, responder, type, code);
                       });

    m_httpServer.route(QStringLiteral("/api/t/<arg>/<arg>/<arg>"),
                       QHttpServerRequest::Method::Get,
                       [this](const QString &sizeMeters, const QString &minX, const QString &minY, const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleTileRequest(request, responder, sizeMeters, minX, minY);
                       });

    m_httpServer.route(QStringLiteral("/tiles/<arg>/<arg>/<arg>/<arg>"),
                       QHttpServerRequest::Method::Get,
                       [this](const QString &sizeMeters, const QString &minX, const QString &minY, const QString &assetName, const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleTileAssetRequest(request, responder, sizeMeters, minX, minY, assetName);
                       });

    m_httpServer.route(QStringLiteral("/api/tiles/<arg>/<arg>/<arg>"),
                       QHttpServerRequest::Method::Get,
                       [this](const QString &type, const QString &code, const QString &sizeMeters, const QHttpServerRequest &request, QHttpServerResponder &responder) {
                           handleEntityTilesRequest(request, responder, type, code, sizeMeters);
                       });

    m_httpServer.setMissingHandler(
        this,
        [](const QHttpServerRequest &request, QHttpServerResponder &responder) {
            responder.write(
                QJsonDocument(QJsonObject{
                    {QStringLiteral("success"), false},
                    {QStringLiteral("error"), QStringLiteral("Route inconnue")},
                    {QStringLiteral("path"), request.url().path()}}),
                QHttpServerResponder::StatusCode::NotFound);
        });
}

bool HttpServer::checkRateLimit(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    QString error;
    int retryAfterSeconds = 0;
    if (!allowRequest(request, &error, &retryAfterSeconds)) {
        responder.write(
            QJsonDocument(QJsonObject{{QStringLiteral("success"), false},
                                      {QStringLiteral("error"), error},
                                      {QStringLiteral("retryAfterSeconds"), retryAfterSeconds}}),
            QHttpServerResponder::StatusCode::TooManyRequests);
        return false;
    }
    return true;
}

bool HttpServer::allowRequest(const QHttpServerRequest &request,
                              QString *error,
                              int *retryAfterSeconds)
{
    const int maxRequests = rateLimitRequestsPerWindow();
    const int windowSeconds = rateLimitWindowSeconds();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 windowMs = static_cast<qint64>(windowSeconds) * 1000;
    const QString key = clientKey(request);

    QMutexLocker locker(&m_rateLimitMutex);
    QList<qint64> &timestamps = m_rateLimitBuckets[key];

    auto it = timestamps.begin();
    while (it != timestamps.end()) {
        if (nowMs - *it > windowMs)
            it = timestamps.erase(it);
        else
            ++it;
    }

    if (timestamps.size() >= maxRequests) {
        if (error)
            *error = QStringLiteral("Trop de requêtes, veuillez réessayer plus tard");
        if (retryAfterSeconds && !timestamps.isEmpty()) {
            const qint64 oldest = timestamps.first();
            const qint64 remaining = windowMs - (nowMs - oldest);
            *retryAfterSeconds = qMax(1, static_cast<int>(remaining / 1000));
        }
        return false;
    }

    timestamps.append(nowMs);
    return true;
}

QString HttpServer::clientKey(const QHttpServerRequest &request) const
{
    const QHostAddress remoteAddress = request.remoteAddress();
    return remoteAddress.isNull() ? QStringLiteral("unknown") : remoteAddress.toString();
}

int HttpServer::rateLimitRequestsPerWindow() const
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue("TERRAVOXEL_RATE_LIMIT_REQUESTS_PER_WINDOW", &ok);
    return ok && value > 0 ? value : 120;
}

int HttpServer::rateLimitWindowSeconds() const
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue("TERRAVOXEL_RATE_LIMIT_WINDOW_SECONDS", &ok);
    return ok && value > 0 ? value : 60;
}

// Request handlers with rate limiting

void HttpServer::handleFranceRequest(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if (!checkRateLimit(request, responder))
        return;

    France value;
    QString error;
    if (!m_repository.loadFrance(&value, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::InternalServerError));
        return;
    }
    responder.sendResponse(json(value.toJson()));
}

void HttpServer::handleRegionRequest(const QHttpServerRequest &request, QHttpServerResponder &responder, const QString &code)
{
    if (!checkRateLimit(request, responder))
        return;

    Region value;
    QString error;
    if (!m_repository.region(code, &value, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::NotFound));
        return;
    }
    responder.sendResponse(json(value.toJson()));
}

void HttpServer::handleDepartementRequest(const QHttpServerRequest &request, QHttpServerResponder &responder, const QString &code)
{
    if (!checkRateLimit(request, responder))
        return;

    Departement value;
    QString error;
    if (!m_repository.departement(code, &value, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::NotFound));
        return;
    }
    responder.sendResponse(json(value.toJson()));
}

void HttpServer::handleEpciRequest(const QHttpServerRequest &request, QHttpServerResponder &responder, const QString &code)
{
    if (!checkRateLimit(request, responder))
        return;

    Epci value;
    QString error;
    if (!m_repository.epci(code, &value, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::NotFound));
        return;
    }
    responder.sendResponse(json(value.toJson()));
}

void HttpServer::handleCommuneRequest(const QHttpServerRequest &request, QHttpServerResponder &responder, const QString &code)
{
    if (!checkRateLimit(request, responder))
        return;

    Commune value;
    QString error;
    if (!m_repository.commune(code, &value, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::NotFound));
        return;
    }
    responder.sendResponse(json(value.toJson()));
}

void HttpServer::handleMeshRequest(const QHttpServerRequest &request, QHttpServerResponder &responder, const QString &type, const QString &code)
{
    if (!checkRateLimit(request, responder))
        return;

    if (!isKnownMeshType(type)) {
        responder.sendResponse(failure(QStringLiteral("Type de mesh inconnu : %1").arg(type),
                                       QHttpServerResponder::StatusCode::NotFound));
        return;
    }

    QByteArray triangles;
    bool found = false;
    QString error;
    if (!m_repository.trianglesWkb(repositoryMeshType(type), code, &triangles, &found, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::InternalServerError));
        return;
    }
    if (!found || triangles.isEmpty()) {
        responder.sendResponse(failure(QStringLiteral("Mesh introuvable : %1/%2").arg(type, code),
                                       QHttpServerResponder::StatusCode::NotFound));
        return;
    }

    QByteArray data;
    if (!m_meshCache.meshData(type, code, triangles, &data, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::InternalServerError));
        return;
    }
    responder.sendResponse(QHttpServerResponse(QByteArrayLiteral("application/octet-stream"), data));
}

void HttpServer::handleTileRequest(const QHttpServerRequest &request, QHttpServerResponder &responder, const QString &sizeMeters, const QString &minX, const QString &minY)
{
    if (!checkRateLimit(request, responder))
        return;

    Tuile value;
    QString error;
    if (!parseTile(sizeMeters, minX, minY, &value, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::BadRequest));
        return;
    }
    responder.sendResponse(json(value.toJson(true)));
}

void HttpServer::handleTileAssetRequest(const QHttpServerRequest &request, QHttpServerResponder &responder, const QString &sizeMeters, const QString &minX, const QString &minY, const QString &assetName)
{
    if (!checkRateLimit(request, responder))
        return;

    Tuile value;
    QString error;
    if (!parseTile(sizeMeters, minX, minY, &value, &error)) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::BadRequest));
        return;
    }

    QByteArray data;
    bool success = false;
    if (assetName == QStringLiteral("mesh.bin"))
        success = m_tileCache.ensureFlatMesh(value, &data, &error);
    else if (assetName == QStringLiteral("ortho.jpg")
             || assetName == QStringLiteral("mnt.bin")
             || assetName == QStringLiteral("mnt.bil"))
        success = m_tileCache.ensureRaster(value, assetName, &data, &error);
    else {
        responder.sendResponse(failure(QStringLiteral("Ressource de tuile inconnue : %1").arg(assetName),
                                       QHttpServerResponder::StatusCode::NotFound));
        return;
    }

    if (!success) {
        responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::BadGateway));
        return;
    }
    responder.sendResponse(QHttpServerResponse(mimeType(assetName), data));
}

void HttpServer::handleEntityTilesRequest(const QHttpServerRequest &request, QHttpServerResponder &responder, const QString &type, const QString &code, const QString &sizeText)
{
    if (!checkRateLimit(request, responder))
        return;

    bool sizeOk = false;
    const qint64 sizeMeters = sizeText.toLongLong(&sizeOk);
    int level = 0;
    if (!sizeOk || !TilePyramid::levelForTileSize(sizeMeters, &level)) {
        responder.sendResponse(failure(QStringLiteral("Taille de tuile invalide"),
                                       QHttpServerResponder::StatusCode::BadRequest));
        return;
    }

    EntityBounds bounds;
    QString name;
    QString error;

    if (type == QStringLiteral("r") || type == QStringLiteral("regions")) {
        Region entity;
        if (!m_repository.region(code, &entity, &error)) {
            responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::NotFound));
            return;
        }
        bounds = entity.rectangle();
        name = entity.name();
    } else if (type == QStringLiteral("d") || type == QStringLiteral("departements")) {
        Departement entity;
        if (!m_repository.departement(code, &entity, &error)) {
            responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::NotFound));
            return;
        }
        bounds = entity.rectangle();
        name = entity.name();
    } else if (type == QStringLiteral("e") || type == QStringLiteral("epci") || type == QStringLiteral("epcis")) {
        Epci entity;
        if (!m_repository.epci(code, &entity, &error)) {
            responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::NotFound));
            return;
        }
        bounds = entity.rectangle();
        name = entity.name();
    } else if (type == QStringLiteral("c") || type == QStringLiteral("communes")) {
        Commune entity;
        if (!m_repository.commune(code, &entity, &error)) {
            responder.sendResponse(failure(error, QHttpServerResponder::StatusCode::NotFound));
            return;
        }
        bounds = entity.rectangle();
        name = entity.name();
    } else {
        responder.sendResponse(failure(QStringLiteral("Type d'entité inconnu : %1").arg(type),
                                       QHttpServerResponder::StatusCode::NotFound));
        return;
    }

    TileSet set;
    set.setId(QStringLiteral("%1-%2-%3m").arg(type, code).arg(sizeMeters));
    set.setName(name);
    set.setCoverage(TileCoverage::Rectangle);
    set.setLevel(level);
    set.setTiles(TileCoverageCalculator::rectangle(bounds, level));
    responder.sendResponse(json(set.toJson()));
}

// Helper methods

bool HttpServer::parseTile(const QString &sizeText,
                           const QString &minXText,
                           const QString &minYText,
                           Tuile *tile,
                           QString *error)
{
    bool sizeOk = false;
    bool minXOk = false;
    bool minYOk = false;
    const qint64 sizeMeters = sizeText.toLongLong(&sizeOk);
    const qint64 minX = minXText.toLongLong(&minXOk);
    const qint64 minY = minYText.toLongLong(&minYOk);

    if (!sizeOk || !minXOk || !minYOk) {
        if (error)
            *error = QStringLiteral("La taille, minX et minY doivent être des entiers en mètres Lambert-93");
        return false;
    }

    int level = 0;
    if (!TilePyramid::levelForTileSize(sizeMeters, &level)) {
        if (error)
            *error = QStringLiteral("Taille de tuile non prise en charge : %1 m").arg(sizeMeters);
        return false;
    }

    const Tuile candidate(level, minX, minY);
    if (!candidate.isValid()) {
        if (error)
            *error = QStringLiteral("Coordonnées minX/minY non alignées sur la grille de %1 m")
                         .arg(sizeMeters);
        return false;
    }
    if (tile)
        *tile = candidate;
    return true;
}

QHttpServerResponse HttpServer::json(const QJsonObject &object,
                                     QHttpServerResponder::StatusCode status)
{
    return QHttpServerResponse(
        QByteArrayLiteral("application/json; charset=utf-8"),
        QJsonDocument(object).toJson(QJsonDocument::Compact),
        status);
}

QHttpServerResponse HttpServer::failure(const QString &error,
                                        QHttpServerResponder::StatusCode status)
{
    return json(QJsonObject{{QStringLiteral("success"), false},
                            {QStringLiteral("error"), error}},
                status);
}
