#ifndef BG_H
#define BG_H

#include "bh.h"

#define COLLISION_NONE 0
#define COLLISION_WALL 1

#define VSCROLL_START_LINE 152
#define VSCROLL_STOP_LINE 224
#define VSCROLL_N_LINES 72
#define VSCROLL_HALFWAY 36
#define LEVER_TOP 184
#define LEVER_GAP 15

typedef enum {
    BG_BEHAVIOR_NONE
} BG_Behavior;

struct BG_s {
    u8 (*collision)[MAP_TILES_H][MAP_TILES_W];
    u8 (*collision_bak)[MAP_TILES_H][MAP_TILES_W];
    Map *map;
    Map *map_fg;

    fixx x;
    fixy y;
    u16 frames;
    u16 tile_ind;

    bool dbg;

    u16 bg_tile_ind;
    u16 fg_tile_ind;

    BG_Behavior behavior;

    u16 theta;

    s16 v_offsets[20];

    s16 invader_x;
    s16 invader_y;
    s16 invader_dx;
    s16 invader_dy;
    s16 invader_next_turn;
    u16 invader_speed;
};

BG *BG_init(
    const MapDefinition *mapDef_bg,
    const TileSet *tileset_bg,
    const MapDefinition *mapDef_fg,
    const TileSet *tileset_fg,
    u8 (*collision)[MAP_TILES_H][MAP_TILES_W],
    const u16* pal);
void BG_change_map(
    BG *bg,
    const MapDefinition *mapDef_bg,
    const TileSet *tileset_bg,
    const MapDefinition *mapDef_fg,
    const TileSet *tileset_fg,
    BG_Behavior behavior
    );
void BG_scroll_to(BG *bg, fixx x, fixy y);
void BG_scroll_to_diff(BG *bg, fix16 dx, fix16 dy);
void BG_update(BG *bg);
void BG_reset_fx(BG *bg);
void BG_del(BG *bg);
fixy BG_collide(BG *bg, fixx x, fixy y);
bool BG_in_range(BG *bg, Phy *p);
fixx BG_x(BG *bg);
fixy BG_y(BG *bg);
void BG_tilt_by(BG *bg, s16 theta);
fix16 BG_net_force(BG *bg); // force down on left side

#endif
