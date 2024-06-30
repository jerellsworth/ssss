#include "bh.h"

void behave(Encounter *e, Physics *p) {
    switch (p->what) {
        case WHAT_CANNON:
            if (p->blocked && p->state == 0) {
                p->state = 1;
                p->state_frames = 0;
                XGM_startPlayPCMNextCh(SND_SAMPLE_CANNON_FALL, 9);
                for (int i = 0; i < MAX_PHYSICS_OBJS; ++i) {
                    Phy *p2 = ALL_PHYSICS_OBJS[i];
                    if (
                        (p2) && 
                        (p2->what == WHAT_ALIEN) &&
                        (p2->state == 2)) {
                        fixx dx = p2->col_x - p->col_x;
                        if (abs(dx) <= FIXX(10)) {
                            p2->dx = dx < 0 ? FIXX(-8) : FIXX(8);
                            p2->dy = FIXX(-8);
                            p2->state = 1;
                            p2->state_frames = 0;
                        }
                    }
                }
            }
            p->dx = 0;
            if (p->blocked) {
                p->force_x += FIX16(e->bg->theta - 512) >> 5;
            }
            p->dx = p->force_x >> 2;

            p->force_x = 0;
            if (p->dx != 0) SPR_nextFrame(p->sp);

            if (!(p->frames_alive & 15)) {
                Physics_new_bullet(e, p->x + FIX16(6), p->y - FIX16(2), 0, FIX16(-3));
                //XGM_startPlayPCMNextCh(SND_SAMPLE_CANNON_SHOT, 7);
            }
            return;
        case WHAT_GUY:
            if (p->partner) {
                if (!collision_box(p, p->partner) && p->state_frames > 2) {
                    p->partner = NULL;
                }
            }
            switch (p->state) {
                case 0: // standard
                    break;
                case 1: // jumped
                    if (p->blocked) {
                        for (int i = 0; i < MAX_PHYSICS_OBJS; ++i) {
                            Phy *p2 = ALL_PHYSICS_OBJS[i];
                            if (
                                (p2) && 
                                (p2->what == WHAT_ALIEN) &&
                                (p2->state == 2)) {
                                fixx dx = p2->col_x - p->col_x;
                                if (abs(dx) <= FIXX(10)) {
                                    if (e->cannon) {
                                        fixx cannon_dx = p2->col_x - e->cannon->col_x;
                                        if (abs(cannon_dx) < FIXX(12)) {
                                            dx = cannon_dx;
                                        }
                                    }
                                    p2->dx = dx < 0 ? FIXX(-8) : FIXX(8);
                                    p2->dy = FIXX(-8);
                                    p2->state = 1;
                                    p2->state_frames = 0;
                                }
                            }
                        }
                        p->state = 0;
                        p->state_frames = 0;
                        if (p->pl) p->pl->cooldown = 10;
                        XGM_startPlayPCMNextCh(SND_SAMPLE_GUY_THUNK, 10);
                    }
                    break;
                default:
                    break;
            }
            return;
        case WHAT_ALIEN:
            switch (p->state) {
                case 0: // reserved
                    return;
                case 1: // falling
                    if (p->blocked) {
                        p->ttl = 600;
                        if (e->cannon) {
                            p->state = 2;
                            p->state_frames = 0;
                            if (e->cannon->x < p->x) {
                                p->dx = FIXX(-1);
                                SPR_setHFlip(p->sp, TRUE);
                            } else {
                                p->dx = FIXX(1);
                                SPR_setHFlip(p->sp, FALSE);
                            }
                        }
                    }
                    return;
                case 2: // pushing
                    if (p->x <= FIXX(10) || p->x >= FIXX(306)) {
                        p->state = 3;
                        p->state_frames = 0;
                        p->dx = 0;
                        return;
                    }
                    if (p->partner) {
                        if (!collision_box(p, p->partner) && p->state_frames > 2) {
                            p->partner = NULL;
                            return;
                        }
                        SPR_ensureAnim(p->sp, 2);
                        if (p->dx > 0 && p->col_x < p->partner->col_x) {
                            SPR_ensureAnim(p->sp, 2);
                            if (p->partner->col_x >= FIXX(160)) {
                                p->partner->force_x += e->alien_force;
                            } 
                        } else if (p->dx < 0 && p->col_x > p->partner->col_x) {
                            SPR_ensureAnim(p->sp, 2);
                            if (p->partner->col_x < FIXX(160)) {
                                p->partner->force_x -= e->alien_force;
                            }
                        }
                    }
                    return;
                case 3: // reserved
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                    return;
                case 10: // flying high
                    if (p->y >= FIXY(104)) {
                        p->goal_x = FIXX(24 + random_with_max(288));
                        p->goal_y = p->y + FIXY(8);
                        p->state = 11;
                        p->state_frames = 0;
                        return;
                    }
                    if (p->dx < 0 && p->f != LEFT) {
                        p->f = LEFT;
                        Physics_bg_element_redraw(p, TRUE);
                    } else if (p->dx > 0 && p->f != RIGHT) {
                        p->f = RIGHT;
                        Physics_bg_element_redraw(p, FALSE);
                    }
                    if (p->state_frames >= 13) {
                        p->anim_no = p->anim_no ? 0 : 1;
                        Physics_bg_element_redraw(p, p->f == LEFT);
                        p->state_frames = 0;
                    }
                    return;
                case 11: // flying last row
                    if (p->y >= p->goal_y && (
                            (p->dx > 0 && p->x >= p->goal_x) ||
                            (p->dx < 0 && p->x <= p->goal_x) ||
                            (p->y >= FIXY(112))
                            )
                        ) {
                            // Converting from bg to sprite
                            p->bg_element = FALSE;
                            VDP_clearTileMapRect(
                                BG_B,
                                fixxToInt(p->start_x) / 8,
                                fixyToInt(p->start_y) / 8,
                                p->w / 8,
                                p->h / 8
                                );
                            p->sp = SPR_addSprite(
                                p->spriteDef,
                                fixxToRoundedInt(p->x),
                                fixyToRoundedInt(p->y),
                                TILE_ATTR(p->pal, TRUE, FALSE, FALSE)
                                );
                            SPR_setPriority(p->sp, TRUE);
                            SPR_setAnim(p->sp, 1);
                            p->dx = 0;
                            p->dy = 0;
                            p->grav_model = TRUE;
                            p->state = 1;
                            p->state_frames = 0;
                            --(*p->instance_counter);
                            p->instance_counter = NULL;
                        }
                    return;
                default:
                    break;
            }
            return;
        case WHAT_HORSE:
            return;
        default:
            return;
    }
}

