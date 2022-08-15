#ifndef MENU_H
#define MENU_H

typedef struct {
    char *instructions;
    char **names;
    int num_names;
    void (*callback)(int);
    int cur_index;
} menu_t;

void menu(menu_t *items);
void menu_render();

#endif
