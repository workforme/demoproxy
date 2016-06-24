/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_config.c:  Chen Huaying <chenyilong@sina.cn>
 *
 *  ldapproxy configure module.
 *
 *  read configure file
 */

#include <sys/types.h>

#include "stk_module.h"
#include "stk_log.h"

#include "lp_comm.h"
#include "lp_config.h"
#include "confparser.h"

#define LP_CONF_FILE "/usr/local/etc/sinamail/ldapproxy.ini"

static void __show_all_config()
{
    stk_log_info("log_level         = %d\n"
                 "max_threads       = %d\n"
                 "task_manager_num  = %d\n"
                 "ldap_num          = %d\n"
                 "mysql_num         = %d\n"
                 "thread_stack_size = %d\n"
                 "max_connections   = %d\n"
                 "listening_ip      = %s\n"
                 "port              = %d\n",
                 lp_configs.log_level, lp_configs.max_threads,
                 lp_configs.task_manager_num, lp_configs.ldap_num,
                 lp_configs.mysql_num, lp_configs.thread_stack_size,
                 lp_configs.max_connections,
                 lp_configs.listening_ip, lp_configs.listening_port);
}

static int lp_config_init()
{
    /* default */
    lp_configs.log_level          = 3;
    lp_configs.max_threads        = 128;
    lp_configs.task_manager_num   = 1;
    lp_configs.ldap_num           = 4;
    lp_configs.mysql_num          = 8;
    lp_configs.thread_stack_size  = 2 * 1024;
    lp_configs.max_connections    = 2 * 1024;
    lp_configs.listening_ip       = NULL;
    lp_configs.listening_port     = 389;
    lp_configs.timeout            = 30;

    /* read configure file */
    struct conf_int_config log_conf[] = {
        {"log_level", &lp_configs.log_level},
        {0, 0}
    };

    struct conf_str_config server_str_conf[] = {
        {"listening", lp_configs.listening_ip},
        {0, 0}
    };
    
    struct conf_int_config  server_int_conf[] = {
        {"port", &lp_configs.listening_port},
        {"max_connections", &lp_configs.max_connections},
        {"timeout", &lp_configs.timeout},
        {0, 0}
    };

    int thread_stack_size = 0;
    struct conf_int_config thread_conf[] = {
        {"max_threads", &lp_configs.max_threads},
        {"thread_stack_size", &thread_stack_size},
        {"task_manager_num", &lp_configs.task_manager_num},
        {"ldap_num", &lp_configs.ldap_num},
        {"mysql_num", &lp_configs.mysql_num},
        {0, 0}
    };

    dictionary *conf = NULL;
    char *conf_file = (lp_config_file) ? lp_config_file : LP_CONF_FILE;
    conf = open_conf_file(conf_file);
    if (conf == NULL) {
        stk_log_error("read configure file failed! Use Default config!!");
        /* stack size in KBytes */
        lp_configs.thread_stack_size *= 1024;
        __show_all_config();
        /* DEBUG use LP_ERR, release use LP_OK */
        return LP_OK;
    }

    parse_conf_file(conf, "thread", thread_conf, NULL);
    /* stack size in KBytes */
    lp_configs.thread_stack_size = (thread_stack_size == 0 ?
            lp_configs.thread_stack_size : thread_stack_size ) * 1024;

    parse_conf_file(conf, "server", server_int_conf, server_str_conf);
    parse_conf_file(conf, "log", log_conf, NULL);

    __show_all_config();

    return LP_OK;
}

static void lp_config_exit()
{
}

stk_module_t lp_config_module = {
    STK_MODULE_V1,
    "LP_CONFIG_MODULE",
    lp_config_init,
    lp_config_exit
};
