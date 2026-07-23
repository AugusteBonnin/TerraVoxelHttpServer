#ifndef FRANCE_H
#define FRANCE_H

#include "entity.h"

class France final : public Entity
{
public:
    France();

    QString cacheType() const override;
    bool hasMesh() const override;
};

#endif
