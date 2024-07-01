#include "bh.h"

Physics *_all_physics_objs[MAX_PHYSICS_OBJS];
Physics **ALL_PHYSICS_OBJS = _all_physics_objs;

s16 Physics_register(Physics *p) {
    for (int i = 0; i < MAX_PHYSICS_OBJS; ++i) {
        if (ALL_PHYSICS_OBJS[i] == NULL) {
            ALL_PHYSICS_OBJS[i] = p;
            return i;
        }
    }
    return -1;
}

Physics *Physics_new(
        Enc *e,
        const SpriteDefinition *spriteDef,
        u8 pal,
        fixx x,
        fixy y,
        bool bg_element
    ) {
    Physics *p = ct_calloc(1, sizeof(Physics));
    p->reg = Physics_register(p);
    if (p->reg < 0) {
        free(p);
        return NULL;
    }
    p->uid = random();
    p->x = x;
    p->y = y;
    p->spriteDef = spriteDef;
    p->pal = pal;
    p->h = p->spriteDef->h;
    p->w = p->spriteDef->w;
    p->center_offset_x = FIXX(p->w) >> 1;
    p->center_offset_y = FIXY(p->h) >> 1;
    if (bg_element) {
        p->bg_element = TRUE;
        p->bg_tile_idx = SpriteDefinition_VDP_idx(e->tm, spriteDef);
        p->start_x = x;
        p->start_y = y;
        Physics_bg_element_redraw(p, TRUE);
        p->n_tiles = (p->h >> 3) * (p->w >> 3);
    } else {
        p->sp = SPR_addSprite(
            spriteDef,
            0,
            0,
            TILE_ATTR(pal, TRUE, FALSE, FALSE)
            );
        if (!p->sp) {
            free(p);
            SPR_defragVRAM();
            return NULL;
        }
        SPR_setPriority(p->sp, TRUE);
        SPR_setVisibility(p->sp, HIDDEN);
    }
    p->dx = FIX16(0);
    p->dy = FIX16(0);
    p->ddx = FIX16(0);
    p->ddy = FIX16(0);
    p->f = LEFT;
    p->ttl = -1;
    p->col_x = p->x + p->center_offset_x;
    p->col_y = p->y + p->center_offset_y;
    p->iframes = 0;
    p->grav_model = FALSE;
    p->frames_alive = 0;
    p->terminal_velocity_up = FIX16(10);
    p->terminal_velocity_down = FIX16(8);
    return p;
}

void Physics_del(Physics *p, Enc *e) {

    if (p->partner) p->partner->partner = NULL;
    if (p->sp) SPR_releaseSprite(p->sp);
    if (p->pl) p->pl->p = NULL;
    if (p->instance_counter && (*p->instance_counter > 0)) --(*p->instance_counter);
    if (e->cannon == p) e->cannon = NULL;
    if (p->bg_element) {
        VDP_clearTileMapRect(
            BG_B,
            fixxToInt(p->start_x) / 8,
            fixyToInt(p->start_y) / 8,
            p->w / 8,
            p->h / 8
            );
    }
    ALL_PHYSICS_OBJS[p->reg] = NULL;
    free(p);
}

void Physics_del_all(Enc *e) {
    for (u8 i = 0; i < MAX_PHYSICS_OBJS; ++i) {
        Physics *p = ALL_PHYSICS_OBJS[i];
        if (p) Physics_del(p, e);
    }
}

void Physics_update(Encounter *e, Physics *p) {

    if (!p) return;

    if (e->state != ENC_RUNNING) return;

    if (p->ttl == 0) {
        Physics_del(p, e);
        return;
    }

    if (p->ttl > 0 && (!p->pl)) {
        --p->ttl;
    }

    ++p->state_frames;
    ++p->frames_alive;

    if (p->bg_element) {
        p->x = p->start_x + FIXX(e->bg->invader_x);
        p->y = p->start_y - FIXY(e->bg->invader_y);

        // for dx and dy, we're just tracking the direction. If we were
        // using them for something else, we'd have to convert to fixed
        p->dx = e->bg->invader_dx;
        p->dy = e->bg->invader_dy;

        behave(e, p);
        return;
    }

    if (p->push_frames > 0) {
        --p->push_frames;
    }

    if (p->grav_model) p->ddy = GRAVITY;

    p->dx += p->ddx;
    p->dy += p->ddy;

    if (p->grav_model) {
        if (p->dy >= p->terminal_velocity_down) p->dy = p->terminal_velocity_down;
        if (p->dy <= -p->terminal_velocity_up) p->dy = -p->terminal_velocity_up;
        if (p->dx <= -p->terminal_velocity_down) p->dx = -p->terminal_velocity_down;
        if (p->dx >= p->terminal_velocity_down) p->dx = p->terminal_velocity_down;
    }

    if (p->collision) {
        fixy h = FIXY(p->h);
        p->blocked = FALSE;
        fixy ytop = BG_collide(e->bg, p->col_x, p->y + h + p->dy + p->y_offset);
        if (ytop) {
            p->blocked = TRUE;
            p->dy = 0;
            p->y = ytop - h - p->y_offset;
        }
    }

    if (abs(p->dx) < FIX16(0.1)) {
        p->dx = 0;
    }
    
    p->x += fix16ToFixx(p->dx);
    p->y += fix16ToFixy(p->dy);

    behave(e, p);

    if (p->y >= FIXY(224)) {
        Physics_del(p, e);
        if (p->what == WHAT_ALIEN) {
            XGM_startPlayPCMNextCh(SND_SAMPLE_ALIEN_FALL, 0);
        }
        return;
    }

    p->col_x = p->x + p->center_offset_x;
    p->col_y = p->y + p->center_offset_y;


    if (p->sp) {
        SPR_setVisibility(p->sp, VISIBLE);
        SPR_setPosition(p->sp, fixxToRoundedInt(p->x - BG_x(e->bg)), fixyToRoundedInt(p->y - BG_y(e->bg)));
    }
}

