/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_task.c:  Chen Huaying <chenhuaying@sina.cn>
 *
 *  ldapproxy task.
 */

#include <sys/time.h>
#include <pthread.h>

#include "stk_list.h"
#include "stk_socket.h"

#include "portable.h"
#include "lber.h"
#include "lber-int.h"
#include "ldap.h"

#include "lp_comm.h"
#include "lp_task.h"
#include "lp_ldap.h"
#include "lp_log.h"
#include "lp_connection.h"

time_t __get_task_tick();

typedef struct task_tick_s {
    stk_list_t      list;
    time_t          tick;
} task_tick_t;

lp_task_t *lp_task_new()
{
    lp_task_t *task;

    stk_pool_t *pool = stk_create_pool();
    if (pool == NULL) {
        lp_log_error("stk_create_pool failed!");
        return NULL;
    }

    task = (lp_task_t *)stk_pcalloc(pool, sizeof(lp_task_t));
    if (task) {
        task->pool = pool;
        task->tick = __get_task_tick();
        lp_log_debug("new task(%p)", task);
    } else {
        stk_destroy_pool(pool);
    }

    return task;
}

void lp_task_destroy(lp_task_t *task)
{
    stk_destroy_pool(task->pool);
}

int lp_conn_register_task_tick(lp_connection_t *c, lp_task_t *task)
{
    lp_assert(c != NULL);
    lp_assert(task != NULL);

    task_tick_t *tick = NULL;

    if ((tick = stk_alloc(sizeof(task_tick_t))) ==  NULL) {
        return LP_ERR;
    }
    tick->tick = task->tick;

    stk_list_add_tail(&tick->list, &c->task_ticks);

    return LP_OK;
}

int lp_conn_check_task_tick(lp_connection_t *c, lp_task_t *task)
{
    TRACE();

    task_tick_t *itr = NULL;
    stk_list_for_each_entry(itr, &c->task_ticks, list) {
        lp_log_debug("task->tick: %ld, itr->tick: %ld", task->tick, itr->tick);
        if (itr->tick == task->tick) {
            stk_list_del(&itr->list);
            stk_free(itr);
            return LP_OK;
        }
    }
    TRACE();
    return LP_ERR;
}

void lp_clean_task_ticks(stk_list_t *list)
{
    TRACE();
    task_tick_t *itr;
    
    while (!stk_list_empty(list)) {
        itr = stk_list_pop_entry(list, task_tick_t, list);
        stk_free(itr);
    }
}

time_t __get_task_tick()
{
    struct timeval timestamp;

    gettimeofday(&timestamp, NULL);
    lp_log_debug("time tick: %ld, %ld", timestamp.tv_sec, timestamp.tv_usec);

    return  (timestamp.tv_sec + timestamp.tv_usec);
}
