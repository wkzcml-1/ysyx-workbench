#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

bool update_watch_state() {
    return false;
}

#endif
