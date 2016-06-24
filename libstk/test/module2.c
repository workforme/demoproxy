/*
 *  Copyright (c) 2015 SINA Corporation, All Rights Reserved.
 *
 *  module2.c:  Yilong Zhao <chenhuaying@sina.cn>
 *
 *  simple tool kit: test module for stk_module.
 */

#include "stk_module.h"
#include "stk_log.h"


static int stk_test_module_init();
static void stk_test_module_exit();

stk_module_t stk_test_module2 = {
    STK_MODULE_V1,
    "TEST MODULE 2",
    stk_test_module_init,
    stk_test_module_exit
};

static int stk_test_module_init()
{
    stk_log_info("init test module2....................");

    return STK_MODULE_OK;
}

static void stk_test_module_exit()
{
    stk_log_info("exit test module2....................");
}
