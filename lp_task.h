/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_task.h:  Chen Huaying chenyilong@sina.cn
 *
 *  ldapproxy task.
 */

#ifndef __LP_TASK_H
#define __LP_TASK_H

typedef struct lp_task_s lp_task_t;

typedef enum {
    LP_TASK_SOURCE_SERVER,
    LP_TASK_SOURCE_LDAP,
    LP_TASK_SOURCE_MYSQL,
    LP_TASK_SOURCE_TASK_MANAGER
} lp_task_source;

typedef enum {
    LP_TASK_IO_READ             = (1 << 0),
    LP_TASK_IO_WRITE            = (1 << 1),
    LP_TASK_IO_READ_REG         = (1 << 2),
    LP_TASK_IO_WRITE_REG        = (1 << 3),
    LP_TASK_IO_UNREG            = (1 << 4),
    LP_TASK_IO_CLOSE            = (1 << 5),
    LP_TASK_LDAP_MSG            = (1 << 6),
    LP_TASK_MYSQL_MSG           = (1 << 7),
    LP_TASK_LISTENING           = (1 << 8),
    LP_TASK_IO_ALL_REG          = (1 << 9),
} lp_task_type;

struct lp_task_s {
    void               *sender;
    void               *receiver;
    lp_task_source      sender_source;
    lp_task_source      receiver_source;
    lp_task_type        task_type;

    lp_connection_t    *c;  /* message exchange between modules */
    int                 c_uuid; /* connection uuid */

    /* for update libevents' event add/set/del */
    event_t            *ev;

    void               *data;
    stk_list_t          list;
    time_t              tick;   /* a time tick, is this task valid or not */

    stk_pool_t         *pool;
};

lp_task_t *lp_task_new();
void lp_task_destroy(lp_task_t *task);
void lp_task_destroy_cb(void *task);
void lp_clean_task_ticks(stk_list_t *list);
int lp_conn_register_task_tick(lp_connection_t *c, lp_task_t *task);
int lp_conn_check_task_tick(lp_connection_t *c, lp_task_t *task);


#endif /*__LP_TASK_H*/
