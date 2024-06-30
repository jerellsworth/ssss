#include "bh.h"

void wait(u8 frames) {
    for (u8 i = 0; i < frames; ++i) {
        SYS_doVBlankProcess();
        VDP_waitVSync();
    }
}

void results(Enc *enc) {
    VDP_init();
    XGM_startPlayPCMNextCh(SND_SAMPLE_GAME_OVER, 15);
    VDP_setTextPalette(PAL3);
    u16 black = 0;
    PAL_setColor(63, 255);
    char buf[32];
    sprintf(buf, "score: %d00", enc->score);
    VDP_drawTextBG(BG_B, buf, 15, 2);
    sprintf(buf, " waves: %d", enc->level);
    VDP_drawTextBG(BG_B, buf, 14, 3);

    VDP_drawTextBG(BG_B, "You must submit to supreme suffering", 2, 18);
    VDP_drawTextBG(BG_B, "in order to discover", 10, 19);
    VDP_drawTextBG(BG_B, "the completion of of joy.", 8, 20);
    VDP_drawTextBG(BG_B, "--Calvin", 16, 21);
    PAL_setPalette(PAL1, PAL_SPRITE.data, DMA);
    Sprite *sp = SPR_addSprite(
            &SPR_GUY1,
            156,
            86,
            TILE_ATTR(PAL1, TRUE, FALSE, FALSE)
            );
    SPR_setAnim(sp, 2);
    while (TRUE) {
        if (JOY_readJoypad(JOY_ALL)) break;
        SPR_update();
        SYS_doVBlankProcess();
    }
    JOY_waitPressBtn();
    PAL_fadeTo(63, 63, &black, 30, FALSE);
    SPR_releaseSprite(sp);
    VDP_init();
}

int main(bool hard_reset) {
    if (!hard_reset) SYS_hardReset();

    SPR_init();
    JOY_init();
    snd_init();

    Menu *m = NULL;
    while (TRUE) {
        m = INTRO_run(m);
        Enc *e = Enc_run(m);
        results(e);
        Enc_del(e);
    }
	return 0;
}
