#include "media_center.h"
#include "cmsis_os2.h"
#include "GLCD.h"
#include "Board_Joystick.h"
#include "pixel_ui.h"
#include "games.h"
#include "gallery.h"
#include "audio_player.h"
#include "audio_engine.h"
#include <stdio.h>

typedef enum {
    APP_MENU = 0,
    APP_GALLERY,
    APP_MUSIC,
    APP_SNAKE,
    APP_MEMORY
} AppState;

#define MENU_COUNT 4

static AppState state = APP_MENU;
static int menu_index = 0;
static uint32_t previous_joy = 0U;

static const char * const menu_items[MENU_COUNT] = {
    "  GALLERY   ",
    " MP3 PLAYER ",
    "   SNAKE    ",
    "MEMORY MATCH"
};

static void draw_menu_item(int index, int selected) {
    unsigned int line  = 2U + (unsigned int)index * 2U; /* lines 2, 4, 6, 8 */
    unsigned int y_top = line * 24U - 6U;
    unsigned short bg  = selected ? PX_SEL_MID : PX_STONE_MID;

    px_draw_button(40U, y_top, 240U, 36U, selected);

    GLCD_SetBackColor(bg);
    GLCD_SetTextColor(PX_TEXT_DARK);
    GLCD_DisplayString(line, 4, 1, (unsigned char *)menu_items[index]);
}

static void draw_menu_full(void) {
    int i;
    px_draw_grass_bg(12U);

    /* Pixel-art title banner */
    px_rect(54U, 14U, 212U, 20U, PX_OUTLINE);
    px_rect(56U, 16U, 208U, 16U, PX_STONE_DARK);
    GLCD_SetBackColor(PX_STONE_DARK);
    GLCD_SetTextColor(PX_SEL_HI);
    GLCD_DisplayString(2, 17, 0, (unsigned char *)"COE718 MEDIA CENTER");

    for (i = 0; i < MENU_COUNT; i++) {
        draw_menu_item(i, i == menu_index);
    }
}

void MediaCenter_Init(void) {
    state = APP_MENU;
    menu_index = 0;
    previous_joy = 0U;
    draw_menu_full();
}

static uint32_t new_press(uint32_t current) {
    uint32_t pressed = current & ~previous_joy;
    previous_joy = current;
    return pressed;
}

void MediaCenter_Task(uint32_t joystick_state) {
    uint32_t pressed = new_press(joystick_state);
    int exit_to_menu = 0;

    if (state == APP_MENU) {
        if (pressed & JOYSTICK_UP) {
            int prev = menu_index;
            menu_index = (menu_index + MENU_COUNT - 1) % MENU_COUNT;
            draw_menu_item(prev, 0);
            draw_menu_item(menu_index, 1);
            Audio_PlaySfx(SFX_NAV);
        } else if (pressed & JOYSTICK_DOWN) {
            int prev = menu_index;
            menu_index = (menu_index + 1) % MENU_COUNT;
            draw_menu_item(prev, 0);
            draw_menu_item(menu_index, 1);
            Audio_PlaySfx(SFX_NAV);
        } else if (pressed & JOYSTICK_CENTER) {
            Audio_PlaySfx(SFX_SELECT);
            switch (menu_index) {
                case 0: state = APP_GALLERY; Gallery_Enter();     break;
                case 1: state = APP_MUSIC;   AudioPlayer_Enter(); break;
                case 2: state = APP_SNAKE;   Snake_Enter();       break;
                case 3: state = APP_MEMORY;  Memory_Enter();      break;
                default: break;
            }
        }
        return;
    }

    switch (state) {
        case APP_GALLERY: exit_to_menu = Gallery_Task(pressed);     break;
        case APP_MUSIC:   exit_to_menu = AudioPlayer_Task(pressed); break;
        case APP_SNAKE:   exit_to_menu = Snake_Task(pressed);       break;
        case APP_MEMORY:  exit_to_menu = Memory_Task(pressed);      break;
        default:          exit_to_menu = 1;                         break;
    }

    if (exit_to_menu) {
        if (state == APP_MUSIC) {
            AudioPlayer_Exit();
        }
        Audio_PlaySfx(SFX_NAV);
        state = APP_MENU;
        draw_menu_full();
    }
}