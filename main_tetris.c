#include <SDL2/SDL.h>
#include <stdbool.h>
#include <string.h>
#include "core/tetris.h"
#include "core/highscore.h"
#include "ui/ui_tetris.h"

typedef enum { ST_HOME=0, ST_NAME, ST_PLAY, ST_GAMEOVER } State;
enum { DAS_FRAMES=10, ARR_FRAMES=2 };

static const char* banner_for(int lines){
    switch(lines){
        case 1: return "SINGLE";
        case 2: return "DOUBLE";
        case 3: return "TRIPLE";
        case 4: return "TETRIS";
        default: return NULL;
    }
}

int main(int argc, char **argv){
    (void)argc; (void)argv;

    HighScores hs; hs_init(&hs); hs_load(&hs, "scores.txt");
    UiT *ui = ui_t_init("Empilha Blocos"); if(!ui) return 1;
    Tetris t; tetris_init(&t);

    State st=ST_HOME;
    char player[HS_NAME_MAX]="Player";
    char typed[HS_NAME_MAX]="";

    const char *names[10]; int scores[10];
    for(int i=0;i<10;i++){ if(i<hs.count){ names[i]=hs.items[i].name; scores[i]=hs.items[i].score; } else { names[i]="-"; scores[i]=0; } }

    int das_l=0,das_r=0; bool hold_l=false,hold_r=false;
    const char *banner=NULL; int banner_timer=0;
    bool prev_tetris=false;  /* para BACK-TO-BACK */

    Uint32 last = SDL_GetTicks();
    bool running=true;

    while(running){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) running=false;

            if(st==ST_HOME){
                if(e.type==SDL_KEYDOWN){ if(e.key.keysym.sym==SDLK_ESCAPE) running=false; if(e.key.keysym.sym==SDLK_RETURN) st=ST_NAME; }
                if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT){
                    int mx=e.button.x,my=e.button.y;
                    if(mx>=HOME_PLAY_X && mx<=HOME_PLAY_X+HOME_PLAY_W && my>=HOME_PLAY_Y && my<=HOME_PLAY_Y+HOME_PLAY_H) st=ST_NAME;
                }
            } else if(st==ST_NAME){
                if(e.type==SDL_KEYDOWN){
                    SDL_Keycode k=e.key.keysym.sym;
                    if(k==SDLK_ESCAPE){ st=ST_HOME; typed[0]=0; }
                    else if(k==SDLK_RETURN){
                        strncpy(player, typed[0]?typed:"Player", sizeof(player));
                        tetris_reset(&t); st=ST_PLAY; banner=NULL; banner_timer=0; prev_tetris=false;
                    } else if(k==SDLK_BACKSPACE){ size_t n=strlen(typed); if(n>0) typed[n-1]=0; }
                    else if(k>=32 && k<127){ size_t n=strlen(typed); if(n<HS_NAME_MAX-1){ typed[n]=(char)k; typed[n+1]=0; } }
                }
                if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT){
                    int mx=e.button.x,my=e.button.y;
                    if(mx>=NAME_START_X && mx<=NAME_START_X+NAME_START_W && my>=NAME_START_Y && my<=NAME_START_Y+NAME_START_H){
                        strncpy(player, typed[0]?typed:"Player", sizeof(player));
                        tetris_reset(&t); st=ST_PLAY; banner=NULL; banner_timer=0; prev_tetris=false;
                    }
                    if(mx>=NAME_BACK_X && mx<=NAME_BACK_X+NAME_BACK_W && my>=NAME_BACK_Y && my<=NAME_BACK_Y+NAME_BACK_H) st=ST_HOME;
                }
            } else if(st==ST_PLAY){
                if(e.type==SDL_KEYDOWN){
                    SDL_Keycode k=e.key.keysym.sym;
                    if(k==SDLK_ESCAPE){ hs_add(&hs, player, t.score); hs_save(&hs,"scores.txt"); running=false; }
                    else if(k==SDLK_a){ tetris_move(&t,-1); hold_l=true; das_l=0; }
                    else if(k==SDLK_d){ tetris_move(&t,+1); hold_r=true; das_r=0; }
                    else if(k==SDLK_s)     tetris_soft_drop(&t);
                    else if(k==SDLK_SPACE) tetris_hard_drop(&t);
                    else if(k==SDLK_e) tetris_rotate(&t,+1);
                    else if(k==SDLK_q) tetris_rotate(&t,-1);
                    else if(k==SDLK_c) tetris_hold(&t);
                }
                if(e.type==SDL_KEYUP){ if(e.key.keysym.sym==SDLK_a){hold_l=false;das_l=0;} if(e.key.keysym.sym==SDLK_d){hold_r=false;das_r=0;} }
                if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT){
                    int mx=e.button.x,my=e.button.y;
                    if(mx>=GAME_BTN_X && mx<=GAME_BTN_X+GAME_BTN_W && my>=GAME_BTN_Y && my<=GAME_BTN_Y+GAME_BTN_H){ tetris_reset(&t); banner=NULL; banner_timer=0; prev_tetris=false; }
                }
            } else if(st==ST_GAMEOVER){
                if(e.type==SDL_KEYDOWN){
                    if(e.key.keysym.sym==SDLK_RETURN){ tetris_reset(&t); banner=NULL; banner_timer=0; prev_tetris=false; st=ST_PLAY; }
                    else if(e.key.keysym.sym==SDLK_ESCAPE){ st=ST_HOME; }
                }
                if(e.type==SDL_MOUSEBUTTONDOWN && e.button.button==SDL_BUTTON_LEFT){
                    int mx=e.button.x,my=e.button.y;
                    if(mx>=GO_TRY_X && mx<=GO_TRY_X+GO_TRY_W && my>=GO_TRY_Y && my<=GO_TRY_Y+GO_TRY_H){ tetris_reset(&t); banner=NULL; banner_timer=0; prev_tetris=false; st=ST_PLAY; }
                    else if(mx>=GO_MENU_X && mx<=GO_MENU_X+GO_MENU_W && my>=GO_MENU_Y && my<=GO_MENU_Y+GO_MENU_H){ st=ST_HOME; }
                }
            }
        }

        Uint32 now=SDL_GetTicks();
        while(now - last >= 16){
            if(st==ST_PLAY){
                if(hold_l){ das_l++; if(das_l>=DAS_FRAMES && ((das_l-DAS_FRAMES)%ARR_FRAMES)==0) tetris_move(&t,-1); }
                if(hold_r){ das_r++; if(das_r>=DAS_FRAMES && ((das_r-DAS_FRAMES)%ARR_FRAMES)==0) tetris_move(&t,+1); }

                tetris_step(&t);

                if(tetris_consume_clear_event(&t)){
                    int c = tetris_last_clear(&t);
                    const char *base = banner_for(c);
                    static char b2b[32];
                    if(c==4){
                        if(prev_tetris){ snprintf(b2b,sizeof(b2b),"TETRIS B2B"); base=b2b; }
                        prev_tetris=true;
                    } else prev_tetris=false;

                    banner = base;
                    banner_timer = (banner && c>0) ? 60 : 0;
                }
                if(banner_timer>0) banner_timer--;

                if(t.game_over){
                    hs_add(&hs, player, t.score); hs_save(&hs,"scores.txt");
                    st=ST_GAMEOVER;
                    for(int i=0;i<10;i++){ if(i<hs.count){ names[i]=hs.items[i].name; scores[i]=hs.items[i].score; } else { names[i]="-"; scores[i]=0; } }
                }
            }
            last += 16;
        }

        ui_t_begin(ui);
        if(st==ST_HOME){
            ui_t_draw_home(ui,"Empilha Blocos"); ui_t_draw_home_play_button(ui,false);
            ui_t_draw_home_ranking(ui,names,scores,hs.count);
        } else if(st==ST_NAME){
            ui_t_draw_name_input(ui,typed);
        } else if(st==ST_PLAY){
            int next3[3]={0,0,0}; for(int i=0;i<3;i++){ int id=tetris_peek_next(&t,i); next3[i]=(id>=0)?(id+1):0; }
            int alpha = (banner_timer>0)? (banner_timer>10?230:banner_timer*23) : 0;
            ui_t_draw_game(ui,&t,player,t.score,next3,banner,alpha);
        } else {
            int dummy[3]={0,0,0};
            ui_t_draw_game(ui,&t,player,t.score,dummy,NULL,0);
            ui_t_draw_gameover(ui,&t,player);
        }
        ui_t_end(ui);
    }
    ui_t_shutdown(ui);
    return 0;
}
