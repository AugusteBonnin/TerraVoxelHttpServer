#ifndef DEPARTEMENT_H
#define DEPARTEMENT_H

#include "entity.h"

class Departement final : public Entity
{
public:
    QString cacheType() const override;
};

#endif
