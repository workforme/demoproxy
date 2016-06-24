/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_threads.c:  Chen Huaying <chenhuaying@sina.cn>
 *
 *  ldapproxy threads modules.
 */

#include <sys/types.h>

#include "stk_module.h"
#include "stk_threads.h"

#include "lp_config.h"

static int lp_init_threads()
{
    int threads_num = lp_configs.max_threads;
    size_t stack_size = lp_configs.thread_stack_size;

    return stk_init_threads(threads_num, stack_size);
}

static void lp_exit_threads()
{
    stk_exit_threads();
}
stk_module_t lp_threads_module = {
    STK_MODULE_V1,
    "LP_THREADS_MODULE",
    lp_init_threads,
    lp_exit_threads
};
