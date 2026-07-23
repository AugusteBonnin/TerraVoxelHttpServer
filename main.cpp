#include "httpserver.h"
#include <QCoreApplication>
#include <QDebug>
int main(int argc,char*argv[]){QCoreApplication app(argc,argv);bool ok=false;int p=qEnvironmentVariableIntValue("TERRAVOXEL_HTTP_PORT",&ok);quint16 port=ok&&p>0&&p<=65535?static_cast<quint16>(p):8080;HttpServer server;QString error;if(!server.start(port,&error)){qCritical().noquote()<<QStringLiteral("Démarrage impossible : %1").arg(error);return 1;}qInfo()<<"TerraVoxel port"<<server.port();return app.exec();}
