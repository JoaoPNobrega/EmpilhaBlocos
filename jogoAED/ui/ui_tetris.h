#ifndef UI_TETRIS_H
#define UI_TETRIS_H

#include <stdbool.h>
#include "../core/tetris.h"

/* Janela */
enum { T_WIN_W = 900, T_WIN_H = 720 };

/* HOME */
#define HOME_PLAY_W 300
#define HOME_PLAY_H 76
#define HOME_PLAY_X ((T_WIN_W/2) - (HOME_PLAY_W/2))
#define HOME_PLAY_Y 300          /* um pouco mais baixo */

/* HOME — Instruções (abaixo do título) */
#define HOME_HELP_Y 180          /* << ajuste aqui a altura das instruções */

/* NAME */
#define NAME_BOX_W   560
#define NAME_BOX_H    80
#define NAME_BOX_X   ((T_WIN_W/2) - (NAME_BOX_W/2))
#define NAME_BOX_Y   ((T_WIN_H/2) - (NAME_BOX_H/2))
#define NAME_START_W  260
#define NAME_START_H   56
#define NAME_START_X  ((T_WIN_W/2) - (NAME_START_W/2))
#define NAME_START_Y  (NAME_BOX_Y + NAME_BOX_H + 36)
#define NAME_BACK_W  140
#define NAME_BACK_H   44
#define NAME_BACK_X   24
#define NAME_BACK_Y   16

/* GAME: botão REINICIAR (EXATAMENTE AQUI vc muda a ALTURA) */
#define GAME_BTN_W  180
#define GAME_BTN_H   42
#define GAME_BTN_X  (T_WIN_W - 20 - GAME_BTN_W)
#define GAME_BTN_Y  420         /* << Mude este valor para ajustar a altura */

/* GAME OVER */
#define GO_BOX_W   560
#define GO_BOX_H   300
#define GO_BOX_X   ((T_WIN_W/2) - (GO_BOX_W/2))
#define GO_BOX_Y   ((T_WIN_H/2) - (GO_BOX_H/2))
#define GO_GAP       16
#define GO_TRY_W    300
#define GO_TRY_H     56
#define GO_MENU_W   220
#define GO_MENU_H    56
#define GO_TRY_X   (GO_BOX_X + (GO_BOX_W - (GO_TRY_W + GO_GAP + GO_MENU_W)) / 2)
#define GO_MENU_X  (GO_TRY_X + GO_TRY_W + GO_GAP)
#define GO_TRY_Y   (GO_BOX_Y + GO_BOX_H - GO_TRY_H - 28)
#define GO_MENU_Y   GO_TRY_Y

typedef struct UiT UiT;

UiT* ui_t_init(const char *title);
void ui_t_shutdown(UiT *ui);
void ui_t_begin(UiT *ui);
void ui_t_end(UiT *ui);

void ui_t_draw_home(UiT *ui, const char *title);
void ui_t_draw_home_play_button(UiT *ui, bool hover);
void ui_t_draw_home_ranking(UiT *ui, const char *const names[], const int scores[], int count);

void ui_t_draw_name_input(UiT *ui, const char *typed);

void ui_t_draw_game(UiT *ui, const Tetris *t,
                    const char *player_name, int score,
                    const int next3_ids[3], const char *banner, int banner_alpha);

void ui_t_draw_gameover(UiT *ui, const Tetris *t, const char *player_name);

#endif