bool interact(Enc *e, Physics *pi, Physics *pj) {
    // sort by WHAT so we don't always have to try both directions
    Phy *p1, *p2;
    if (pi->what <= pj->what) {
        p1 = pi;
        p2 = pj;
    } else {
        p1 = pj;
        p2 = pi;
    }
    if (p1->what == WHAT_CANNON && (p2->what == WHAT_GUY || p2->what == WHAT_ALIEN)) {
        p2->partner = p1;
        p2->state_frames = 0;
        if (p1->col_x < p2->col_x) {
            p2->x = p1->x + FIXX(p1->w) - FIXX(1);
        } else if (p1->col_x > p2->col_x) {
            p2->x = p1->x - FIXX(p2->w) + FIXX(1);
        }
        return TRUE;
    } else if (p1->what == WHAT_CANNON && p2->what == WHAT_BULLET) {
        return TRUE;
    } else if (p1->what == WHAT_ALIEN && (p2->what == WHAT_BULLET || p2->what == WHAT_PARTICLE)) {
        Physics_del(p1, e);
        Physics_del(p2, e);
        for (u8 i = 0; i < 4; ++i) {
            Physics_new_particle(e, p1->x + FIXX(4), p1->y + FIXY(4));
        }
        XGM_startPlayPCMNextCh(SND_SAMPLE_ALIEN_POP, 7);
        Enc_update_score(e, 1);
        return TRUE;
    } else if (p1->what == WHAT_GUY && p2->what == WHAT_BULLET) {
        return TRUE;
    } else if (p1->what == WHAT_HORSE && p2->what == WHAT_BULLET) {
        Physics_del(p2, e);
        for (u8 i = 0; i < 4; ++i) {
            Physics_new_bullet_particle(e, p2->x + FIXX(4), p2->y + FIXY(4));
        }
        Enc_update_score(e, 100);
        XGM_startPlayPCMNextCh(SND_SAMPLE_HORSE, 13);
        return TRUE;
    } else if (p2->what == WHAT_PARTICLE) {
        return TRUE;
    }
    return FALSE;
}
