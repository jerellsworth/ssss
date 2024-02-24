#ifndef PHYSICS_INIT_H
#define PHYSICS_INIT_H

#include "bh.h"

enum Thing_e {
    WHAT_PROP, // NOTE: PROP should always be first
    WHAT_CANNON,
    WHAT_GUY,
    WHAT_ALIEN,
    WHAT_HORSE,
    WHAT_BULLET,
    WHAT_PARTICLE
};

Phy *Physics_new_guy(Enc *e, const SpriteDefinition *spriteDef, fixx x, fixy y);
Phy *Physics_new_cannon(Enc *e, fixx x, fixy y, u8 *counter);
Phy *Physics_new_bullet(Enc *e, fixx x, fixy y, fix16 dx, fix16 dy);
Phy *Physics_new_alien(Enc *e, fixx x, fixy y, fix16 dx, fix16 dy, u8 *counter);
Phy *Physics_new_horse(Enc *e, fixx x, fixy y, u8 *counter);
Phy *Physics_new_particle(Enc *e, fixx x, fixy y);
Phy *Physics_new_bullet_particle(Enc *e, fixx x, fixy y);
Phy *Physics_new_star(Enc *e, fixx x, fixx y);

#endif
