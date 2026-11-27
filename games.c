#include "games.h"
#include "GLCD.h"
#include "Board_Joystick.h"
#include "pixel_ui.h"
#include "audio_engine.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include "wojack1.c"
#include "wojack2.c"
#include "wojack3.c"
#include "wojack4.c"
#include "wojack5.c"
#include "wojack6.c"
#include "wojack7.c"
#include "wojack8.c"

static uint32_t rng_state = 0x51A7C3D9u;

static uint32_t rand_next(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int rand_range(int n) {
    return (int)(rand_next() % (uint32_t)n);
}

/* =========================================================================
 * FULL-SCREEN SNAKE (Options only appear on Game Over)
 * ========================================================================= */
#define SNAKE_COLS      25
#define SNAKE_ROWS      18
#define SNAKE_MAX       (SNAKE_COLS * SNAKE_ROWS)
#define CELL_SIZE       12U
#define BOARD_X         10U
#define BOARD_Y         12U
#define BOARD_W         (SNAKE_COLS * CELL_SIZE)  /* 300px */
#define BOARD_H         (SNAKE_ROWS * CELL_SIZE)  /* 216px */

/* Images for the card matching game (must match wojack1.c - wojack8.c) */
#define CARD_IMG_W      48U
#define CARD_IMG_H      48U

static const unsigned char * const wojack_bitmaps[8] = {
    WOJACK1_PIXEL_DATA ,
    WOJACK2_PIXEL_DATA ,
    WOJACK3_PIXEL_DATA ,
    WOJACK4_PIXEL_DATA ,
    WOJACK5_PIXEL_DATA ,
    WOJACK6_PIXEL_DATA ,
    WOJACK7_PIXEL_DATA ,
    WOJACK8_PIXEL_DATA 
};

static int sx[SNAKE_MAX], sy[SNAKE_MAX], snake_len;
static int food_x, food_y, snake_dir, next_dir, snake_score, snake_over;
static int snake_menu_sel; /* 0 = RETRY, 1 = EXIT */
static uint32_t snake_last_step;

static void draw_snake_empty(int gx, int gy) {
    unsigned int px = BOARD_X + (unsigned int)gx * CELL_SIZE;
    unsigned int py = BOARD_Y + (unsigned int)gy * CELL_SIZE;
    px_rect(px, py, CELL_SIZE, CELL_SIZE, PX_ARENA_BG);
}

static void draw_snake_body(int gx, int gy) {
    unsigned int px = BOARD_X + (unsigned int)gx * CELL_SIZE;
    unsigned int py = BOARD_Y + (unsigned int)gy * CELL_SIZE;
    px_rect(px + 1U, py + 1U, CELL_SIZE - 2U, CELL_SIZE - 2U, PX_SNAKE_BODY);
    px_rect(px + 2U, py + 2U, 3U, 3U, PX_SNAKE_HI);
}

static void draw_snake_head(int gx, int gy) {
    unsigned int px = BOARD_X + (unsigned int)gx * CELL_SIZE;
    unsigned int py = BOARD_Y + (unsigned int)gy * CELL_SIZE;
    px_rect(px + 1U, py + 1U, CELL_SIZE - 2U, CELL_SIZE - 2U, PX_APPLE_RED);
    px_rect(px + 3U, py + 3U, 2U, 2U, White);
    px_rect(px + 7U, py + 3U, 2U, 2U, White);
}

static void draw_snake_food(int gx, int gy) {
    unsigned int px = BOARD_X + (unsigned int)gx * CELL_SIZE;
    unsigned int py = BOARD_Y + (unsigned int)gy * CELL_SIZE;
    px_rect(px + 2U, py + 3U, 8U, 7U, PX_APPLE_RED);
    px_rect(px + 5U, py + 1U, 3U, 2U, PX_SNAKE_HI);
    px_rect(px + 3U, py + 4U, 2U, 2U, White);
}

static void update_snake_wall_score(void) {
    char msg[16];
    GLCD_SetBackColor(PX_OUTLINE);
    GLCD_SetTextColor(PX_SEL_HI);
    sprintf(msg, "PTS:%-3d", snake_score);
    GLCD_DisplayString(0, 2, 0, (unsigned char *)msg);
}

static void snake_spawn_food(void) {
    int ok, i;
    do {
        food_x = rand_range(SNAKE_COLS);
        food_y = rand_range(SNAKE_ROWS);
        ok = 1;
        for (i = 0; i < snake_len; i++) {
            if (sx[i] == food_x && sy[i] == food_y) {
                ok = 0;
                break;
            }
        }
    } while (!ok);
    draw_snake_food(food_x, food_y);
}

static void snake_draw_gameover_buttons(void) {
    px_draw_button(84U, 92U, 152U, 32U, snake_menu_sel == 0);
    GLCD_SetBackColor(snake_menu_sel == 0 ? PX_SEL_MID : PX_STONE_MID);
    GLCD_SetTextColor(PX_TEXT_DARK);
    GLCD_DisplayString(4, 7, 1, (unsigned char *)"RETRY ");

    px_draw_button(84U, 140U, 152U, 32U, snake_menu_sel == 1);
    GLCD_SetBackColor(snake_menu_sel == 1 ? PX_SEL_MID : PX_STONE_MID);
    GLCD_SetTextColor(PX_TEXT_DARK);
    GLCD_DisplayString(6, 7, 1, (unsigned char *)" EXIT ");
}

static void snake_draw_gameover(void) {
    char msg[20];
    /* Centered pixel-art stone dialog box */
    px_rect(54U, 36U, 212U, 152U, PX_OUTLINE);
    px_rect(56U, 38U, 208U, 148U, PX_STONE_DARK);
    px_rect(60U, 42U, 200U, 140U, PX_BG_LIGHT);

    GLCD_SetBackColor(PX_BG_LIGHT);
    GLCD_SetTextColor(PX_APPLE_RED);
    GLCD_DisplayString(2, 5, 1, (unsigned char *)"GAME OVER");

    GLCD_SetTextColor(PX_TEXT_DARK);
    sprintf(msg, "FINAL SCORE: %d", snake_score);
    GLCD_DisplayString(9, 18, 0, (unsigned char *)msg);

    snake_menu_sel = 0;
    snake_draw_gameover_buttons();
}

static void snake_draw_initial(void) {
    int i;
    GLCD_Clear(PX_ARENA_BG);
    px_draw_stone_border(10U);
    px_rect(BOARD_X, BOARD_Y, BOARD_W, BOARD_H, PX_ARENA_BG);

    for (i = snake_len - 1; i >= 1; i--) {
        draw_snake_body(sx[i], sy[i]);
    }
    draw_snake_head(sx[0], sy[0]);
    update_snake_wall_score();
}

void Snake_Enter(void) {
    snake_len = 3;
    sx[0] = 6; sy[0] = 9;
    sx[1] = 5; sy[1] = 9;
    sx[2] = 4; sy[2] = 9;
    snake_dir = 1;
    next_dir  = 1;
    snake_score = 0;
    snake_over  = 0;
    snake_menu_sel = 0;
    snake_last_step = osKernelGetTickCount();
    rng_state ^= snake_last_step + 0x9E3779B9u;

    snake_draw_initial();
    snake_spawn_food();
}

int Snake_Task(uint32_t pressed) {
    int nx, ny, i, grow = 0;
    uint32_t now;

    if (snake_over) {
        if ((pressed & JOYSTICK_UP) || (pressed & JOYSTICK_DOWN)) {
            snake_menu_sel = !snake_menu_sel;
            snake_draw_gameover_buttons();
            Audio_PlaySfx(SFX_NAV);
        } else if (pressed & JOYSTICK_CENTER) {
            Audio_PlaySfx(SFX_SELECT);
            if (snake_menu_sel == 0) {
                Snake_Enter();
            } else {
                return 1; /* Exit to Main Menu */
            }
        }
        return 0;
    }

    if ((pressed & JOYSTICK_UP)         && snake_dir != 2) next_dir = 0;
    else if ((pressed & JOYSTICK_RIGHT) && snake_dir != 3) next_dir = 1;
    else if ((pressed & JOYSTICK_DOWN)  && snake_dir != 0) next_dir = 2;
    else if ((pressed & JOYSTICK_LEFT)  && snake_dir != 1) next_dir = 3;

    now = osKernelGetTickCount();
    if ((uint32_t)(now - snake_last_step) < 180U) {
        return 0;
    }
    snake_last_step = now;
    snake_dir = next_dir;

    nx = sx[0];
    ny = sy[0];
    if (snake_dir == 0) ny--;
    else if (snake_dir == 1) nx++;
    else if (snake_dir == 2) ny++;
    else if (snake_dir == 3) nx--;

    if (nx < 0 || nx >= SNAKE_COLS || ny < 0 || ny >= SNAKE_ROWS) {
        snake_over = 1;
    }

    grow = (nx == food_x && ny == food_y);

    if (!snake_over) {
        for (i = 0; i < snake_len - (grow ? 0 : 1); i++) {
            if (sx[i] == nx && sy[i] == ny) {
                snake_over = 1;
                break;
            }
        }
    }

    if (snake_over) {
        Audio_PlaySfx(SFX_GAMEOVER);
        snake_draw_gameover();
        return 0;
    }

    if (!grow) {
        draw_snake_empty(sx[snake_len - 1], sy[snake_len - 1]);
    } else if (snake_len < SNAKE_MAX) {
        snake_len++;
        snake_score += 10;
        update_snake_wall_score();
        Audio_PlaySfx(SFX_EAT);
    }

    draw_snake_body(sx[0], sy[0]);

    for (i = snake_len - 1; i > 0; i--) {
        sx[i] = sx[i - 1];
        sy[i] = sy[i - 1];
    }
    sx[0] = nx;
    sy[0] = ny;

    draw_snake_head(sx[0], sy[0]);

    if (grow) {
        snake_spawn_food();
    }

    return 0;
}

/* =========================================================================
 * FULL-SCREEN MEMORY MATCH (With On-Screen Clickable EXIT Button)
 * ========================================================================= */
#define CARD_COLS       4
#define CARD_ROWS       3
#define CARD_COUNT      (CARD_COLS * CARD_ROWS)
#define CURSOR_EXIT     CARD_COUNT  /* Index 12 = On-screen EXIT button */

#define CARD_W          64U
#define CARD_H          52U
#define CARD_GAP_X      10U
#define CARD_GAP_Y      5U
#define GRID_X0         17U
#define GRID_Y0         12U

static uint8_t cards[CARD_COUNT];
static uint8_t revealed[CARD_COUNT], matched[CARD_COUNT];
static int cursor, last_card_col, first_card, second_card, attempts, matches;
static int win_menu_sel; /* 0 = PLAY, 1 = EXIT */
static uint32_t mismatch_tick;
static int mismatch_wait;

static void draw_card_image(unsigned int px, unsigned int py, uint8_t img_id) {
    unsigned int img_x = px + (CARD_W - CARD_IMG_W) / 2U;
    unsigned int img_y = py + (CARD_H - CARD_IMG_H) / 2U;

    if (img_id < 8U) {
        GLCD_Bitmap(img_x, img_y, CARD_IMG_W, CARD_IMG_H,
                    (unsigned char *)wojack_bitmaps[img_id]);
    }
}

static void draw_memory_exit_btn(int selected) {
    px_draw_button(204U, 186U, 98U, 34U, selected);
    GLCD_SetBackColor(selected ? PX_SEL_MID : PX_STONE_MID);
    GLCD_SetTextColor(PX_TEXT_DARK);
    GLCD_DisplayString(8, 14, 1, (unsigned char *)"EXIT");
}

static void draw_card(int index) {
    int r = index / CARD_COLS;
    int c = index % CARD_COLS;
    unsigned int px = GRID_X0 + (unsigned int)c * (CARD_W + CARD_GAP_X);
    unsigned int py = GRID_Y0 + (unsigned int)r * (CARD_H + CARD_GAP_Y);
    int sel = (index == cursor);

    if (matched[index]) {
        px_draw_button(px, py, CARD_W, CARD_H, sel);
        px_rect(px + 2U, py + 2U, CARD_W - 4U, CARD_H - 4U, PX_BG_LIGHT);
        draw_card_image(px, py, cards[index]);
    } else if (revealed[index]) {
        px_draw_button(px, py, CARD_W, CARD_H, sel);
        px_rect(px + 2U, py + 2U, CARD_W - 4U, CARD_H - 4U, White);
        draw_card_image(px, py, cards[index]);
    } else {
        px_draw_button(px, py, CARD_W, CARD_H, sel);
        /* Pixel emblem on card back */
        px_rect(px + 24U, py + 18U, 16U, 16U, PX_OUTLINE);
        px_rect(px + 26U, py + 20U, 12U, 12U, sel ? PX_SEL_HI : PX_STONE_DARK);
    }
}

static void update_memory_hud(void) {
    char msg[28];
    px_rect(17U, 190U, 174U, 26U, PX_OUTLINE);
    px_rect(19U, 192U, 170U, 22U, PX_STONE_DARK);
    GLCD_SetBackColor(PX_STONE_DARK);
    GLCD_SetTextColor(PX_SEL_HI);
    sprintf(msg, "MOVES:%-2d  PAIRS:%d/6", attempts, matches);
    GLCD_DisplayString(25, 4, 0, (unsigned char *)msg);
}

static void memory_draw_win_buttons(void) {
    px_draw_button(84U, 92U, 152U, 32U, win_menu_sel == 0);
    GLCD_SetBackColor(win_menu_sel == 0 ? PX_SEL_MID : PX_STONE_MID);
    GLCD_SetTextColor(PX_TEXT_DARK);
    GLCD_DisplayString(4, 7, 1, (unsigned char *)" PLAY ");

    px_draw_button(84U, 140U, 152U, 32U, win_menu_sel == 1);
    GLCD_SetBackColor(win_menu_sel == 1 ? PX_SEL_MID : PX_STONE_MID);
    GLCD_SetTextColor(PX_TEXT_DARK);
    GLCD_DisplayString(6, 7, 1, (unsigned char *)" EXIT ");
}

static void memory_draw_win(void) {
    char msg[24];
    px_rect(54U, 36U, 212U, 152U, PX_OUTLINE);
    px_rect(56U, 38U, 208U, 148U, PX_STONE_DARK);
    px_rect(60U, 42U, 200U, 140U, PX_BG_LIGHT);

    GLCD_SetBackColor(PX_BG_LIGHT);
    GLCD_SetTextColor(PX_MOSS_DARK);
    GLCD_DisplayString(2, 6, 1, (unsigned char *)"YOU WIN!");

    GLCD_SetTextColor(PX_TEXT_DARK);
    sprintf(msg, "TOTAL MOVES: %d", attempts);
    GLCD_DisplayString(9, 19, 0, (unsigned char *)msg);

    win_menu_sel = 0;
    memory_draw_win_buttons();
}

static void memory_draw_initial(void) {
    int i;
    px_draw_grass_bg(8U);

    for (i = 0; i < CARD_COUNT; i++) {
        draw_card(i);
    }

    update_memory_hud();
    draw_memory_exit_btn(cursor == CURSOR_EXIT);
}

void Memory_Enter(void) {
    uint8_t pool[8] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    int i, j;

    rng_state ^= osKernelGetTickCount() + 0xA341316Cu;
    if (rng_state == 0U) {
        rng_state = 0x51A7C3D9u;
    }

    /* 1. Shuffle all 8 Wojack image indices */
    for (i = 7; i > 0; i--) {
        uint8_t t;
        j = rand_range(i + 1);
        t = pool[i];
        pool[i] = pool[j];
        pool[j] = t;
    }

    /* 2. Take the first 6 unique images and assign 2 cards per image */
    for (i = 0; i < 6; i++) {
        cards[i * 2]     = pool[i];
        cards[i * 2 + 1] = pool[i];
    }

    for (i = 0; i < CARD_COUNT; i++) {
        revealed[i] = 0;
        matched[i] = 0;
    }

    /* 3. Shuffle the 12 cards on the 4x3 grid */
    for (i = CARD_COUNT - 1; i > 0; i--) {
        uint8_t t;
        j = rand_range(i + 1);
        t = cards[i];
        cards[i] = cards[j];
        cards[j] = t;
    }

    cursor = 0;
    last_card_col = 0;
    first_card = -1;
    second_card = -1;
    attempts = 0;
    matches = 0;
    win_menu_sel = 0;
    mismatch_wait = 0;

    memory_draw_initial();
}

int Memory_Task(uint32_t pressed) {
    int next = cursor;

    if (matches == 6) {
        if ((pressed & JOYSTICK_UP) || (pressed & JOYSTICK_DOWN)) {
            win_menu_sel = !win_menu_sel;
            memory_draw_win_buttons();
            Audio_PlaySfx(SFX_NAV);
        } else if (pressed & JOYSTICK_CENTER) {
            Audio_PlaySfx(SFX_SELECT);
            if (win_menu_sel == 0) {
                Memory_Enter();
            } else {
                return 1;
            }
        }
        return 0;
    }

    if (mismatch_wait) {
        if ((uint32_t)(osKernelGetTickCount() - mismatch_tick) >= 800U) {
            int c1 = first_card;
            int c2 = second_card;
            revealed[c1] = 0;
            revealed[c2] = 0;
            first_card = second_card = -1;
            mismatch_wait = 0;
            draw_card(c1);
            draw_card(c2);
        }
        return 0;
    }

    if (cursor == CURSOR_EXIT) {
        if (pressed & JOYSTICK_UP) {
            next = (CARD_ROWS - 1) * CARD_COLS + last_card_col;
        } else if (pressed & JOYSTICK_DOWN) {
            next = last_card_col;
        }
    } else {
        if (pressed & JOYSTICK_UP) {
            if (cursor < CARD_COLS) {
                last_card_col = cursor % CARD_COLS;
                next = CURSOR_EXIT;
            } else {
                next = cursor - CARD_COLS;
            }
        } else if (pressed & JOYSTICK_DOWN) {
            if (cursor >= (CARD_ROWS - 1) * CARD_COLS) {
                last_card_col = cursor % CARD_COLS;
                next = CURSOR_EXIT;
            } else {
                next = cursor + CARD_COLS;
            }
        } else if (pressed & JOYSTICK_LEFT) {
            next = (cursor % CARD_COLS == 0) ? (cursor + CARD_COLS - 1) : (cursor - 1);
        } else if (pressed & JOYSTICK_RIGHT) {
            next = (cursor % CARD_COLS == CARD_COLS - 1) ? (cursor - CARD_COLS + 1) : (cursor + 1);
        }
    }

    if (next != cursor) {
        int prev = cursor;
        cursor = next;
        if (prev == CURSOR_EXIT) {
            draw_memory_exit_btn(0);
        } else {
            draw_card(prev);
        }
        if (cursor == CURSOR_EXIT) {
            draw_memory_exit_btn(1);
        } else {
            draw_card(cursor);
        }
        Audio_PlaySfx(SFX_NAV);
        return 0;
    }

    if (pressed & JOYSTICK_CENTER) {
        /* Clicking the on-screen EXIT button exits immediately */
        if (cursor == CURSOR_EXIT) {
            Audio_PlaySfx(SFX_SELECT);
            return 1;
        }

        if (matched[cursor] || revealed[cursor]) {
            return 0;
        }
        revealed[cursor] = 1;
        draw_card(cursor);
        Audio_PlaySfx(SFX_SELECT);

        if (first_card < 0) {
            first_card = cursor;
        } else {
            second_card = cursor;
            attempts++;
            if (cards[first_card] == cards[second_card]) {
                matched[first_card] = matched[second_card] = 1;
                matches++;
                draw_card(first_card);
                draw_card(second_card);
                first_card = second_card = -1;
                Audio_PlaySfx(SFX_MATCH);
            } else {
                mismatch_wait = 1;
                mismatch_tick = osKernelGetTickCount();
            }
            update_memory_hud();

            if (matches == 6) {
                Audio_PlaySfx(SFX_WIN);
                memory_draw_win();
            }
        }
    }

    return 0;
}