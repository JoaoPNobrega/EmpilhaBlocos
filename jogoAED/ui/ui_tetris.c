#include "ui_tetris.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* --------- LAYOUT/STATE --------- */
struct UiT {
    SDL_Window   *win;
    SDL_Renderer *ren;
    TTF_Font     *font_title;
    TTF_Font     *font_big;
    TTF_Font     *font_med;
    TTF_Font     *font_small;
    int cell;
    int field_x, field_y;
};

/* --------- UTIL --------- */
static TTF_Font* try_open(const char *p, int sz){
    TTF_Font *f = TTF_OpenFont(p, sz);
    if (f) return f;
    f = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", sz);
    if (f) return f;
    f = TTF_OpenFont("C:\\Windows\\Fonts\\segoeui.ttf", sz);
    return f;
}
static void draw_text(SDL_Renderer *r, TTF_Font *font, const char *txt, int x, int y, SDL_Color c){
    if(!font || !txt) return;
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, c); if(!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst = {x, y, s->w, s->h};
    SDL_RenderCopy(r, t, NULL, &dst);
    SDL_DestroyTexture(t); SDL_FreeSurface(s);
}
static void text_size(TTF_Font *font, const char *txt, int *w, int *h){
    int tw=0, th=0; if(font && txt) TTF_SizeUTF8(font, txt, &tw, &th);
    if(w) *w=tw; if(h) *h=th;
}

