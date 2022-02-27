#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

bool create_WP(char *args);

bool update_WP_state();

void print_WP_info();

void free_wp(int no);

void new_up(char *arg, bool *success);

#endif
