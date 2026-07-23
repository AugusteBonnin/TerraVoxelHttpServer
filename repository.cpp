#include "repository.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariant>
namespace {
QString env(const char* n,const QString& fallback){ const QString v=qEnvironmentVariable(n); return v.isEmpty()?fallback:v; }
int dbPort(){ bool ok=false; int p=qEnvironmentVariableIntValue("TERRAVOXEL_DB_PORT",&ok); return ok&&p>0&&p<=65535?p:5432; }
}
Repository::Repository():m_connectionName(QStringLiteral("terravoxel-")+QUuid::createUuid().toString(QUuid::WithoutBraces)),m_database(QSqlDatabase::addDatabase(QStringLiteral("QPSQL"),m_connectionName)){
 m_database.setHostName(env("TERRAVOXEL_DB_HOST",QStringLiteral("127.0.0.1"))); m_database.setPort(dbPort());
 m_database.setDatabaseName(env("TERRAVOXEL_DB_NAME",QStringLiteral("geodb"))); m_database.setUserName(env("TERRAVOXEL_DB_USER",QStringLiteral("geo")));
 m_database.setPassword(qEnvironmentVariable("TERRAVOXEL_DB_PASSWORD"));
}
Repository::~Repository(){ close(); m_database=QSqlDatabase(); QSqlDatabase::removeDatabase(m_connectionName); }
bool Repository::open(QString* e){ if(m_database.isOpen()) return true; if(!m_database.open()){setError(e,m_database.lastError().text()); return false;} return true; }
void Repository::close(){ if(m_database.isOpen()) m_database.close(); }
bool Repository::isOpen() const{return m_database.isOpen();}
void Repository::setError(QString* e,const QString& m){if(e)*e=m;}
bool Repository::ensureOpen(QString* e) const{if(m_database.isOpen())return true; setError(e,QStringLiteral("La base PostgreSQL n'est pas ouverte")); return false;}
QString Repository::commonColumns(const QString&a,bool triangles){QString p=a.isEmpty()?QString():a+QLatin1Char('.'); QStringList c{p+QStringLiteral("code_insee"),p+QStringLiteral("nom_officiel"),p+QStringLiteral("nom_officiel_en_majuscules")}; if(triangles)c<<QStringLiteral("ST_AsBinary(%1triangles)").arg(p); c<<QStringLiteral("ST_XMin(%1rectangle)").arg(p)<<QStringLiteral("ST_YMin(%1rectangle)").arg(p)<<QStringLiteral("ST_XMax(%1rectangle)").arg(p)<<QStringLiteral("ST_YMax(%1rectangle)").arg(p)<<QStringLiteral("ST_XMin(%1carre)").arg(p)<<QStringLiteral("ST_YMin(%1carre)").arg(p)<<QStringLiteral("ST_XMax(%1carre)").arg(p)<<QStringLiteral("ST_YMax(%1carre)").arg(p); return c.join(QStringLiteral(", "));}
QString Repository::meshUrl(const Entity&e){return e.hasMesh()?QStringLiteral("/cache/%1/%2/mesh.bin").arg(e.cacheType(),e.code()):QString();}
QJsonObject Repository::childJson(const Entity&e){QJsonObject j=e.toJson(false); if(e.hasMesh())j.insert(QStringLiteral("mesh"),meshUrl(e)); return j;}
bool Repository::loadFrance(France*f,QString*e) const{if(!f||!ensureOpen(e))return false; f->setCode(QStringLiteral("FR"));f->setName(QStringLiteral("France"));f->setUpperName(QStringLiteral("FRANCE")); auto v=regions(e); if(e&&!e->isEmpty())return false; for(const auto&i:v)f->addChild(childJson(i)); return true;}
QVector<Region> Repository::regions(QString*e) const{QVector<Region> out;if(!ensureOpen(e))return out;QSqlQuery q(m_database);QString s=QStringLiteral("SELECT %1 FROM region r ORDER BY r.code_insee").arg(commonColumns(QStringLiteral("r"),true));if(!q.exec(s)){setError(e,q.lastError().text());return{};}while(q.next()){Region x;x.readCommonColumns(q);x.setMeshUrl(meshUrl(x));out<<x;}return out;}
bool Repository::region(const QString&code,Region*r,QString*e) const{if(!r||!ensureOpen(e))return false;QSqlQuery q(m_database);q.prepare(QStringLiteral("SELECT %1 FROM region r WHERE r.code_insee=:code").arg(commonColumns(QStringLiteral("r"),true)));q.bindValue(QStringLiteral(":code"),code);if(!q.exec()){setError(e,q.lastError().text());return false;}if(!q.next()){setError(e,QStringLiteral("Région introuvable : %1").arg(code));return false;}r->readCommonColumns(q);r->setMeshUrl(meshUrl(*r));auto c=departements(code,e);if(e&&!e->isEmpty())return false;for(const auto&i:c)r->addChild(childJson(i));return true;}
QVector<Departement> Repository::departements(const QString&parent,QString*e) const{QVector<Departement> out;if(!ensureOpen(e))return out;QSqlQuery q(m_database);q.prepare(QStringLiteral("SELECT %1 FROM departement d WHERE d.code_insee_de_la_region=:parent ORDER BY d.code_insee").arg(commonColumns(QStringLiteral("d"),true)));q.bindValue(QStringLiteral(":parent"),parent);if(!q.exec()){setError(e,q.lastError().text());return{};}while(q.next()){Departement x;x.readCommonColumns(q);x.setParentCode(parent);x.setMeshUrl(meshUrl(x));out<<x;}return out;}
bool Repository::departement(const QString&code,Departement*d,QString*e) const{if(!d||!ensureOpen(e))return false;QSqlQuery q(m_database);q.prepare(QStringLiteral("SELECT %1,d.code_insee_de_la_region FROM departement d WHERE d.code_insee=:code").arg(commonColumns(QStringLiteral("d"),true)));q.bindValue(QStringLiteral(":code"),code);if(!q.exec()){setError(e,q.lastError().text());return false;}if(!q.next()){setError(e,QStringLiteral("Département introuvable : %1").arg(code));return false;}d->readCommonColumns(q);d->setParentCode(q.value(12).toString());d->setMeshUrl(meshUrl(*d));auto c=epcis(code,e);if(e&&!e->isEmpty())return false;for(const auto&i:c)d->addChild(childJson(i));return true;}
QVector<Epci> Repository::epcis(const QString&dep,QString*e) const{QVector<Epci> out;if(!ensureOpen(e))return out;QSqlQuery q(m_database);q.prepare(QStringLiteral("SELECT DISTINCT e.code_siren,e.nom_officiel,e.nom_officiel_en_majuscules,ST_AsBinary(e.triangles),ST_XMin(e.rectangle),ST_YMin(e.rectangle),ST_XMax(e.rectangle),ST_YMax(e.rectangle),ST_XMin(e.carre),ST_YMin(e.carre),ST_XMax(e.carre),ST_YMax(e.carre) FROM epci e JOIN commune c ON e.code_siren=ANY(c.codes_siren_des_epci) WHERE c.code_insee_du_departement=:dep ORDER BY e.code_siren"));q.bindValue(QStringLiteral(":dep"),dep);if(!q.exec()){setError(e,q.lastError().text());return{};}while(q.next()){Epci x;x.readCommonColumns(q);x.setParentCode(dep);x.setMeshUrl(meshUrl(x));out<<x;}return out;}
bool Repository::epci(const QString&code,Epci*x,QString*e) const{if(!x||!ensureOpen(e))return false;QSqlQuery q(m_database);q.prepare(QStringLiteral("SELECT e.code_siren,e.nom_officiel,e.nom_officiel_en_majuscules,ST_AsBinary(e.triangles),ST_XMin(e.rectangle),ST_YMin(e.rectangle),ST_XMax(e.rectangle),ST_YMax(e.rectangle),ST_XMin(e.carre),ST_YMin(e.carre),ST_XMax(e.carre),ST_YMax(e.carre) FROM epci e WHERE e.code_siren=:code"));q.bindValue(QStringLiteral(":code"),code);if(!q.exec()){setError(e,q.lastError().text());return false;}if(!q.next()){setError(e,QStringLiteral("EPCI introuvable : %1").arg(code));return false;}x->readCommonColumns(q);x->setMeshUrl(meshUrl(*x));auto c=communesForEpci(code,e);if(e&&!e->isEmpty())return false;for(const auto&i:c)x->addChild(childJson(i));return true;}
QVector<Commune> Repository::communesForEpci(const QString&epci,QString*e) const{QVector<Commune> out;if(!ensureOpen(e))return out;QSqlQuery q(m_database);q.prepare(QStringLiteral("SELECT %1 FROM commune c WHERE :epci=ANY(c.codes_siren_des_epci) ORDER BY c.code_insee").arg(commonColumns(QStringLiteral("c"),true)));q.bindValue(QStringLiteral(":epci"),epci);if(!q.exec()){setError(e,q.lastError().text());return{};}while(q.next()){Commune x;x.readCommonColumns(q);x.setParentCode(epci);x.setMeshUrl(meshUrl(x));out<<x;}return out;}
bool Repository::commune(const QString&code,Commune*c,QString*e) const{if(!c||!ensureOpen(e))return false;QSqlQuery q(m_database);q.prepare(QStringLiteral("SELECT %1 FROM commune c WHERE c.code_insee=:code").arg(commonColumns(QStringLiteral("c"),true)));q.bindValue(QStringLiteral(":code"),code);if(!q.exec()){setError(e,q.lastError().text());return false;}if(!q.next()){setError(e,QStringLiteral("Commune introuvable : %1").arg(code));return false;}c->readCommonColumns(q);c->setMeshUrl(meshUrl(*c));return true;}