/* Fundo: gradiente + listras diagonais suaves (sem “piscar”) */
static void draw_bg(SDL_Renderer *r){
    int w,h; SDL_GetRendererOutputSize(r,&w,&h);
    for(int y=0;y<h;y++){
        float t = (float)y/(float)(h-1);
        Uint8 R = (Uint8)(22 + 20*t);
        Uint8 G = (Uint8)(30 + 36*t);
        Uint8 B = (Uint8)(44 + 60*t);
        SDL_SetRenderDrawColor(r,R,G,B,255);
        SDL_RenderDrawLine(r,0,y,w,y);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    Uint32 ms = SDL_GetTicks();
    float offset = fmodf(ms*0.015f, 48.f);
    SDL_SetRenderDrawColor(r,255,255,255,14);
    for(float d=-240; d<w+h; d+=48){
        float x1 = d+offset, y1 = 0;
        float x2 = d - h + offset, y2 = h;
        SDL_RenderDrawLine(r,(int)x1,(int)y1,(int)x2,(int)y2);
    }
}

/* Retângulo arredondado com borda 2px e leve highlight interno */
static void round_fill(SDL_Renderer *r, SDL_Rect rc, int rad, SDL_Color bg, SDL_Color bd){
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    if(rad<6) rad=6;

    // preenchimento
    SDL_SetRenderDrawColor(r, bg.r,bg.g,bg.b,bg.a);
    SDL_Rect core = { rc.x+rad, rc.y, rc.w-2*rad, rc.h };
    SDL_RenderFillRect(r,&core);
    core = (SDL_Rect){ rc.x, rc.y+rad, rc.w, rc.h-2*rad };
    SDL_RenderFillRect(r,&core);

    // borda 2px
    SDL_SetRenderDrawColor(r, bd.r,bd.g,bd.b,bd.a);
    for(int i=0;i<2;i++){
        SDL_Rect b = { rc.x+i, rc.y+i, rc.w-2*i, rc.h-2*i };
        SDL_RenderDrawRect(r, &b);
    }

    // highlight superior suave
    SDL_SetRenderDrawColor(r, 255,255,255, 22);
    SDL_RenderDrawLine(r, rc.x+8, rc.y+3, rc.x+rc.w-8, rc.y+3);
}

/* Botão arredondado com hover e auto-fit de fonte */
static void button_fit(SDL_Renderer *r,
                       TTF_Font *font_big, TTF_Font *font_med, TTF_Font *font_small,
                       SDL_Rect rc, SDL_Color base, SDL_Color border,
                       const char *label, SDL_Color fg)
{
    int mx,my; SDL_GetMouseState(&mx,&my);
    const bool hover = (mx>=rc.x && mx<=rc.x+rc.w && my>=rc.y && my<=rc.y+rc.h);
    SDL_Color bg = base;
    if(hover){ bg.r=(Uint8)fminf(255,bg.r+16); bg.g=(Uint8)fminf(255,bg.g+16); bg.b=(Uint8)fminf(255,bg.b+16); }
    round_fill(r, rc, 14, bg, border);

    const int pad=16; TTF_Font *use=font_big; int tw,th; text_size(use,label,&tw,&th);
    if(tw>rc.w-2*pad){ use=font_med; text_size(use,label,&tw,&th); }
    if(tw>rc.w-2*pad){ use=font_small; text_size(use,label,&tw,&th); }
    int tx=rc.x+(rc.w-tw)/2, ty=rc.y+(rc.h-th)/2;
    draw_text(r,use,label,tx,ty,fg);
}

/* Cores de texto */
static SDL_Color FG_LIGHT = {245,245,250,255};
static SDL_Color FG_DIM   = {225,230,240,255};
static SDL_Color FG_DARK  = { 34, 36, 42,255};

/* Peças com cores mais vibrantes */
static SDL_Color color_for(int id){
    switch(id){
        case 1: return (SDL_Color){  0,255,255,255}; // I
        case 2: return (SDL_Color){255,230,  0,255}; // O
        case 3: return (SDL_Color){200, 80,255,255}; // T
        case 4: return (SDL_Color){ 60,230, 80,255}; // S
        case 5: return (SDL_Color){255, 70, 70,255}; // Z
        case 6: return (SDL_Color){ 70,100,255,255}; // J
        case 7: return (SDL_Color){255,170, 40,255}; // L
        default:return (SDL_Color){ 40, 40, 45,255};
    }
}

/* --------- INIT / FRAME --------- */
UiT* ui_t_init(const char *title){
    if(SDL_Init(SDL_INIT_VIDEO)!=0) return NULL;
    if(TTF_Init()!=0){ SDL_Quit(); return NULL; }
    SDL_Window *w = SDL_CreateWindow(title?title:"Empilha Blocos",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, T_WIN_W, T_WIN_H, SDL_WINDOW_SHOWN);
    if(!w){ TTF_Quit(); SDL_Quit(); return NULL; }
    SDL_Renderer *r = SDL_CreateRenderer(w,-1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!r){ SDL_DestroyWindow(w); TTF_Quit(); SDL_Quit(); return NULL; }
    UiT *ui = (UiT*)SDL_calloc(1,sizeof(UiT));
    ui->win=w; ui->ren=r;
    ui->font_title = try_open("arial.ttf", 64);
    ui->font_big   = try_open("arial.ttf", 32);
    ui->font_med   = try_open("arial.ttf", 22);
    ui->font_small = try_open("arial.ttf", 16);
    ui->cell=28; ui->field_x=60; ui->field_y=(T_WIN_H-(20*ui->cell))/2;
    return ui;
}
void ui_t_shutdown(UiT *ui){
    if(!ui) return;
    if(ui->font_title) TTF_CloseFont(ui->font_title);
    if(ui->font_big)   TTF_CloseFont(ui->font_big);
    if(ui->font_med)   TTF_CloseFont(ui->font_med);
    if(ui->font_small) TTF_CloseFont(ui->font_small);
    SDL_DestroyRenderer(ui->ren); SDL_DestroyWindow(ui->win);
    TTF_Quit(); SDL_Quit();
}
void ui_t_begin(UiT *ui){ draw_bg(ui->ren); }
void ui_t_end(UiT *ui){ SDL_RenderPresent(ui->ren); }

/* --------- TÍTULO CENTRAL --------- */
static void title_center(SDL_Renderer *r, TTF_Font *font, const char *text, int y){
    int total=0, th=0;
    for(int i=0;text[i];i++){ int w=0,h=0; char ch[5]={text[i],0}; TTF_SizeUTF8(font,ch,&w,&h); total+= (text[i]==' ')? 12 : w; th=h; }
    int x = (T_WIN_W-total)/2;
    SDL_Color pal[6]={{255,96,96,255},{255,184,96,255},{252,226,96,255},{96,214,140,255},{96,178,255,255},{180,110,255,255}};
    for(int i=0;text[i];i++){
        char buf[5]={text[i],0}; if(text[i]==' '){ x+=12; continue; }
        SDL_Surface *s=TTF_RenderUTF8_Blended(font,buf,pal[i%6]); if(!s) continue;
        SDL_Texture *t=SDL_CreateTextureFromSurface(r,s);
        SDL_Rect dst={x,y,s->w,s->h}; SDL_RenderCopy(r,t,NULL,&dst); x+=s->w;
        SDL_DestroyTexture(t); SDL_FreeSurface(s);
    }
}

/* --------- HOME --------- */
void ui_t_draw_home(UiT *ui, const char *title){
    title_center(ui->ren, ui->font_title, title, 80);

    /* Instruções mais abaixo e organizadas em “blocos” */
    SDL_Rect help = { (T_WIN_W/2)-360, HOME_HELP_Y, 720, 70 };
    round_fill(ui->ren, help, 12, (SDL_Color){250,250,255,40}, (SDL_Color){220,225,235,80});

    draw_text(ui->ren, ui->font_med, "Mover: A / D    Descer: S    Girar: E / Q", help.x+24, help.y+14, FG_LIGHT);
    draw_text(ui->ren, ui->font_med, "Hard Drop: ESPAÇO    Hold: C    Sair: ESC",  help.x+24, help.y+38, FG_LIGHT);
}
void ui_t_draw_home_play_button(UiT *ui, bool hover){
    (void)hover;
    SDL_Rect btn = { HOME_PLAY_X, HOME_PLAY_Y, HOME_PLAY_W, HOME_PLAY_H };
    button_fit(ui->ren, ui->font_big, ui->font_med, ui->font_small,
               btn, (SDL_Color){34,172,98,230}, (SDL_Color){18,110,58,255},
               "JOGAR", (SDL_Color){255,255,255,255});
}
/* Ranking — Top 5 somente */
void ui_t_draw_home_ranking(UiT *ui, const char *const names[], const int scores[], int count){
    SDL_Rect rc = { (T_WIN_W/2)-340, HOME_PLAY_Y + HOME_PLAY_H + 36, 680, 260 };
    round_fill(ui->ren, rc, 12, (SDL_Color){250,250,255,40}, (SDL_Color){220,225,235,80});
    int tw, th; text_size(ui->font_big, "RANKING", &tw, &th);
    draw_text(ui->ren, ui->font_big, "RANKING", rc.x + (rc.w - tw)/2, rc.y+16, FG_LIGHT);

    int y0 = rc.y + 64;
    draw_text(ui->ren, ui->font_med, "Pos.",    rc.x+40,  y0, FG_DIM);
    draw_text(ui->ren, ui->font_med, "Jogador", rc.x+140, y0, FG_DIM);
    draw_text(ui->ren, ui->font_med, "Score",   rc.x+rc.w-120, y0, FG_DIM);

    int y = y0 + 28;
    int max = count < 5 ? count : 5;    /* Top 5 */
    for(int i=0;i<max;i++){
        char pos[8]; snprintf(pos,sizeof(pos), "%d", i+1);
        char sc[32]; snprintf(sc,sizeof(sc), "%d", scores[i]);
        draw_text(ui->ren, ui->font_med, pos, rc.x+40, y, FG_LIGHT);
        draw_text(ui->ren, ui->font_med, names[i]?names[i]:"-", rc.x+140, y, FG_LIGHT);
        draw_text(ui->ren, ui->font_med, sc, rc.x+rc.w-120, y, FG_LIGHT);
        y += 28;
    }
}

/* --------- NAME --------- */
void ui_t_draw_name_input(UiT *ui, const char *typed){
    title_center(ui->ren, ui->font_title, "Empilha Blocos", 80);
    int tw, th; const char *cap="DIGITE SEU NOME";
    text_size(ui->font_big, cap, &tw, &th);
    draw_text(ui->ren, ui->font_big, cap, (T_WIN_W/2)-(tw/2), 150, FG_LIGHT);

    SDL_Rect boxRc = { NAME_BOX_X, NAME_BOX_Y, NAME_BOX_W, NAME_BOX_H };
    round_fill(ui->ren, boxRc, 12, (SDL_Color){255,255,255,235}, (SDL_Color){180,185,195,255});
    const char *show = (typed && typed[0]) ? typed : "(seu nome aqui)";
    text_size(ui->font_big, show, &tw, &th);
    draw_text(ui->ren, ui->font_big, show, NAME_BOX_X+20, NAME_BOX_Y+(NAME_BOX_H-th)/2,
              (SDL_Color){ typed && typed[0] ? 40:140, typed && typed[0] ? 45:140, typed && typed[0] ? 50:150, 255 });

    SDL_Rect start = { NAME_START_X, NAME_START_Y, NAME_START_W, NAME_START_H };
    button_fit(ui->ren, ui->font_big, ui->font_med, ui->font_small,
               start, (SDL_Color){34,172,98,230}, (SDL_Color){18,110,58,255},
               "INICIAR", (SDL_Color){255,255,255,255});

    SDL_Rect back = { NAME_BACK_X, NAME_BACK_Y, NAME_BACK_W, NAME_BACK_H };
    button_fit(ui->ren, ui->font_med, ui->font_small, ui->font_small,
               back, (SDL_Color){92,116,148,230}, (SDL_Color){60,80,110,255},
               "VOLTAR", (SDL_Color){255,255,255,255});
}

/* --------- JOGO --------- */
static void draw_cell(SDL_Renderer *r, int cx,int cy,int cell,int ox,int oy, SDL_Color col, int a){
    SDL_Rect d={ox+cx*cell, oy+cy*cell, cell, cell};
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r,col.r,col.g,col.b,a); SDL_RenderFillRect(r,&d);
    SDL_SetRenderDrawColor(r,220,225,230,255); SDL_RenderDrawRect(r,&d);
}
static void iter_cb(int gx,int gy, void *ud){
    struct { SDL_Renderer *r; int cell, ox, oy, id, alpha; } *P=ud;
    draw_cell(P->r, gx, gy, P->cell, P->ox, P->oy, color_for(P->id), P->alpha);
}

