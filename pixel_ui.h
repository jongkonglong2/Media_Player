#ifndef PIXEL_UI_H
#define PIXEL_UI_H

#include "GLCD.h"

/* Retro pixel-art RGB565 palette inspired by stone & moss UI */
#define PX_BG_LIGHT     0x7F31U  /* Mint green background        */
#define PX_BG_DOT       0x668FU  /* Subtle grass dither dot      */
#define PX_OUTLINE      0x1945U  /* Deep charcoal pixel outline  */
#define PX_STONE_HI     0xDF3DU  /* Stone top/left highlight     */
#define PX_STONE_MID    0xADF9U  /* Stone button face            */
#define PX_STONE_SHD    0x6BF1U  /* Stone bottom/right shadow    */
#define PX_STONE_DARK   0x3A8AU  /* Dark border rock             */
#define PX_MOSS_DARK    0x2B67U  /* Border moss dark             */
#define PX_MOSS_LIGHT   0x4D4AU  /* Border moss highlight        */
#define PX_SEL_HI       0xFF95U  /* Selected gold highlight      */
#define PX_SEL_MID      0xF648U  /* Selected gold face           */
#define PX_SEL_SHD      0xB423U  /* Selected gold shadow         */
#define PX_TEXT_DARK    0x2187U  /* Crisp dark pixel text        */
#define PX_ARENA_BG     0x1945U  /* Dark dungeon floor for Snake */
#define PX_SNAKE_BODY   0x3666U  /* Retro pixel green            */
#define PX_SNAKE_HI     0x87F0U  /* Pixel green highlight        */
#define PX_APPLE_RED    0xE924U  /* Pixel apple red              */

static void px_rect(unsigned int x, unsigned int y,
                    unsigned int w, unsigned int h,
                    unsigned short color) {
    if (w == 0U || h == 0U) {
        return;
    }
    GLCD_SetBackColor(color);
    GLCD_SetTextColor(color);
    GLCD_Bargraph(x, y, w, h, 1000U);
}

/* Draws a 3D beveled pixel-art stone button with notched corners */
static void px_draw_button(unsigned int x, unsigned int y,
                           unsigned int w, unsigned int h,
                           int selected) {
    unsigned short hi  = selected ? PX_SEL_HI  : PX_STONE_HI;
    unsigned short mid = selected ? PX_SEL_MID : PX_STONE_MID;
    unsigned short shd = selected ? PX_SEL_SHD : PX_STONE_SHD;

    /* Notched dark pixel outline */
    px_rect(x + 2U, y, w - 4U, h, PX_OUTLINE);
    px_rect(x, y + 2U, w, h - 4U, PX_OUTLINE);

    /* Main button face */
    px_rect(x + 2U, y + 2U, w - 4U, h - 4U, mid);

    /* Top and left bright pixel bevel */
    px_rect(x + 2U, y + 2U, w - 4U, 3U, hi);
    px_rect(x + 2U, y + 2U, 3U, h - 6U, hi);

    /* Bottom and right shadow bevel */
    px_rect(x + 2U, y + h - 6U, w - 4U, 4U, shd);
    px_rect(x + w - 5U, y + 4U, 3U, h - 8U, shd);
}

/* Draws a chunky mossy stone pixel border around the 320x240 screen */
static void px_draw_stone_border(unsigned int t) {
    unsigned int i;

    /* Solid dark outline frame */
    px_rect(0U, 0U, 320U, t, PX_OUTLINE);
    px_rect(0U, 240U - t, 320U, t, PX_OUTLINE);
    px_rect(0U, 0U, t, 240U, PX_OUTLINE);
    px_rect(320U - t, 0U, t, 240U, PX_OUTLINE);

    /* Top & bottom alternating stone/moss bricks */
    for (i = 0U; i < 320U; i += 20U) {
        unsigned short c_top = ((i / 20U) & 1U) ? PX_STONE_DARK : PX_MOSS_DARK;
        unsigned short c_hi  = ((i / 20U) & 1U) ? PX_STONE_SHD  : PX_MOSS_LIGHT;
        px_rect(i + 1U, 1U, 18U, t - 2U, c_top);
        px_rect(i + 2U, 2U, 14U, 2U, c_hi);

        px_rect(i + 1U, 240U - t + 1U, 18U, t - 2U, c_top);
        px_rect(i + 2U, 240U - t + 2U, 14U, 2U, c_hi);
    }

    /* Left & right alternating stone/moss bricks */
    for (i = t; i < 240U - t; i += 20U) {
        unsigned int bh = (i + 20U <= 240U - t) ? 18U : (240U - t - i);
        unsigned short c_side = ((i / 20U) & 1U) ? PX_MOSS_DARK  : PX_STONE_DARK;
        unsigned short c_hi   = ((i / 20U) & 1U) ? PX_MOSS_LIGHT : PX_STONE_SHD;
        px_rect(1U, i + 1U, t - 2U, bh, c_side);
        px_rect(2U, i + 2U, 2U, (bh > 4U) ? (bh - 4U) : bh, c_hi);

        px_rect(320U - t + 1U, i + 1U, t - 2U, bh, c_side);
        px_rect(320U - t + 2U, i + 2U, 2U, (bh > 4U) ? (bh - 4U) : bh, c_hi);
    }
}

/* Clears screen to mint green with sparse pixel-art grass dots */
static void px_draw_grass_bg(unsigned int border_t) {
    unsigned int x, y;
    GLCD_Clear(PX_BG_LIGHT);
    for (y = border_t + 8U; y < 240U - border_t - 6U; y += 20U) {
        for (x = border_t + 8U; x < 320U - border_t - 6U; x += 24U) {
            unsigned int ox = ((y / 20U) & 1U) ? 12U : 0U;
            if (x + ox < 320U - border_t - 4U) {
                px_rect(x + ox, y, 4U, 4U, PX_BG_DOT);
            }
        }
    }
    px_draw_stone_border(border_t);
}

#endif