u32 Physics_dist(Physics *p1, Physics *p2) {
    s16 dx = fixxToRoundedInt(p1->col_x) - fixxToRoundedInt(p2->col_x);
    s16 dy = fixyToRoundedInt(p1->col_y) - fixyToRoundedInt(p2->col_y);
    u32 dist = dx * dx + dy * dy;

    return dist;
}

bool collision_box(Phy *p1, Phy *p2) {
    fixx p1x = p1->x + fix16ToFixx(p1->dx);
    fixx p2x = p2->x + fix16ToFixx(p2->dx);
    fixy p1y = p1->y + p1->y_offset + fix16ToFixy(p1->dy);
    fixy p2y = p2->y + p2->y_offset + fix16ToFixy(p2->dy);
    return (
        p1x < p2x + FIXX(p2->w) &&
        p1x + FIXX(p1->w) > p2->x &&
        p1y < p2y + FIXY(p2->h) &&
        p1y + FIXY(p1->h) > p2->y
        );
}

void Physics_update_all(Encounter *enc) {
    for (int i = 0; i < MAX_PHYSICS_OBJS; ++i) {
        Physics_update(enc, ALL_PHYSICS_OBJS[i]);
    }

    if (enc->state != ENC_RUNNING) return;

    fixy interval = FIXY(80);
    fixy ymin = FIXY(0);
    fixy ymax = interval;
    while (ymin < FIXY(224)) {
        Phy *phys_in_range[MAX_PHYSICS_OBJS];
        u8 n_phys_in_range = 0;

        for (u8 i = 0; i < MAX_PHYSICS_OBJS; ++i) {
            Phy *p = ALL_PHYSICS_OBJS[i];
            if (!p) continue;
            if (!p->collision) continue;
            if (p->iframes > 0) continue;
            if (p->y < ymin || p->y >= ymax) continue;
            phys_in_range[n_phys_in_range] = p;
            ++n_phys_in_range;
        }
        for (u8 i = 0; i < n_phys_in_range; ++i) {
            Phy *pi = phys_in_range[i];
            if (!pi->calc_collisions) continue;
            for (u8 j = 0; j < n_phys_in_range; ++j) {
                if (i == j) continue;
                Phy *pj = phys_in_range[j];
                if (!collision_box(pi, pj)) continue;

                if (interact(enc, pi, pj)) continue;
                // interact was false, so handling isn't complete. execute bounce

                // don't let objects actually get entangled
                pi->x -= pi->dx;
                pj->x -= pj->dx;
                pi->y -= pi->dy;
                pj->y -= pj->dy;

                if ((!pi->bouncy) && (!pj->bouncy)) continue;
                // Nobody is moving anywhere. Don't bother calculating

                if (pi->mass == 0 || pj->mass == 0) continue;
                // interaction doesn't make sense without mass

                fix16 total_mass, diff_mass, rat_mass;
                if (pi->bouncy && pj->bouncy) {
                    total_mass = pi->mass + pj->mass;
                    diff_mass = pi->mass - pj->mass;
                    rat_mass = fix16Div(diff_mass, total_mass);
                    pi->dx = fix16Mul(rat_mass, pi->dx) + fix16Mul(fix16Div(pj->mass << 1, total_mass), pj->dx);
                    pi->dy = fix16Mul(rat_mass, pi->dy) + fix16Mul(fix16Div(pj->mass << 1, total_mass), pj->dy);
                    pj->dx = fix16Mul(rat_mass, pj->dx) + fix16Mul(fix16Div(pi->mass << 1, total_mass), pi->dx);
                    pj->dy = fix16Mul(rat_mass, pj->dy) + fix16Mul(fix16Div(pi->mass << 1, total_mass), pi->dy);
                } else if (pi->bouncy) {
                    fixx new_x = pi->x + fix16ToFixx(pi->dx);
                    fixy new_y = pi->y + fix16ToFixy(pi->dy);
                    if (new_x + FIXX(pi->w) >= pj->x && new_x <= pj->x + FIXX(pj->w)) {
                        pi->dx = -pi->dx;
                    }
                    if (new_y + FIXY(pi->h) >= pj->y && new_y <= pj->y + FIXY(pj->h)) {
                        pi->dy = -pi->dy;
                    }
                } else { // only pj->bouncy
                    fix16 new_x = pj->x + fix16ToFixx(pj->dx);
                    fix32 new_y = pj->y + fix16ToFixy(pj->dy);
                    if (new_x + FIXX(pj->w) >= pi->x && new_x <= pi->x + FIXX(pi->w)) {
                        pj->dx = -pj->dx;
                    }
                    if (new_y + FIXY(pj->h) >= pi->y && new_y <= pi->y + FIXY(pi->h)) {
                        pj->dy = -pj->dy;
                    }
                }
            }
        }
        ymin = ymax - FIXY(32);
        ymax = ymin + interval;
    }
}

void Physics_bg_element_redraw(Physics *p, bool hflip) {
    if (!p->bg_element) return;
    VDP_fillTileMapRectIncT(
        BG_B,
        TILE_ATTR_FULL(p->pal, TRUE, FALSE, hflip, p->bg_tile_idx + p->anim_no * p->n_tiles),
        fixxToInt(p->start_x) / 8,
        fixyToInt(p->start_y) / 8,
        p->w / 8,
        p->h / 8
        );
}
