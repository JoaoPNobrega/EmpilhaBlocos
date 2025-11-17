#ifndef TETRIS_H
#define TETRIS_H

#include <stdbool.h>

enum { T_BOARD_W = 10, T_BOARD_H = 20, T_QUEUE_CAP = 64 };

typedef enum { T_I, T_O, T_T, T_S, T_Z, T_J, T_L, T_PIECE_COUNT } Tetromino;

typedef struct {
    int q[T_QUEUE_CAP];
    int head, tail, size;
} TQueue;

typedef struct {
    Tetromino type;
    int rot, x, y;
} TPiece;

typedef struct {
    unsigned char board[T_BOARD_H][T_BOARD_W];

    TPiece current;
    bool   has_current;
    bool   hold_locked;
    int    hold;

    TQueue queue;

    int score, lines_cleared, level;

    int gravity_ticks, gravity_step;
    int lock_ticks,   lock_max;

    bool game_over;

    /* limpeza com animação */
    bool clearing;
    int  clear_ticks;           /* decresce até 0 enquanto anima */
    unsigned char clear_row[T_BOARD_H];
    int  last_clear;            /* 1..4 */
    bool just_cleared;
} Tetris;

/* API */
void tetris_init(Tetris *t);
void tetris_reset(Tetris *t);
void tetris_free(Tetris *t);

bool tetris_step(Tetris *t);     /* avança tempo (gravidade/lock/clear) */

bool tetris_move(Tetris *t, int dx);
bool tetris_soft_drop(Tetris *t);
int  tetris_hard_drop(Tetris *t);
bool tetris_rotate(Tetris *t, int dir); /* +1 horário, -1 anti */
bool tetris_hold(Tetris *t);

/* helpers p/ UI */
unsigned char tetris_cell(const Tetris *t, int x, int y);
void tetris_iter_blocks(Tetromino type, int rot, int base_x, int base_y,
                        void (*cb)(int gx,int gy, void*), void *user);
int  tetris_ghost_y(const Tetris *t);
int  tetris_peek_next(const Tetris *t, int index);
int  tetris_get_hold(const Tetris *t);

int  tetris_last_clear(const Tetris *t);
bool tetris_consume_clear_event(Tetris *t);
bool tetris_is_clearing(const Tetris *t);
bool tetris_row_marked(const Tetris *t, int y);

/* NOVO: progresso (0..1) da animação de limpeza atual */
float tetris_clear_progress(const Tetris *t);

#endif
