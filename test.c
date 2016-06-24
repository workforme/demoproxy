/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  test.c:  Chen Huaying <yilong@sina.cn>
 *
 *  test for ldapproxy.
 */

#include <stdlib.h>
#include <pthread.h>

#include "stk_module.h"

#include "lp_comm.h"
#include "lp_log.h"
#include "lp_task.h"

extern void test1();
extern void test2();
int main(int argc, char *argv[])
{
#if 0
    if (stk_init_modules() == STK_MODULE_ERROR) {
        stk_log_error("stk_init_modules() failed!");
        stk_exit_modules();
        exit(1);
    }
    lp_log_info("test connections................");
    test1();
    test2();
    lp_log_info("test connections end............");
#endif

    lp_log_info("test task.......................");
    lp_task_t *task1 = lp_task_new();
    stk_log_debug("task1 tick: %ld", task1->tick);
    lp_task_destroy(task1);

    lp_log_info("test task end...................");



    //stk_exit_modules();
    return 0;
}
