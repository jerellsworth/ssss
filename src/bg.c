#include "bh.h"

BG *BG_init(
    const MapDefinition *mapDef_bg,
    const TileSet *tileset_bg,
    const MapDefinition *mapDef_fg,
    const TileSet *tileset_fg,
    u8 (*collision)[MAP_TILES_H][MAP_TILES_W],
    const u16* pal) {
    BG *bg = (BG *)ct_calloc(sizeof(BG), 1);
    bg->theta = 512;
    bg->invader_speed = 1;

    BG_change_map(bg, mapDef_bg, tileset_bg, mapDef_fg, tileset_fg, BG_BEHAVIOR_NONE);

    bg->collision_bak = collision;
    bg->collision = ct_calloc(MAP_TILES_H * MAP_TILES_W, sizeof(u8));
    memcpy(bg->collision, bg->collision_bak, MAP_TILES_H * MAP_TILES_W * sizeof(u8));

    PAL_setPalette(PAL0, pal, DMA);
    VDP_setBackgroundColor(1);
    SYS_doVBlankProcess();

    return bg;
};

void BG_change_map(
    BG *bg,
    const MapDefinition* mapDef_bg,
    const TileSet *tileset_bg,
    const MapDefinition* mapDef_fg,
    const TileSet *tileset_fg,
    BG_Behavior behavior
    ) {

    if (bg->map) {
        MAP_release(bg->map);
        bg->map = NULL;
    }
    bg->behavior = behavior;
    bg->tile_ind = TILE_USER_INDEX;
    bg->bg_tile_ind = bg->tile_ind;
    VDP_loadTileSet(tileset_bg, bg->tile_ind, DMA);
    bg->map = MAP_create(mapDef_bg, BG_B, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, bg->tile_ind));
    bg->tile_ind += tileset_bg->numTile;
    BG_scroll_to(bg, 80, 0);

    bg->fg_tile_ind = bg->tile_ind;
    VDP_loadTileSet(tileset_fg, bg->fg_tile_ind, DMA);
    bg->tile_ind += tileset_bg->numTile;
    bg->map_fg = MAP_create(mapDef_fg, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, bg->fg_tile_ind));
    MAP_scrollTo(bg->map_fg, 0, 0);
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_COLUMN);

    SYS_doVBlankProcess();
}

void BG_scroll_to(BG *bg, fixx x, fixy y) {
    MAP_scrollTo(bg->map, fixxToRoundedInt(x), fixyToRoundedInt(y));
}

void BG_scroll_to_diff(BG *bg, fix16 dx, fix16 dy) {
    u32 x = bg->map->posX + fix16ToRoundedInt(dx);
    u32 y = bg->map->posY + fix16ToRoundedInt(dy);
    MAP_scrollTo(bg->map, x, y);
}

fixx BG_x(BG *bg) {
    return FIXX(bg->map->posX);
}

fixy BG_y(BG *bg) {
    return FIXY(bg->map->posY);
}

void BG_update(BG *bg) {
    
    ++bg->frames;

    // BG_A block
    if (!(bg->frames & 3)) {

        fix16 offset_diff = sinFix16(bg->theta);
        fix16 off_here = 0;
        for (u8 i = 10; i <= 19; ++i) {
            off_here += offset_diff;
            (bg->v_offsets)[i] = fix16ToRoundedInt(off_here);
        }
        off_here = 0;
        for (s16 i = 9; i >= 0; --i) {
            off_here -= offset_diff;
            (bg->v_offsets)[(u16)i] = fix16ToRoundedInt(off_here);
        }
        VDP_setVerticalScrollTile(BG_A, 0, bg->v_offsets, 20, DMA);

        u16 n_lines = VSCROLL_N_LINES;
        s16 h_offsets[VSCROLL_N_LINES];
        offset_diff = offset_diff >> 3;
        u16 halfway = VSCROLL_HALFWAY;
        off_here = 0;
        
        for (u16 i = halfway;
             i < n_lines;
             ++i) {
            off_here += offset_diff;
            h_offsets[i] = fix16ToRoundedInt(off_here);
         }
         
        off_here = 0;
        
        for (s16 i = halfway - 1;
             i >= 0;
             --i) {
            off_here -= offset_diff;
            h_offsets[(u16)i] = fix16ToRoundedInt(off_here);
         }
         VDP_setHorizontalScrollLine(BG_A, VSCROLL_START_LINE, h_offsets, n_lines, DMA);
    }
    
    // BG_B block
    s16 h_offsetsb[224];
    s16 v_offsetsb[20];
    if ((bg->invader_dx < 0 && bg->invader_x <= -92) ||
        (bg->invader_dx > 0 && bg->invader_x >= 0)) {
        bg->invader_dx = 0;
        bg->invader_dy = -bg->invader_speed;
        bg->invader_next_turn = bg->invader_y - 8;
    } else if (bg->invader_dy < 0 && bg->invader_y <= bg->invader_next_turn) {
        bg->invader_dy = 0;
        if (bg->invader_x < -40) {
            bg->invader_dx = bg->invader_speed;
        } else {
            bg->invader_dx = -bg->invader_speed;
        }
    }
    bg->invader_x += bg->invader_dx;
    bg->invader_y += bg->invader_dy;
    memsetU16((u16 *)h_offsetsb, (u16)bg->invader_x, 224);
    memsetU16((u16 *)v_offsetsb, (u16)bg->invader_y, 20);
    VDP_setHorizontalScrollLine(BG_B, 0, h_offsetsb, 224, DMA);
    VDP_setVerticalScrollTile(BG_B, 0, v_offsetsb, 20, DMA);

}

void BG_reset_fx(BG *bg) {
}

void BG_del(BG *bg) {
    free(bg->collision);
    MAP_release(bg->map);
    free(bg);
    VDP_init();
    SYS_doVBlankProcess();
    VDP_waitVSync();
    VDP_init(); // I don't know why you need to do this, but you do
}

bool BG_in_range(BG *bg, Phy *p) {
    u16 int_y = fixyToInt(p->y);
    u16 int_x = fixxToInt(p->x);
    return (
        int_y + p->h >= bg->map->posY &&
        int_y <= bg->map->posY + 224 &&
        int_x + p->w >= bg->map->posX &&
        int_x <= bg->map->posX + 320);
}

fixy BG_collide(BG *bg, fixx x, fixy y) {
    s16 rounded = fixxToRoundedInt(x);
    if (rounded < 16 || rounded > 304) return FALSE;
    u8 col = rounded >> 4;
    fixy top = FIXY(LEVER_TOP - bg->v_offsets[col]);
    if (y >= top) {
        return top;
    }
    return 0;
}

void BG_tilt_by(BG *bg, s16 dtheta) {
    s16 sum = bg->theta + dtheta;
    bg->theta = min(max(sum, 256), 768);
}

fix16 BG_net_force(BG *bg) {
    fix16 f = 0;
    for (u8 i = 0; i < MAX_PHYSICS_OBJS; ++i) {
        Phy *p = ALL_PHYSICS_OBJS[i];
        if (!p) continue;
        if (!BG_collide(bg, p->x, p->y + FIXY(p->h) + FIX16(2))) continue;
        // TODO not sure about that 2
        if (p->x >= FIXX(160)) {
            f -= fix16Mul((p->x - FIXX(160)) >> 5, FIX16(p->mass));
        } else {
            f += fix16Mul((FIXX(160) - p->x) >> 5, FIX16(p->mass));
        }
    }
    return f;
}
