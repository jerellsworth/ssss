#include "bh.h"

void Enc_reset_pc(Enc *e, Player *pl, bool death, u8 iframes) {
    if (pl->p) {
        Physics_del(pl->p, e);
    }
    if (death) {
        --e->lives;
        if (e->lives == 0) {
            e->state = ENC_COMPLETE;
            return;
        }
    }
    const SpriteDefinition *spriteDef;
    switch(pl->player_no) {
        case 0:
            spriteDef = &SPR_GUY1;
            break;
        case 1:
            spriteDef = &SPR_GUY2;
            break;
        case 2:
            spriteDef = &SPR_GUY3;
            break;
        case 4:
            spriteDef = &SPR_GUY4;
            break;
        default:
            spriteDef = &SPR_GUY4;
            break;
    }
    Phy *p = Physics_new_guy(e, spriteDef, FIXX(120) + FIXX(pl->player_no * 20), FIXY(112));
    p->pl = pl;
    pl->p = p;
    e->level_frames = 0;
}

void Enc_alien_line(Enc *e) {
    e->bg->invader_dx = -e->bg->invader_speed;
    e->bg->invader_dy = 0;
    e->bg->invader_x = 0;
    e->bg->invader_y = 0;
    fixx x = FIXX(312);
    fixy y = FIXY(88);
    for (u8 r = 0; r < 3; ++r) {
        x = FIXX(312);
        for (u8 c = 0; c < 10; ++c) {
            Physics_new_alien(e, x, y, FIX16(-1), 0, &e->aliens);
            x -= FIXX(24);
        }
        y -= FIXY(24);
    }
}

Enc *Enc_new(u8 n_players) {
    Enc *e = ct_calloc(1, sizeof(Encounter));
    e->n_players = n_players;
    e->state = ENC_RUNNING;

    PAL_setPalette(PAL0, PAL_BG.data, DMA);
    PAL_setPalette(PAL1, PAL_SPRITE.data, DMA);

    e->bg = BG_init(
        &MAP_BG,
        &TLS_BG,
        &MAP_FG,
        &TLS_FG,
        &COLLISION_BG,
        PAL_BG.data
        );
    e->tm = Tile_Manager_new();

    u16 joy = JOY_1;
    for (u8 player_no = 0; player_no < n_players; ++player_no) {
        if (joy == JOY_2 && JOY_getPortType(PORT_2) == PORT_TYPE_EA4WAYPLAY) ++joy;
        e->players[player_no] = Player_new(joy);
        e->players[player_no]->player_no = player_no;
        ++joy;
    }
    e->alien_force = FIX16(2) * n_players;
    e->horse_frames = random_with_max(900);
    e->lives = 4;
    Enc_update_score(e, 0);

    for (u8 i = 0; i < 16; ++i) {
        Physics_new_star(e, FIXX(random_with_max(312)), FIXY(random_with_max(216)));
    }

    return e;
}

void Enc_del(Enc *e) {
    BG_del(e->bg);
    free(e);
}

void Enc_cleanup(Enc *e) {
    u16 black[16];
    memset(black, 0, 32);
    PAL_fadeTo(0, 63, black, 60, FALSE);
    Physics_del_all(e);
    SPR_reset();
    SPR_update();
    SYS_doVBlankProcess();
    XGM_stopPlayPCM(SOUND_PCM_CH1);
}

void Enc_update(Enc *e) {
    if (e->music_on && !(e->frames & 3) && !XGM_isPlayingPCM(SOUND_PCM_CH1_MSK)) {
        switch (e->song) {
            case 0:
                XGM_startPlayPCM(SND_SAMPLE_SONG_1, 15, SOUND_PCM_CH1);
                break;
            default:
                break;
        }
    }
    ++e->song_frames;
    if (e->state == ENC_PAUSED) return;

    if (e->cannon_counter == 0) {
        --e->lives;
        if (e->lives == 0) {
            e->state = ENC_COMPLETE;
            return;
        }
        char buf[16];
        sprintf(buf, "%d", e->lives);
        VDP_clearTextBG(BG_A, 39, 1, 1); 
        VDP_drawTextBG(BG_A, buf, 39, 1);
        Physics_new_cannon(e, FIX16(160), FIX16(112), &(e->cannon_counter));
    }
    for (u8 player_no = 0; player_no < e->n_players; ++player_no) {
        Player *pl = e->players[player_no];
        if (pl->p == NULL) {
            Enc_reset_pc(e, pl, FALSE, 0);
        }
    }
    if (e->aliens == 0) {
        ++e->level;
        if (!(e->level & 3)) e->bg->invader_speed *= 2;
        Enc_alien_line(e);
    }
    if (e->horse_frames > 0) {
        --e->horse_frames;
    }
    if ((e->horse_frames == 0) && (e->horses == 0)) {
        Physics_new_horse(e, FIXX(288), FIXY(8), &(e->horses));
        e->horse_frames = random_with_max(900);
    }
    if (!(e->frames & 3)) {
        fix16 net_force = BG_net_force(e->bg);

        if (net_force < 0) {
            BG_tilt_by(e->bg, min(16, (-net_force) >> 4));
        } else if (net_force > 0) {
            BG_tilt_by(e->bg, max(-16, (-net_force) >> 4));
        } else if (e->bg->theta < 512) {
            BG_tilt_by(e->bg, 16);
        } else if (e->bg->theta > 512) {
            BG_tilt_by(e->bg, -16);
        }

    }

    ++e->frames;
    ++e->state_frames;
    ++e->level_frames;
}

Enc *Enc_run(Menu *m) {
    Enc *e = Enc_new(m->first->option_selected + 1);

    e->music_on = !(m->first->next->option_selected);

    while (e->state != ENC_COMPLETE) {
        Enc_update(e);
        for (u8 player_no = 0; player_no < e->n_players; ++player_no) {
            Player_input(e->players[player_no], e);
        }
        if (e->state != ENC_PAUSED) {
            BG_update(e->bg);
            Physics_update_all(e);
        }
        SPR_update();
        SYS_doVBlankProcess();
    }
    Enc_cleanup(e);
    return e;
}

void Enc_update_score(Enc *e, u16 inc) {
    e->score += inc;
    char buf[16];
    sprintf(buf, "%d00", e->score);
    VDP_clearTextBG(BG_A, 20, 1, 16); 
    VDP_drawTextBG(BG_A, buf, 20, 1);
}
