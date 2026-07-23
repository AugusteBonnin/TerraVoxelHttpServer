#include "entity.h"

#include <QJsonValue>
#include <QSqlQuery>
#include <QVariant>

QJsonObject EntityBounds::toJson() const
{
    if (!valid)
        return {};

    return QJsonObject{
        {QStringLiteral("min"), QJsonObject{
             {QStringLiteral("x"), minX},
             {QStringLiteral("y"), minY}
         }},
        {QStringLiteral("max"), QJsonObject{
             {QStringLiteral("x"), maxX},
             {QStringLiteral("y"), maxY}
         }}
    };
}

EntityBounds EntityBounds::fromQuery(const QSqlQuery &query, int firstColumn)
{
    EntityBounds result;

    for (int i = 0; i < 4; ++i) {
        if (query.value(firstColumn + i).isNull())
            return result;
    }

    result.minX = query.value(firstColumn).toDouble();
    result.minY = query.value(firstColumn + 1).toDouble();
    result.maxX = query.value(firstColumn + 2).toDouble();
    result.maxY = query.value(firstColumn + 3).toDouble();
    result.valid = true;
    return result;
}

bool Entity::hasMesh() const { return true; }
QString Entity::code() const { return m_code; }
void Entity::setCode(const QString &value) { m_code = value; }
QString Entity::parentCode() const { return m_parentCode; }
void Entity::setParentCode(const QString &value) { m_parentCode = value; }
QString Entity::name() const { return m_name; }
void Entity::setName(const QString &value) { m_name = value; }
QString Entity::upperName() const { return m_upperName; }
void Entity::setUpperName(const QString &value) { m_upperName = value; }
EntityBounds Entity::rectangle() const { return m_rectangle; }
void Entity::setRectangle(const EntityBounds &value) { m_rectangle = value; }
EntityBounds Entity::square() const { return m_square; }
void Entity::setSquare(const EntityBounds &value) { m_square = value; }
QByteArray Entity::triangles() const { return m_triangles; }
void Entity::setTriangles(const QByteArray &value) { m_triangles = value; }
QString Entity::meshUrl() const { return m_meshUrl; }
void Entity::setMeshUrl(const QString &value) { m_meshUrl = value; }
QString Entity::orthoUrl() const { return m_orthoUrl; }
void Entity::setOrthoUrl(const QString &value) { m_orthoUrl = value; }
QString Entity::mntUrl() const { return m_mntUrl; }
void Entity::setMntUrl(const QString &value) { m_mntUrl = value; }
QJsonArray Entity::children() const { return m_children; }
void Entity::setChildren(const QJsonArray &value) { m_children = value; }
void Entity::addChild(const QJsonObject &value) { m_children.append(value); }

void Entity::readCommonColumns(const QSqlQuery &query,
                               int firstColumn,
                               bool withTriangles)
{
    setCode(query.value(firstColumn).toString());
    setName(query.value(firstColumn + 1).toString());
    setUpperName(query.value(firstColumn + 2).toString());

    const int geometryColumn = firstColumn + 3;
    if (withTriangles)
        setTriangles(query.value(geometryColumn).toByteArray());

    const int rectangleColumn = geometryColumn + (withTriangles ? 1 : 0);
    setRectangle(EntityBounds::fromQuery(query, rectangleColumn));
    setSquare(EntityBounds::fromQuery(query, rectangleColumn + 4));
}

QJsonObject Entity::toJson(bool includeChildren) const
{
    QJsonObject object{
        {QStringLiteral("code"), m_code},
        {QStringLiteral("nom"), m_name},
        {QStringLiteral("nomMaj"), m_upperName},
        {QStringLiteral("rectangle"), m_rectangle.toJson()},
        {QStringLiteral("carre"), m_square.toJson()}
    };

    if (!m_parentCode.isEmpty())
        object.insert(QStringLiteral("code_parent"), m_parentCode);
    if (!m_meshUrl.isEmpty())
        object.insert(QStringLiteral("mesh"), m_meshUrl);
    if (!m_orthoUrl.isEmpty())
        object.insert(QStringLiteral("ortho"), m_orthoUrl);
    if (!m_mntUrl.isEmpty())
        object.insert(QStringLiteral("mnt"), m_mntUrl);
    if (includeChildren)
        object.insert(QStringLiteral("enfants"), m_children);

    return object;
}
