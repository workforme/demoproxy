/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_config.h:  Chen Huaying yilong@sina.cn
 *
 *  ldapproxy configure module.
 */

#ifndef __LP_CONFIG_H
#define __LP_CONFIG_H

typedef struct lp_config_s lp_config_t;

struct lp_config_s {
    int             log_level;
    int             max_threads;
    int             task_manager_num;
    int             ldap_num;
    int             mysql_num;
    size_t          thread_stack_size;
    int             max_connections;
    char           *listening_ip;
    int             listening_port;
    int             timeout;
};

lp_config_t lp_configs;
char *lp_config_file;


#endif /*__LP_CONFIG_H*/
