#ifndef REGION_H
#define REGION_H

#include "entity.h"

class Region final : public Entity
{
public:
    QString cacheType() const override;
};

#endif
