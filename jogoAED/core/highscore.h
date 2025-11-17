#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <stdbool.h>

#define HS_MAX 100
#define HS_NAME_MAX 24

typedef struct {
    char name[HS_NAME_MAX];
    int  score;
} HSItem;

typedef struct {
    HSItem items[HS_MAX];
    int count;
} HighScores;

void hs_init(HighScores *hs);
bool hs_load(HighScores *hs, const char *path);
bool hs_save(const HighScores *hs, const char *path);
void hs_add(HighScores *hs, const char *name, int score); // insertion sort (desc)
int  hs_find_names(const HighScores *hs, const char *unique_names[], int max_names);

#endif
