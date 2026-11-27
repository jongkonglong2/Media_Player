#include "gallery.h"
#include "GLCD.h"
#include "Board_Joystick.h"
#include "pixel_ui.h"
#include "audio_engine.h"
#include <stdio.h>

typedef unsigned char  guint8;
typedef unsigned int   guint;
typedef char           gchar;

#include "mort.c"
#include "G_khan.c"
#include "Jensen.c"

static const unsigned int img_w[3] = {
    MORTY_WIDTH, KHAN_WIDTH, JENSEN_WIDTH
};
static const unsigned int img_h[3] = {
    MORTY_HEIGHT, KHAN_HEIGHT, JENSEN_HEIGHT
};
static const unsigned char * const img_data[3] = {
    (const unsigned char *)MORTY_PIXEL_DATA,
    (const unsigned char *)KHAN_PIXEL_DATA,
    (const unsigned char *)JENSEN_PIXEL_DATA
};
static const char * const img_names[3] = {
    "Morty   ",
    "Dr. Khan",
    "Jensen  "
};

static int image_index;

static void gallery_update_content(void) {
    char msg[32];
    unsigned int x, y;

    px_rect(52U, 14U, 216U, 18U, PX_OUTLINE);
    px_rect(54U, 16U, 212U, 14U, PX_STONE_DARK);
    GLCD_SetBackColor(PX_STONE_DARK);
    GLCD_SetTextColor(PX_SEL_HI);
    sprintf(msg, "PHOTO %d/3 : %s", image_index + 1, img_names[image_index]);
    GLCD_DisplayString(2, 16, 0, (unsigned char *)msg);

    /* Beveled Pixel Picture Frame */
    px_rect(18U, 36U, 284U, 174U, PX_OUTLINE);
    px_rect(20U, 38U, 280U, 170U, PX_STONE_SHD);
    px_rect(24U, 42U, 272U, 162U, Black);

    x = (320U - img_w[image_index]) / 2U;
    y = 42U + (162U - img_h[image_index]) / 2U;
    GLCD_Bitmap(x, y, img_w[image_index], img_h[image_index],
                (unsigned char *)img_data[image_index]);
}

void Gallery_Enter(void) {
    image_index = 0;
    px_draw_grass_bg(10U);

    px_rect(18U, 212U, 284U, 16U, PX_STONE_DARK);
    GLCD_SetBackColor(PX_STONE_DARK);
    GLCD_SetTextColor(PX_STONE_HI);
    GLCD_DisplayString(27, 6, 0, (unsigned char *)"UP/DN: Browse Photos        LEFT: Exit");

    gallery_update_content();
}

int Gallery_Task(uint32_t pressed) {
    if (pressed & JOYSTICK_LEFT) {
        return 1;
    }
    if (pressed & JOYSTICK_UP) {
        image_index = (image_index + 2) % 3;
        Audio_PlaySfx(SFX_NAV);
    } else if (pressed & JOYSTICK_DOWN) {
        image_index = (image_index + 1) % 3;
        Audio_PlaySfx(SFX_NAV);
    } else {
        return 0;
    }
    gallery_update_content();
    return 0;
}