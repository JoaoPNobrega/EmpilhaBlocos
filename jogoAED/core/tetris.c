#include "tetris.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
static int t_min(int a,int b){return a<b?a:b;}
static int t_max(int a,int b){return a>b?a:b;}

/* -------------------- Tetrominós -------------------- */
static const unsigned short SHAPES[T_PIECE_COUNT][4] = {
    { 0x0F00, 0x2222, 0x00F0, 0x4444 }, // I
    { 0x6600, 0x6600, 0x6600, 0x6600 }, // O
    { 0x4E00, 0x4640, 0x0E40, 0x4C40 }, // T
    { 0x6C00, 0x4620, 0x06C0, 0x8C40 }, // S
    { 0xC600, 0x2640, 0x0C60, 0x4C80 }, // Z
    { 0x8E00, 0x6440, 0x0E20, 0x44C0 }, // J
    { 0x2E00, 0x4460, 0x0E80, 0xC440 }  // L
};
static int shape_at(Tetromino t, int rot, int x, int y){
    unsigned short m = SHAPES[t][(rot & 3)];
    return (m >> (15 - (y*4 + x))) & 1;
}

/* ---- SRS tables ---- */
typedef struct { int dx,dy; } Kick;
typedef struct { Kick k[5]; } SRSRow;
static const SRSRow SRS_JLSTZ[8] = {
    { { { 0, 0}, {-1, 0}, {-1, 1}, { 0,-2}, {-1,-2} } },
    { { { 0, 0}, { 1, 0}, { 1,-1}, { 0, 2}, { 1, 2} } },
    { { { 0, 0}, { 1, 0}, { 1,-1}, { 0, 2}, { 1, 2} } },
    { { { 0, 0}, {-1, 0}, {-1, 1}, { 0,-2}, {-1,-2} } },
    { { { 0, 0}, { 1, 0}, { 1, 1}, { 0,-2}, { 1,-2} } },
    { { { 0, 0}, {-1, 0}, {-1,-1}, { 0, 2}, {-1, 2} } },
    { { { 0, 0}, {-1, 0}, {-1,-1}, { 0, 2}, {-1, 2} } },
    { { { 0, 0}, { 1, 0}, { 1, 1}, { 0,-2}, { 1,-2} } },
};
static const SRSRow SRS_I[8] = {
    { { { 0, 0}, {-2, 0}, { 1, 0}, {-2,-1}, { 1, 2} } },
    { { { 0, 0}, { 2, 0}, {-1, 0}, { 2, 1}, {-1,-2} } },
    { { { 0, 0}, {-1, 0}, { 2, 0}, {-1, 2}, { 2,-1} } },
    { { { 0, 0}, { 1, 0}, {-2, 0}, { 1,-2}, {-2, 1} } },
    { { { 0, 0}, { 2, 0}, {-1, 0}, { 2, 1}, {-1,-2} } },
    { { { 0, 0}, {-2, 0}, { 1, 0}, {-2,-1}, { 1, 2} } },
    { { { 0, 0}, { 1, 0}, {-2, 0}, { 1,-2}, {-2, 1} } },
    { { { 0, 0}, {-1, 0}, { 2, 0}, {-1, 2}, { 2,-1} } },
};

/* -------------------- 7-bag / fila -------------------- */
static void q_clear(TQueue *q){ q->head=q->tail=q->size=0; }
static int  q_push(TQueue *q,int v){ if(q->size>=T_QUEUE_CAP) return 0; q->q[q->tail]=v; q->tail=(q->tail+1)%T_QUEUE_CAP; q->size++; return 1; }
static int  q_pop (TQueue *q,int *o){ if(q->size==0) return 0; if(o)*o=q->q[q->head]; q->head=(q->head+1)%T_QUEUE_CAP; q->size--; return 1; }
static int  q_peek(const TQueue*q,int i){ if(i<0||i>=q->size) return -1; int idx=(q->head+i)%T_QUEUE_CAP; return q->q[idx]; }
static void shuffle7(int *v){ for(int i=0;i<7;i++) v[i]=i; for(int i=6;i>0;i--){ int j=rand()%(i+1); int t=v[i]; v[i]=v[j]; v[j]=t; } }
static void refill_bag(TQueue *q){ int bag[7]; shuffle7(bag); for(int i=0;i<7;i++) q_push(q,bag[i]); }