void ui_t_draw_game(UiT *ui, const Tetris *t,
                    const char *player_name, int score,
                    const int next3_ids[3], const char *banner, int banner_alpha)
{
    /* barra topo info */
    SDL_Rect top={12,10,T_WIN_W-24,56};
    round_fill(ui->ren, top, 10, (SDL_Color){250,250,255,40}, (SDL_Color){210,215,220,80});
    title_center(ui->ren, ui->font_big, "Empilha Blocos", top.y+12);
    char left[160]; snprintf(left,sizeof(left),"Jogador: %s   Score: %d", player_name?player_name:"Player", score);
    draw_text(ui->ren, ui->font_med, left, 26, top.y+18, FG_LIGHT);

    /* botão reiniciar — posição por macro GAME_BTN_Y (ver .h) */
    SDL_Rect btr = { GAME_BTN_X, GAME_BTN_Y, GAME_BTN_W, GAME_BTN_H };
    button_fit(ui->ren, ui->font_med, ui->font_small, ui->font_small,
               btr, (SDL_Color){34,172,98,230}, (SDL_Color){18,110,58,255},
               "REINICIAR", (SDL_Color){255,255,255,255});

    /* painel de próximas — MAIS ALTO (cresce para baixo) */
    SDL_Rect pvBox={T_WIN_W-230, 110, 210, 300};
    round_fill(ui->ren, pvBox, 12, (SDL_Color){250,250,255,40}, (SDL_Color){200,205,210,80});
    draw_text(ui->ren, ui->font_med, "Próximas", pvBox.x+62, pvBox.y-24, FG_LIGHT);
    int pvy = pvBox.y + 18;
    for(int i=0;i<3;i++){
        int id = next3_ids? next3_ids[i] : 0; if(id<=0) continue;
        int cellMini = 20;
        int ox = pvBox.x + (pvBox.w - 4*cellMini)/2;
        int oy = pvy;
        struct { SDL_Renderer *r; int cell, ox, oy, id, alpha; } P = { ui->ren, cellMini, ox, oy, id, 255 };
        tetris_iter_blocks((Tetromino)(id-1), 0, 0, 0, (void(*)(int,int,void*))iter_cb, &P);
        pvy += 4*cellMini + 18;
    }

    /* campo com moldura */
    SDL_Rect frame={ui->field_x-8, ui->field_y-8, 10*ui->cell+16, 20*ui->cell+16};
    round_fill(ui->ren, frame, 12, (SDL_Color){250,250,255,40}, (SDL_Color){200,205,210,80});
    SDL_Rect field={ui->field_x, ui->field_y, 10*ui->cell, 20*ui->cell};
    SDL_SetRenderDrawColor(ui->ren, 28,32,40,220); SDL_RenderFillRect(ui->ren,&field);

    /* células + animação sweep de limpeza */
    for(int y=0;y<T_BOARD_H;y++){
        int marked = tetris_row_marked(t,y);
        float prog  = tetris_clear_progress(t); // 0..1
        for(int x=0;x<T_BOARD_W;x++){
            int id = t->board[y][x];
            int a = id?255:18;
            if(marked){ a = 160; }
            draw_cell(ui->ren, x,y, ui->cell, ui->field_x, ui->field_y, color_for(id), a);
        }
        if(marked){
            SDL_SetRenderDrawBlendMode(ui->ren, SDL_BLENDMODE_BLEND);
            int sweep_w = (int)(ui->cell * 10 * prog);
            SDL_Rect sweep = { ui->field_x, ui->field_y + y*ui->cell, sweep_w, ui->cell };
            SDL_SetRenderDrawColor(ui->ren, 255,255,255, (Uint8)(90 + 120*prog));
            SDL_RenderFillRect(ui->ren, &sweep);
        }
    }

    /* peça atual + fantasma */
    if(t->has_current){
        int gy = tetris_ghost_y(t);
        struct { SDL_Renderer *r; int cell, ox, oy, id, alpha; } G = { ui->ren, ui->cell, ui->field_x, ui->field_y, (int)t->current.type+1, 70 };
        tetris_iter_blocks(t->current.type, t->current.rot, t->current.x, gy, (void(*)(int,int,void*))iter_cb, &G);
        struct { SDL_Renderer *r; int cell, ox, oy, id, alpha; } C = { ui->ren, ui->cell, ui->field_x, ui->field_y, (int)t->current.type+1, 255 };
        tetris_iter_blocks(t->current.type, t->current.rot, t->current.x, t->current.y, (void(*)(int,int,void*))iter_cb, &C);
    }

    /* banner */
    if(banner && banner_alpha>0){
        int bw,bh; text_size(ui->font_big,banner,&bw,&bh);
        int bgw=bw+72; if(bgw<220) bgw=220;
        SDL_Rect bg={ ui->field_x + (10*ui->cell - bgw)/2, ui->field_y - 48, bgw, 40 };
        round_fill(ui->ren, bg, 12, (SDL_Color){255,230,120,banner_alpha}, (SDL_Color){210,180,90,banner_alpha});
        draw_text(ui->ren, ui->font_big, banner, bg.x+(bg.w-bw)/2, bg.y+(bg.h-bh)/2, FG_DARK);
    }
}

