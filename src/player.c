# include "bh.h"

Player *Player_new(
        u8 joy
    ) {
    Player *pl = ct_calloc(1, sizeof(Player));
    pl->joy = joy;
    pl->cooldown = 0;
    return pl;
}

void Player_del(Player *p) {
    free(p);
}

void Player_input(Player *pl, Enc *e) {
    u16 joy = JOY_readJoypad(pl->joy);
    Phy *p = pl->p;
    if (!p) return;
    if (pl->cooldown > 0) --pl->cooldown;
    if (pl->cooldown2> 0) --pl->cooldown2;

    if (e->state == ENC_RUNNING) {

        if (!p) return;
        if ((joy & BUTTON_START) && (pl->cooldown == 0)) {
            e->state = ENC_PAUSED;
            pl->cooldown = 10;
            XGM_startPlayPCMNextCh(SND_SAMPLE_PAUSE, 14);
            return;
        }
        switch (p->what) {
            case WHAT_GUY:
                fix16 force = FIX16(5); 
                p->dx = 0;
                if (joy & BUTTON_RIGHT) {
                    SPR_setHFlip(p->sp, FALSE);
                    p->f = RIGHT;
                    p->dx = FIXX(2);
                    SPR_ensureAnim(p->sp, 1);
                } else if (joy & BUTTON_LEFT) {
                    SPR_setHFlip(p->sp, TRUE);
                    p->f = LEFT;
                    p->dx = FIXX(-2);
                    SPR_ensureAnim(p->sp, 1);
                } else {
                    SPR_ensureAnim(p->sp, 0);
                }
                if (
                    (joy & BUTTON_ACTION) &&
                    (pl->cooldown == 0) &&
                    BG_collide(e->bg, p->x, p->y + FIXY(p->h) + FIX16(3)) &&
                    p->state == 0
                    ) {
                    p->dy = FIX16(-8);
                    pl->cooldown = 3;
                    p->state = 1;
                    p->state_frames = 0;
                    XGM_startPlayPCMNextCh(SND_SAMPLE_JUMP, 11);
                }
                if (!(joy & BUTTON_B)) {
                    p->push_ready = TRUE;
                }
                if (p->partner) {
                    if ((joy & BUTTON_B) &&
                        (p->push_ready == TRUE) &&
                        (p->push_frames == 0)) {
                        p->push_ready = FALSE;
                        p->push_frames = 6;
                    }
                    if (p->push_frames > 0) {
                        force = FIX16(10);
                    }
                    if (p->dx > 0 && p->col_x < p->partner->col_x) {
                        SPR_ensureAnim(p->sp, 2);
                        p->partner->force_x += force;
                    } else if (p->dx < 0 && p->col_x > p->partner->col_x) {
                        SPR_ensureAnim(p->sp, 2);
                        p->partner->force_x -= force;
                    }
                }
                return;
            default:
                return;
        }
    } else if (e->state == ENC_PAUSED) {
        if ((joy & BUTTON_START) && (pl->cooldown == 0)) {
            e->state = ENC_RUNNING;
            pl->cooldown = 10;
            XGM_startPlayPCMNextCh(SND_SAMPLE_PAUSE, 14);
            return;
        }
    }
}
