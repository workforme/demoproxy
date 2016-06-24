/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_modules.c:  Chen Huaying <chenyilong@sina.cn>
 *
 *  ldapproxy modules.
 */

#include <stdlib.h>

#include "stk_module.h"

extern stk_module_t lp_log_module;
extern stk_module_t lp_connection_module;
extern stk_module_t lp_task_manager;
extern stk_module_t lp_config_module;
extern stk_module_t lp_threads_module;
extern stk_module_t lp_ldap_module;
extern stk_module_t lp_server_module;
extern stk_module_t lp_mysql_module;

stk_module_t *stk_modules[] = {
    &lp_config_module,
    &lp_log_module,
    &lp_connection_module,
    &lp_threads_module,
    &lp_ldap_module,
    &lp_mysql_module,
    &lp_server_module,
    NULL
};

