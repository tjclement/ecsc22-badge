#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "menu.h"
#include "console.h"

static menu_t *cur_menu;

void menu_render() {
    if (cur_menu == NULL) { return; }
    
    console_clear();
    console_printf("\033[H\033[2J%s\r\n\r\n", cur_menu->instructions);

    for (int i = 0; i < cur_menu->num_names; i++) {
        if (i == cur_menu->cur_index) {
            console_printf(" > %s\r\n", cur_menu->names[i]);
        } else {
            console_printf("   %s\r\n", cur_menu->names[i]);
        }
    }
}

void menu_input_handler(char *input, int len) {
    if (cur_menu == NULL) { return; }

    if (strstr(input, "\r") != NULL || strstr(input, "\n") != NULL) {
        // Enter key pressed
        console_pop_handler();
        console_clear();
        cur_menu->callback(cur_menu->cur_index);
        cur_menu = NULL;
        return;
    } else if (strnstr(input, "\033[B", 3) != NULL) {
        // Down key pressed
        cur_menu->cur_index = fmin(cur_menu->cur_index+1, cur_menu->num_names-1);
    } else if (strnstr(input, "\033[A", 3) != NULL) {
        // Up key pressed
        cur_menu->cur_index = fmax(cur_menu->cur_index-1, 0);
    }

    menu_render();
}


static console_handler_t console_handler = {
    menu_input_handler,
    .echo_input = false,
    .wait_for_newline = false
};

void menu(menu_t *menu_info) {
    cur_menu = menu_info;
    cur_menu->cur_index = 0;
    console_push_handler(console_handler);
}