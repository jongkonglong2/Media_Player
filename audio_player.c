#include "audio_player.h"
#include "audio_engine.h"
#include "GLCD.h"
#include "Board_Joystick.h"
#include "pixel_ui.h"
#include <stdio.h>

static uint8_t volume_percent = 50;
static uint8_t last_volume_percent = 255;
static uint8_t anim_step = 0;
static uint8_t frame_div = 0;
static uint8_t exit_armed = 0;

static void draw_volume_bar(uint8_t vol) {
    char msg[24];
    unsigned int fill_w = (232U * (unsigned int)vol) / 100U;

    GLCD_SetBackColor(PX_BG_LIGHT);
    GLCD_SetTextColor(PX_TEXT_DARK);
    sprintf(msg, "POT VOLUME: %3u%%", vol);
    GLCD_DisplayString(18, 17, 0, (unsigned char *)msg);

    px_rect(40U, 158U, 240U, 22U, PX_OUTLINE);
    px_rect(42U, 160U, 236U, 18U, PX_STONE_DARK);
    if (fill_w > 0U) {
        px_rect(44U, 162U, fill_w, 14U, (vol > 80U) ? PX_APPLE_RED : PX_SEL_MID);
    }
}

static void draw_visualizer(void) {
    static const uint8_t heights[4][8] = {
        {10, 26, 18, 34, 22, 14, 28, 16},
        {24, 14, 32, 18, 36, 24, 14, 26},
        {16, 34, 22, 28, 14, 30, 20, 12},
        {28, 20, 14, 24, 30, 18, 34, 22}
    };
    int i;
    anim_step = (uint8_t)((anim_step + 1U) & 0x03U);

    for (i = 0; i < 8; i++) {
        unsigned int bx = 56U + (unsigned int)i * 26U;
        unsigned int h  = heights[anim_step][i];
        px_rect(bx, 82U, 18U, 42U, PX_OUTLINE);
        px_rect(bx + 2U, 122U - h, 14U, h, PX_SEL_MID);
    }
}

void AudioPlayer_Enter(void) {
    last_volume_percent = 255;
    frame_div = 0;
    exit_armed = 0;

    px_draw_grass_bg(12U);

    /* Pixel-art Title Banner */
    px_rect(54U, 14U, 212U, 20U, PX_OUTLINE);
    px_rect(56U, 16U, 208U, 16U, PX_STONE_DARK);
    GLCD_SetBackColor(PX_STONE_DARK);
    GLCD_SetTextColor(PX_SEL_HI);
    GLCD_DisplayString(2, 18, 0, (unsigned char *)"USB AUDIO PLAYER");

    /* Visualizer Stone Panel */
    px_rect(32U, 44U, 256U, 90U, PX_OUTLINE);
    px_rect(34U, 46U, 252U, 86U, PX_STONE_DARK);
    GLCD_SetBackColor(PX_STONE_DARK);
    GLCD_SetTextColor(PX_STONE_HI);
    GLCD_DisplayString(7, 17, 0, (unsigned char *)"PC STREAM CONNECTED");

    /* Clickable-style Exit Hint Button */
    px_draw_button(96U, 190U, 128U, 30U, 1);
    GLCD_SetBackColor(PX_SEL_MID);
    GLCD_SetTextColor(PX_TEXT_DARK);
    GLCD_DisplayString(25, 19, 0, (unsigned char *)"LEFT : EXIT");

    AudioUSB_Connect();

    volume_percent = AudioUSB_UpdatePotVolume();
    draw_volume_bar(volume_percent);
    draw_visualizer();
    last_volume_percent = volume_percent;
}

int AudioPlayer_Task(uint32_t pressed) {
    uint32_t joy_now = Joystick_GetState();

    if ((joy_now & (JOYSTICK_LEFT | JOYSTICK_CENTER)) == 0U) {
        exit_armed = 1;
    }

    if ((pressed & JOYSTICK_LEFT) || (exit_armed && (joy_now & JOYSTICK_LEFT))) {
        return 1;
    }

    volume_percent = AudioUSB_UpdatePotVolume();
    if (last_volume_percent == 255 ||
        volume_percent >= last_volume_percent + 2U ||
        volume_percent + 2U <= last_volume_percent) {
        draw_volume_bar(volume_percent);
        last_volume_percent = volume_percent;
    }

    frame_div++;
    if (frame_div >= 8U) {
        frame_div = 0;
        draw_visualizer();
    }

    return 0;
}

void AudioPlayer_Exit(void) {
    AudioUSB_Disconnect();
}