#include "bh.h"

Phy *Physics_new_guy(Enc *e, const SpriteDefinition *spriteDef, fixx x, fixy y) {
    Physics *p = Physics_new(e, spriteDef, PAL1, x, y, FALSE);
    if (!p) return NULL;
    p->what = WHAT_GUY;
    p->collision = TRUE;
    p->mass = 1;
    p->f = RIGHT;
    p->grav_model = TRUE;
    SPR_setAlwaysOnTop(p->sp);
    p->calc_collisions = FALSE;
    p->push_ready = TRUE;
    /*
    // possible hack so we don't accidentally go under the cannon
    p->y_offset = FIXY(-8);
    p->h = 16;
    */
    return p;
}

Phy *Physics_new_cannon(Enc *e, fixx x, fixy y, u8 *counter) {
    Physics *p = Physics_new(e, &SPR_CANNON, PAL1, x, y, FALSE);
    if (!p) return NULL;
    p->what = WHAT_CANNON;
    p->collision = TRUE;
    p->mass = 50;
    p->f = RIGHT;
    p->grav_model = TRUE;
    ++*counter;
    p->instance_counter = counter;
    e->cannon = p;
    p->calc_collisions = TRUE;
    SPR_setDepth(p->sp, SPR_MIN_DEPTH); // send to top
    return p;
}

Phy *Physics_new_bullet(Enc *e, fixx x, fixy y, fix16 dx, fix16 dy) {
    Physics *p = Physics_new(e, &SPR_BULLET, PAL1, x, y, FALSE);
    if (!p) return NULL;
    p->what = WHAT_BULLET;
    p->dx = dx;
    p->dy = dy;
    p->w = 4;
    p->h = 4;
    p->ttl = 90;
    p->collision = TRUE;
    p->calc_collisions = TRUE;
    return p;
}

Phy *Physics_new_alien(Enc *e, fixx x, fixy y, fix16 dx, fix16 dy, u8 *counter) {
    Physics *p = Physics_new(e, &SPR_ALIEN, PAL1, x, y, TRUE);
    if (!p) return NULL;
    p->what = WHAT_ALIEN;
    p->dx = dx;
    p->dy = dy;
    p->v = max(abs(dx), abs(dy));
    p->collision = TRUE;
    ++*counter;
    p->instance_counter = counter;
    p->state = 10;
    p->mass = FIX16(0.125);
    SPR_setHFlip(p->sp, TRUE);
    p->calc_collisions = FALSE;
    return p;
}

Phy *Physics_new_horse(Enc *e, fixx x, fixy y, u8 *counter) {
    Physics *p = Physics_new(e, &SPR_HORSE, PAL1, x, y, FALSE);
    if (!p) return NULL;
    p->what = WHAT_HORSE;
    p->dx = FIXX(-2);
    p->collision = TRUE;
    ++*counter;
    p->instance_counter = counter;
    p->ttl = 200;
    p->w = 28;
    //XGM_startPlayPCMNextCh(SND_SAMPLE_HORSE, 13);
    return p;
}

Phy *Physics_new_particle(Enc *e, fixx x, fixy y) {
    Physics *p = Physics_new(e, &SPR_PARTICLE, PAL1, x, y, FALSE);
    if (!p) return NULL;
    p->what = WHAT_PROP;
    p->dx = FIX16(1) - (FIX16(random_with_max(8)) >> 2);
    p->dy = FIX16(1) - (FIX16(random_with_max(8)) >> 2);
    p->grav_model = TRUE;
    p->collision = FALSE;
    SPR_setAnim(p->sp, random_with_max(3));
    p->ttl = 60;
    return p;
}

Phy *Physics_new_bullet_particle(Enc *e, fixx x, fixy y) {
    Physics *p = Physics_new(e, &SPR_PARTICLE, PAL1, x, y, FALSE);
    if (!p) return NULL;
    p->what = WHAT_PARTICLE;
    p->dx = FIX16(1) - (FIX16(random_with_max(8)) >> 2);
    p->dy = FIX16(1) - (FIX16(random_with_max(8)) >> 2);
    p->grav_model = TRUE;
    p->collision = TRUE;
    p->calc_collisions = TRUE;
    p->w = 2;
    p->h = 2;
    SPR_setAnim(p->sp, 4 + random_with_max(1));
    p->ttl = 60;
    return p;
}

Phy *Physics_new_star(Enc *e, fixx x, fixx y) {
    Physics *p = Physics_new(e, &SPR_STAR, PAL0, x, y, FALSE);
    if (!p) return NULL;
    p->what = WHAT_PROP;
    SPR_setAnim(p->sp, random_with_max(2));
    SPR_setPriority(p->sp, FALSE);
    for (u16 i = 0; i < random_with_max(20 * 4 - 1); ++i) SPR_nextFrame(p->sp);
    return p;
}
