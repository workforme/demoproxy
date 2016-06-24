/*
 *  Copyright (c) 2015 SINA Corporation, All Rights Reserved.
 *
 *  stk_signal.h:  Yilong Zhao <chenhuaying@sina.cn>
 *
 *  simple tool kit: signal API.
 */

#include <signal.h>


typedef struct {
    int     signo;                  /* signal number */
    char   *signame;                /* signal name */
    char   *name;                   /* process name */
    void  (*handler)(int signo);    /* process handler */
} stk_signal_t;

int stk_init_signals();
