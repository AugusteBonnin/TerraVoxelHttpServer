#include "httpserver.h"

#include "tilecoveragecalculator.h"
#include "tilepyramid.h"
#include "tileset.h"

#include <QHostAddress>
#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QJsonDocument>
#include <QJsonObject>

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
                       [] {
                           return QHttpServerResponse(
                               QByteArrayLiteral("text/plain; charset=utf-8"),
                               QByteArrayLiteral("OK\n"));
                       });

    m_httpServer.route(QStringLiteral("/api/france"), QHttpServerRequest::Method::Get,
                       [this] { return france(); });
    m_httpServer.route(QStringLiteral("/api/r/<arg>"), QHttpServerRequest::Method::Get,
                       [this](const QString &code) { return region(code); });
    m_httpServer.route(QStringLiteral("/api/d/<arg>"), QHttpServerRequest::Method::Get,
                       [this](const QString &code) { return departement(code); });
    m_httpServer.route(QStringLiteral("/api/e/<arg>"), QHttpServerRequest::Method::Get,
                       [this](const QString &code) { return epci(code); });
    m_httpServer.route(QStringLiteral("/api/c/<arg>"), QHttpServerRequest::Method::Get,
                       [this](const QString &code) { return commune(code); });

    m_httpServer.route(QStringLiteral("/cache/<arg>/<arg>/mesh.bin"),
                       QHttpServerRequest::Method::Get,
                       [this](const QString &type, const QString &code) {
                           return mesh(type, code);
                       });

    m_httpServer.route(QStringLiteral("/api/t/<arg>/<arg>/<arg>"),
                       QHttpServerRequest::Method::Get,
                       [this](const QString &sizeMeters, const QString &minX, const QString &minY) {
                           return tile(sizeMeters, minX, minY);
                       });

    m_httpServer.route(QStringLiteral("/tiles/<arg>/<arg>/<arg>/<arg>"),
                       QHttpServerRequest::Method::Get,
                       [this](const QString &sizeMeters,
                              const QString &minX,
                              const QString &minY,
                              const QString &assetName) {
                           return tileAsset(sizeMeters, minX, minY, assetName);
                       });

    m_httpServer.route(QStringLiteral("/api/tiles/<arg>/<arg>/<arg>"),
                       QHttpServerRequest::Method::Get,
                       [this](const QString &type,
                              const QString &code,
                              const QString &sizeMeters) {
                           return entityTiles(type, code, sizeMeters);
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

QHttpServerResponse HttpServer::france()
{
    France value;
    QString error;
    if (!m_repository.loadFrance(&value, &error))
        return failure(error, QHttpServerResponder::StatusCode::InternalServerError);
    return json(value.toJson());
}

QHttpServerResponse HttpServer::region(const QString &code)
{
    Region value;
    QString error;
    if (!m_repository.region(code, &value, &error))
        return failure(error, QHttpServerResponder::StatusCode::NotFound);
    return json(value.toJson());
}

QHttpServerResponse HttpServer::departement(const QString &code)
{
    Departement value;
    QString error;
    if (!m_repository.departement(code, &value, &error))
        return failure(error, QHttpServerResponder::StatusCode::NotFound);
    return json(value.toJson());
}

QHttpServerResponse HttpServer::epci(const QString &code)
{
    Epci value;
    QString error;
    if (!m_repository.epci(code, &value, &error))
        return failure(error, QHttpServerResponder::StatusCode::NotFound);
    return json(value.toJson());
}

QHttpServerResponse HttpServer::commune(const QString &code)
{
    Commune value;
    QString error;
    if (!m_repository.commune(code, &value, &error))
        return failure(error, QHttpServerResponder::StatusCode::NotFound);
    return json(value.toJson());
}

QHttpServerResponse HttpServer::mesh(const QString &type, const QString &code)
{
    if (!isKnownMeshType(type))
        return failure(QStringLiteral("Type de mesh inconnu : %1").arg(type),
                       QHttpServerResponder::StatusCode::NotFound);

    QByteArray triangles;
    bool found = false;
    QString error;
    if (!m_repository.trianglesWkb(repositoryMeshType(type), code, &triangles, &found, &error))
        return failure(error, QHttpServerResponder::StatusCode::InternalServerError);
    if (!found || triangles.isEmpty())
        return failure(QStringLiteral("Mesh introuvable : %1/%2").arg(type, code),
                       QHttpServerResponder::StatusCode::NotFound);

    QByteArray data;
    if (!m_meshCache.meshData(type, code, triangles, &data, &error))
        return failure(error, QHttpServerResponder::StatusCode::InternalServerError);
    return QHttpServerResponse(QByteArrayLiteral("application/octet-stream"), data);
}

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

    // L'index public et le modèle interne désignent tous deux le coin
    // inférieur gauche : (minX, minY).
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

QHttpServerResponse HttpServer::tile(const QString &sizeMeters,
                                     const QString &minX,
                                     const QString &minY)
{
    Tuile value;
    QString error;
    if (!parseTile(sizeMeters, minX, minY, &value, &error))
        return failure(error, QHttpServerResponder::StatusCode::BadRequest);
    return json(value.toJson(true));
}

QHttpServerResponse HttpServer::tileAsset(const QString &sizeMeters,
                                          const QString &minX,
                                          const QString &minY,
                                          const QString &assetName)
{
    Tuile value;
    QString error;
    if (!parseTile(sizeMeters, minX, minY, &value, &error))
        return failure(error, QHttpServerResponder::StatusCode::BadRequest);

    QByteArray data;
    bool success = false;
    if (assetName == QStringLiteral("mesh.bin"))
        success = m_tileCache.ensureFlatMesh(value, &data, &error);
    else if (assetName == QStringLiteral("ortho.jpg")
             || assetName == QStringLiteral("mnt.bin")
             || assetName == QStringLiteral("mnt.bil"))
        success = m_tileCache.ensureRaster(value, assetName, &data, &error);
    else
        return failure(QStringLiteral("Ressource de tuile inconnue : %1").arg(assetName),
                       QHttpServerResponder::StatusCode::NotFound);

    if (!success)
        return failure(error, QHttpServerResponder::StatusCode::BadGateway);
    return QHttpServerResponse(mimeType(assetName), data);
}

QHttpServerResponse HttpServer::entityTiles(const QString &type,
                                            const QString &code,
                                            const QString &sizeText)
{
    bool sizeOk = false;
    const qint64 sizeMeters = sizeText.toLongLong(&sizeOk);
    int level = 0;
    if (!sizeOk || !TilePyramid::levelForTileSize(sizeMeters, &level))
        return failure(QStringLiteral("Taille de tuile invalide"),
                       QHttpServerResponder::StatusCode::BadRequest);

    EntityBounds bounds;
    QString name;
    QString error;

    if (type == QStringLiteral("r") || type == QStringLiteral("regions")) {
        Region entity;
        if (!m_repository.region(code, &entity, &error))
            return failure(error, QHttpServerResponder::StatusCode::NotFound);
        bounds = entity.rectangle();
        name = entity.name();
    } else if (type == QStringLiteral("d") || type == QStringLiteral("departements")) {
        Departement entity;
        if (!m_repository.departement(code, &entity, &error))
            return failure(error, QHttpServerResponder::StatusCode::NotFound);
        bounds = entity.rectangle();
        name = entity.name();
    } else if (type == QStringLiteral("e") || type == QStringLiteral("epci") || type == QStringLiteral("epcis")) {
        Epci entity;
        if (!m_repository.epci(code, &entity, &error))
            return failure(error, QHttpServerResponder::StatusCode::NotFound);
        bounds = entity.rectangle();
        name = entity.name();
    } else if (type == QStringLiteral("c") || type == QStringLiteral("communes")) {
        Commune entity;
        if (!m_repository.commune(code, &entity, &error))
            return failure(error, QHttpServerResponder::StatusCode::NotFound);
        bounds = entity.rectangle();
        name = entity.name();
    } else {
        return failure(QStringLiteral("Type d'entité inconnu : %1").arg(type),
                       QHttpServerResponder::StatusCode::NotFound);
    }

    TileSet set;
    set.setId(QStringLiteral("%1-%2-%3m").arg(type, code).arg(sizeMeters));
    set.setName(name);
    set.setCoverage(TileCoverage::Rectangle);
    set.setLevel(level);
    set.setTiles(TileCoverageCalculator::rectangle(bounds, level));
    return json(set.toJson());
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