/* --------- GAME OVER --------- */
void ui_t_draw_gameover(UiT *ui, const Tetris *t, const char *player_name){
    SDL_SetRenderDrawBlendMode(ui->ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ui->ren, 0,0,0,140);
    SDL_Rect full={0,0,T_WIN_W,T_WIN_H}; SDL_RenderFillRect(ui->ren,&full);

    SDL_Rect boxRc = { GO_BOX_X, GO_BOX_Y, GO_BOX_W, GO_BOX_H };
    round_fill(ui->ren, boxRc, 16, (SDL_Color){255,255,255,245}, (SDL_Color){210,215,220,255});

    int tw, th; const char* ttl="GAME OVER";
    text_size(ui->font_title, ttl, &tw, &th);
    draw_text(ui->ren, ui->font_title, ttl, boxRc.x+(boxRc.w-tw)/2, boxRc.y+24, (SDL_Color){180,40,40,255});

    char line[128];
    snprintf(line,sizeof(line),"Jogador: %s", player_name?player_name:"Player");
    text_size(ui->font_big,line,&tw,&th);
    draw_text(ui->ren, ui->font_big, line, boxRc.x+(boxRc.w-tw)/2, boxRc.y+110, FG_DARK);
    snprintf(line,sizeof(line),"Score: %d", t->score);
    text_size(ui->font_big,line,&tw,&th);
    draw_text(ui->ren, ui->font_big, line, boxRc.x+(boxRc.w-tw)/2, boxRc.y+150, FG_DARK);

    SDL_Rect tryb  = { GO_TRY_X,  GO_TRY_Y,  GO_TRY_W,  GO_TRY_H };
    SDL_Rect menub = { GO_MENU_X, GO_MENU_Y, GO_MENU_W, GO_MENU_H };
    button_fit(ui->ren, ui->font_big, ui->font_med, ui->font_small,
               tryb, (SDL_Color){34,172,98,230}, (SDL_Color){18,110,58,255},
               "RECOMEÇAR", (SDL_Color){255,255,255,255});
    button_fit(ui->ren, ui->font_big, ui->font_med, ui->font_small,
               menub, (SDL_Color){80,148,210,255}, (SDL_Color){40,90,150,255},
               "MENU", (SDL_Color){255,255,255,255});
}
