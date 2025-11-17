#include "highscore.h"
#include <stdio.h>
#include <string.h>

void hs_init(HighScores *hs){ hs->count=0; }

bool hs_load(HighScores *hs, const char *path){
    hs_init(hs);
    FILE *f=fopen(path,"r");
    if(!f) return false;
    char name[HS_NAME_MAX]; int score;
    while(hs->count<HS_MAX && fscanf(f,"%23[^;];%d\n",name,&score)==2){
        strncpy(hs->items[hs->count].name, name, HS_NAME_MAX-1);
        hs->items[hs->count].name[HS_NAME_MAX-1]=0;
        hs->items[hs->count].score=score;
        hs->count++;
    }
    fclose(f);
    return true;
}

bool hs_save(const HighScores *hs, const char *path){
    FILE *f=fopen(path,"w");
    if(!f) return false;
    for(int i=0;i<hs->count;i++){
        fprintf(f,"%s;%d\n", hs->items[i].name, hs->items[i].score);
    }
    fclose(f);
    return true;
}

void hs_add(HighScores *hs, const char *name, int score){
    char nm[HS_NAME_MAX]; strncpy(nm, name?name:"Player", HS_NAME_MAX-1); nm[HS_NAME_MAX-1]=0;

    if(hs->count < HS_MAX){
        hs->items[hs->count].score=score;
        strncpy(hs->items[hs->count].name, nm, HS_NAME_MAX);
        hs->count++;
    }else{
        if(score <= hs->items[hs->count-1].score) return;
        hs->items[hs->count-1].score=score;
        strncpy(hs->items[hs->count-1].name, nm, HS_NAME_MAX);
    }
    for(int i=hs->count-1;i>0;i--){
        if(hs->items[i].score > hs->items[i-1].score){
            HSItem t=hs->items[i]; hs->items[i]=hs->items[i-1]; hs->items[i-1]=t;
        }else break;
    }
}

int hs_find_names(const HighScores *hs, const char *unique_names[], int max_names){
    int out=0;
    for(int i=0;i<hs->count && out<max_names;i++){
        const char *nm=hs->items[i].name;
        int seen=0;
        for(int j=0;j<out;j++) if(strcmp(unique_names[j],nm)==0){ seen=1; break; }
        if(!seen) unique_names[out++]=nm;
    }
    return out;
}