bool Repository::meshTable(const QString &type,
                           QString *tableName,
                           QString *codeColumn)
{
    if (!tableName || !codeColumn)
        return false;

    if (type == QStringLiteral("regions")) {
        *tableName = QStringLiteral("region");
        *codeColumn = QStringLiteral("code_insee");
        return true;
    }
    if (type == QStringLiteral("departements")) {
        *tableName = QStringLiteral("departement");
        *codeColumn = QStringLiteral("code_insee");
        return true;
    }
    if (type == QStringLiteral("epcis")) {
        *tableName = QStringLiteral("epci");
        *codeColumn = QStringLiteral("code_siren");
        return true;
    }
    if (type == QStringLiteral("communes")) {
        *tableName = QStringLiteral("commune");
        *codeColumn = QStringLiteral("code_insee");
        return true;
    }

    return false;
}

bool Repository::trianglesWkb(const QString &type,
                              const QString &code,
                              QByteArray *wkb,
                              bool *found,
                              QString *error) const
{
    if (wkb)
        wkb->clear();
    if (found)
        *found = false;

    if (!wkb || !found) {
        setError(error, QStringLiteral("Paramètre de sortie absent"));
        return false;
    }
    if (!ensureOpen(error))
        return false;

    QString tableName;
    QString codeColumn;
    if (!meshTable(type, &tableName, &codeColumn)) {
        setError(error, QStringLiteral("Type de mesh inconnu : %1").arg(type));
        return false;
    }

    // Les noms de table et de colonne viennent exclusivement de meshTable().
    // Le code utilisateur reste lié par un paramètre préparé.
    const QString sql = QStringLiteral(
        "SELECT ST_AsBinary(triangles) FROM %1 WHERE %2 = :code")
                            .arg(tableName, codeColumn);

    QSqlQuery query(m_database);
    if (!query.prepare(sql)) {
        setError(error, query.lastError().text());
        return false;
    }
    query.bindValue(QStringLiteral(":code"), code);

    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next())
        return true;

    *found = true;
    *wkb = query.value(0).toByteArray();
    return true;
}