/* -------------------- Colisão / util -------------------- */
static int test_collision(const Tetris *t, Tetromino typ, int rot, int px, int py){
    for(int y=0;y<4;y++) for(int x=0;x<4;x++){
        if(!shape_at(typ,rot,x,y)) continue;
        int gx=px+x, gy=py+y;
        if(gx<0 || gx>=T_BOARD_W || gy<0 || gy>=T_BOARD_H) return 1;
        if(t->board[gy][gx]) return 1;
    }
    return 0;
}
static int on_ground(const Tetris *t){
    if(!t->has_current) return 0;
    return test_collision(t, t->current.type, t->current.rot, t->current.x, t->current.y+1);
}
static void lock_piece(Tetris *t){
    Tetromino typ=t->current.type; int rot=t->current.rot;
    int px=t->current.x, py=t->current.y;
    for(int y=0;y<4;y++) for(int x=0;x<4;x++){
        if(!shape_at(typ,rot,x,y)) continue;
        int gx=px+x, gy=py+y;
        if(gy>=0 && gy<T_BOARD_H && gx>=0 && gx<T_BOARD_W)
            t->board[gy][gx]=(unsigned char)(typ+1);
    }
}
static int scan_full_lines(Tetris *t){
    int count=0;
    memset(t->clear_row,0,sizeof(t->clear_row));
    for(int y=0;y<T_BOARD_H;y++){
        int filled=1;
        for(int x=0;x<T_BOARD_W;x++){ if(t->board[y][x]==0){ filled=0; break; } }
        if(filled){ t->clear_row[y]=1; count++; }
    }
    return count;
}
static void actually_clear_marked(Tetris *t){
    unsigned char new_board[T_BOARD_H][T_BOARD_W];
    int write = T_BOARD_H - 1;
    for (int y = T_BOARD_H - 1; y >= 0; y--) {
        if (!t->clear_row[y]) {
            memcpy(new_board[write], t->board[y], T_BOARD_W);
            write--;
        }
    }
    for (int y = write; y >= 0; y--) memset(new_board[y], 0, T_BOARD_W);
    memcpy(t->board, new_board, sizeof(new_board));
    memset(t->clear_row, 0, sizeof(t->clear_row));
}

/* -------------------- Pontuação / nível -------------------- */
static void add_score(Tetris *t, int lines, int drop){
    static const int base[5]={0,100,300,500,800};
    t->score += base[t_min(t_max(lines,0),4)];
    t->score += drop;
    t->lines_cleared += lines;
    t->level = 1 + t->lines_cleared/10;
    t->gravity_step = (t->level < 15) ? t_max(6, 48 - (t->level - 1) * 3) : 6;
}

/* -------------------- Spawn / rotação SRS -------------------- */
static void spawn_next(Tetris *t){
    if(t->queue.size < 7) refill_bag(&t->queue);
    int v; q_pop(&t->queue,&v);
    t->current.type=(Tetromino)v;
    t->current.rot=0; t->current.x=3; t->current.y=0;
    t->hold_locked=false; t->has_current=true;
    t->lock_ticks=0;
    if(test_collision(t,t->current.type,t->current.rot,t->current.x,t->current.y)){
        t->game_over=true; t->has_current=false;
    }
}
static bool try_rotate_with_kicks(Tetris *t, int dir){
    Tetromino typ=t->current.type;
    int r0=t->current.rot, r1=(r0 + (dir>0?1:3)) & 3;
    if(typ==T_O){
        if(!test_collision(t,typ,r1,t->current.x,t->current.y)){
            t->current.rot=r1; t->lock_ticks=0; return true;
        }
        return false;
    }
    static const int map[4][2]={{0,7},{2,1},{4,3},{6,5}};
    const SRSRow *table = (typ==T_I) ? SRS_I : SRS_JLSTZ;
    int row_index = (dir>0)? map[r0][0] : map[r0][1];
    for(int i=0;i<5;i++){
        int nx = t->current.x + table[row_index].k[i].dx;
        int ny = t->current.y + table[row_index].k[i].dy;
        if(!test_collision(t,typ,r1,nx,ny)){
            t->current.rot=r1; t->current.x=nx; t->current.y=ny;
            t->lock_ticks=0; // move reset
            return true;
        }
    }
    return false;
}

/* -------------------- API -------------------- */
void tetris_init(Tetris *t){
    memset(t,0,sizeof(*t));
    t->hold=-1; q_clear(&t->queue);
    srand((unsigned)time(NULL)); refill_bag(&t->queue);
    t->gravity_step=16; t->lock_max=30; t->level=1;
    t->clearing=false; t->clear_ticks=0; t->last_clear=0; t->just_cleared=false;
    spawn_next(t);
}
void tetris_reset(Tetris *t){
    for(int y=0;y<T_BOARD_H;y++) memset(t->board[y],0,T_BOARD_W);
    t->score=0; t->lines_cleared=0; t->level=1;
    t->game_over=false; t->hold=-1; t->hold_locked=false;
    t->lock_ticks=0; q_clear(&t->queue); refill_bag(&t->queue);
    t->clearing=false; t->clear_ticks=0; t->last_clear=0; t->just_cleared=false;
    spawn_next(t);
}
void tetris_free(Tetris *t){ (void)t; }

