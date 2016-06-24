/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_comm.h:  Chen Huaying <chenhuaying@sina.cn>
 *
 *  ldapproxy common use.
 */

#ifndef __LP_COMM_H
#define __LP_COMM_H

#include <stdlib.h>
#include <string.h>

#include "stk_list.h"
#include "stk_pool.h"

typedef struct lp_ldap_s lp_ldap_t;
typedef struct lp_mysql_s lp_mysql_t;
typedef struct lp_connection_s lp_connection_t;
typedef struct lp_task_manager_s lp_task_manager_t;

typedef struct event event_t;
typedef struct event_base event_base_t;

#define LP_AGAIN 1
#define LP_OK    0
#define LP_ERR  -1

#define true     1
#define false    0

#define LP_IP_LEN       32
#define LP_ERR_STR_LEN  128

#define lp_assert(p)    \
    if (!(p)) {lp_log_error("%s[%d]: %s Warning: "#p" failed!", \
                            __FILE__, __LINE__, __func__); abort();}
#define return_if_failed(p) \
    if (!(p)) {lp_log_error("%s[%d]: %s Warning: "#p" failed!", \
                            __FILE__, __LINE__, __func__); return;}
#define return_val_if_failed(p, val)    \
    if (!(p)) {lp_log_error("%s[%d]: %s Warning: "#p" failed!", \
                            __FILE__, __LINE__, __func__); return val;}

#define LP_UNUSED_ARG(arg)  (void)(arg);
#define lp_memzero(buf, n)  (void)memset(buf, 0, n)


#endif /*__LP_COMM_H*/
