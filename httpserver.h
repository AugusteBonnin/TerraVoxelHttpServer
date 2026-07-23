#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "meshcache.h"
#include "repository.h"
#include "tilecache.h"

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QObject>
#include <QTcpServer>
#include <QMutex>
#include <QList>
#include <QHash>
#include <functional>

class HttpServer final : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(QObject *parent = nullptr);

    bool start(quint16 port = 8080, QString *error = nullptr);
    quint16 port() const;

private:
    void configureRoutes();
    QHttpServerResponse withRateLimit(const QHttpServerRequest &request,
                                       const std::function<QHttpServerResponse()> &handler);
    bool allowRequest(const QHttpServerRequest &request,
                      QString *error,
                      int *retryAfterSeconds = nullptr);
    QString clientKey(const QHttpServerRequest &request) const;
    int rateLimitRequestsPerWindow() const;
    int rateLimitWindowSeconds() const;

    QHttpServerResponse france();
    QHttpServerResponse region(const QString &code);
    QHttpServerResponse departement(const QString &code);
    QHttpServerResponse epci(const QString &code);
    QHttpServerResponse commune(const QString &code);
    QHttpServerResponse mesh(const QString &type, const QString &code);

    QHttpServerResponse tile(const QString &sizeMeters,
                             const QString &minX,
                             const QString &minY);
    QHttpServerResponse tileAsset(const QString &sizeMeters,
                                  const QString &minX,
                                  const QString &minY,
                                  const QString &assetName);
    QHttpServerResponse entityTiles(const QString &type,
                                    const QString &code,
                                    const QString &sizeMeters);

    static bool parseTile(const QString &sizeMeters,
                          const QString &minX,
                          const QString &minY,
                          Tuile *tile,
                          QString *error);
    static QHttpServerResponse json(
        const QJsonObject &object,
        QHttpServerResponder::StatusCode status = QHttpServerResponder::StatusCode::Ok);
    static QHttpServerResponse failure(
        const QString &error,
        QHttpServerResponder::StatusCode status);

    Repository m_repository;
    MeshCache m_meshCache;
    TileCache m_tileCache;
    QHttpServer m_httpServer;
    QTcpServer m_tcpServer;
    mutable QMutex m_rateLimitMutex;
    QHash<QString, QList<qint64>> m_rateLimitBuckets;
};

#endif