bool tetris_step(Tetris *t){
    if(t->game_over) return false;

    if(t->clearing){
        if(--t->clear_ticks<=0){
            actually_clear_marked(t);
            add_score(t, t->last_clear, 0);
            t->clearing=false;
            t->just_cleared=true;
            spawn_next(t);
        }
        return true;
    }

    if(!t->has_current) return false;

    t->gravity_ticks++;
    if(t->gravity_ticks >= t->gravity_step){
        t->gravity_ticks=0;
        if(!test_collision(t,t->current.type,t->current.rot,t->current.x,t->current.y+1)){
            t->current.y++;
        }
    }

    if(on_ground(t)){
        if(++t->lock_ticks >= t->lock_max){
            lock_piece(t);
            int cnt = scan_full_lines(t);
            t->last_clear = cnt;
            if (cnt > 0) {
                t->has_current = false;
                t->clearing    = true;
                t->clear_ticks = 12;  /* duração da animação */
            } else {
                add_score(t, 0, 0);
                spawn_next(t);
            }
            return true;
        }
    } else {
        t->lock_ticks=0;
    }
    return false;
}

bool tetris_move(Tetris *t, int dx){
    if(t->game_over || !t->has_current || t->clearing) return false;
    int was_ground = on_ground(t);
    int nx=t->current.x + dx;
    if(!test_collision(t,t->current.type,t->current.rot,nx,t->current.y)){
        t->current.x=nx; if(was_ground) t->lock_ticks=0; return true;
    }
    return false;
}
bool tetris_soft_drop(Tetris *t){
    if(t->game_over || !t->has_current || t->clearing) return false;
    if(!test_collision(t,t->current.type,t->current.rot,t->current.x,t->current.y+1)){
        t->current.y++; t->score+=1; return true;
    }
    return false;
}
int tetris_hard_drop(Tetris *t){
    if(t->game_over || !t->has_current || t->clearing) return 0;
    int drop=0;
    while(!test_collision(t,t->current.type,t->current.rot,t->current.x,t->current.y+1)){
        t->current.y++; drop++;
    }
    lock_piece(t);
    int cnt = scan_full_lines(t);
    t->last_clear = cnt;
    if (cnt > 0) {
        t->has_current = false;
        t->clearing    = true;
        t->clear_ticks = 12;
    } else {
        add_score(t, 0, drop);
        spawn_next(t);
    }
    return drop;
}
bool tetris_rotate(Tetris *t, int dir){
    if(t->game_over || !t->has_current || t->clearing) return false;
    return try_rotate_with_kicks(t, dir);
}
bool tetris_hold(Tetris *t){
    if(t->game_over || !t->has_current || t->hold_locked || t->clearing) return false;
    Tetromino cur=t->current.type;
    if(t->hold<0){ t->hold=cur; spawn_next(t); }
    else{
        Tetromino tmp=t->hold; t->hold=cur;
        t->current.type=tmp; t->current.rot=0; t->current.x=3; t->current.y=0;
        if(test_collision(t,t->current.type,t->current.rot,t->current.x,t->current.y)){
            t->game_over=true; t->has_current=false;
        }
    }
    t->hold_locked=true; t->lock_ticks=0;
    return true;
}

/* -------------------- Helpers p/ UI -------------------- */
unsigned char tetris_cell(const Tetris *t, int x, int y){
    if(x<0||x>=T_BOARD_W||y<0||y>=T_BOARD_H) return 255;
    return t->board[y][x];
}
static void iter_cb(Tetromino type,int rot,int bx,int by,void (*cb)(int,int,void*),void*ud){
    for(int y=0;y<4;y++) for(int x=0;x<4;x++)
        if(shape_at(type,rot,x,y)) cb(bx+x,by+y,ud);
}
void tetris_iter_blocks(Tetromino type, int rot, int base_x, int base_y,
                        void (*cb)(int gx,int gy, void*), void *user){
    if(!cb) return; iter_cb(type,rot,base_x,base_y,cb,user);
}
int tetris_ghost_y(const Tetris *t){
    if(!t->has_current) return 0;
    int y=t->current.y; while(!test_collision(t,t->current.type,t->current.rot,t->current.x,y+1)) y++;
    return y;
}
int tetris_peek_next(const Tetris *t, int index){
    if(index<0) return -1; if(t->queue.size<index+1) return -1; return q_peek(&t->queue,index);
}
int tetris_get_hold(const Tetris *t){ return t->hold; }
int  tetris_last_clear(const Tetris *t){ return t->last_clear; }
bool tetris_consume_clear_event(Tetris *t){ bool v=t->just_cleared; t->just_cleared=false; return v; }
bool tetris_is_clearing(const Tetris *t){ return t->clearing; }
bool tetris_row_marked(const Tetris *t, int y){ return (y>=0 && y<T_BOARD_H)? (t->clear_row[y]!=0) : false; }

/* NOVO: 0..1 do progresso da animação de limpeza */
float tetris_clear_progress(const Tetris *t){
    if(!t->clearing) return 0.f;
    float p = (12.0f - (float)t->clear_ticks) / 12.0f;
    if(p<0.f) p=0.f; if(p>1.f) p=1.f;
    return p;
}
