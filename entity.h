#ifndef ENTITY_H
#define ENTITY_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class QSqlQuery;

struct EntityBounds
{
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    bool valid = false;

    QJsonObject toJson() const;
    static EntityBounds fromQuery(const QSqlQuery &query, int firstColumn);
};

class Entity
{
public:
    virtual ~Entity() = default;

    virtual QString cacheType() const = 0;
    virtual bool hasMesh() const;

    QString code() const;
    void setCode(const QString &code);

    QString parentCode() const;
    void setParentCode(const QString &parentCode);

    QString name() const;
    void setName(const QString &name);

    QString upperName() const;
    void setUpperName(const QString &upperName);

    EntityBounds rectangle() const;
    void setRectangle(const EntityBounds &rectangle);

    EntityBounds square() const;
    void setSquare(const EntityBounds &square);

    QString meshUrl() const;
    void setMeshUrl(const QString &meshUrl);

    QString orthoUrl() const;
    void setOrthoUrl(const QString &orthoUrl);

    QString mntUrl() const;
    void setMntUrl(const QString &mntUrl);

    QJsonArray children() const;
    void setChildren(const QJsonArray &children);
    void addChild(const QJsonObject &child);

    void readCommonColumns(const QSqlQuery &query, int firstColumn = 0);

    virtual QJsonObject toJson(bool includeChildren = true) const;

private:
    QString m_code;
    QString m_parentCode;
    QString m_name;
    QString m_upperName;
    EntityBounds m_rectangle;
    EntityBounds m_square;
    QString m_meshUrl;
    QString m_orthoUrl;
    QString m_mntUrl;
    QJsonArray m_children;
};

#endif
