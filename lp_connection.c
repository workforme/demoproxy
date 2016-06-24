/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_connection.c:  Chen Huaying <chenyilong@sina.cn>
 *
 *  ldapproxy connection manager.
 */

#include <pthread.h>
#include <event.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "stk_list.h"
#include "stk_socket.h"
#include "stk_module.h"
#include "stk_pool.h"

#include "lber.h"
#include "lber-int.h"
#include "ldap.h"

#include "lp_log.h"
#include "lp_comm.h"
#include "lp_config.h"
#include "lp_task.h"
#include "lp_ldap.h"
#include "lp_connection.h"

static int __max_connections;
static lp_connection_t *__lp_connections;
static event_t *__read_events;
static event_t *__write_events;
static pthread_mutex_t __conn_mutex;
static lp_connection_t *__free_connections;
static int __free_connections_n;
static int __connections_n;

static void __discard_connection_data(lp_connection_t *c);
static void __discard_list_data(lp_connection_t *c, stk_list_t *list);
static void __discard_task_ticks(lp_connection_t *c);

static int lp_connections_init();
static void lp_connections_exit();

lp_connection_t *lp_get_connection(stk_socket_t sockfd)
{
    lp_connection_t *c = NULL;

    pthread_mutex_lock(&__conn_mutex);
    c = __free_connections;
    if (c == NULL) {
        lp_log_error("__free_connections is NULL, error!");
        pthread_mutex_unlock(&__conn_mutex);
        return NULL;
    }
    __free_connections = c->data;
    --__free_connections_n;
    ++__connections_n;
    pthread_mutex_unlock(&__conn_mutex);
    c->fd = sockfd;


    return c;
}

void lp_free_connection(lp_connection_t *c)
{
    __discard_connection_data(c);
    __discard_task_ticks(c);
    lp_memzero(c->rev, sizeof(event_t));
    lp_memzero(c->wev, sizeof(event_t));
    if (c->sb) {
        ber_sockbuf_free(c->sb);
    }

    event_t *rev = c->rev;
    event_t *wev = c->wev;
    int      idx = c->idx;
    /* connection data to zeror */
    lp_memzero(c, sizeof(lp_connection_t));

    c->rev = rev;
    c->wev = wev;
    c->idx = idx;
    stk_list_init(&c->req_list);
    stk_list_init(&c->rpl_list);
    stk_list_init(&c->task_ticks);

    pthread_mutex_lock(&__conn_mutex);
    c->data = __free_connections;
    __free_connections = c;
    ++__free_connections_n;
    --__connections_n;
    pthread_mutex_unlock(&__conn_mutex);
}

void lp_close_connection(lp_connection_t *c)
{
    lp_assert(c != NULL);
    //int fd = c->fd;
    lp_free_connection(c);

#if 0
    struct stat stat_buf = {0};
    if (fstat(fd, &stat_buf) < 0) {
        lp_log_error("client fd (%d) | connection(%d) conn fd (%d) error(%d) %m", fd, c->idx, c->fd, errno);
        exit(2);
    }
    lp_log_info("close fd(%d) start , inode(%d)", fd, stat_buf.st_ino);
    close(fd);
    lp_log_info("close %d end, (c->fd = %d)", fd, c->fd);
#endif
}

void lp_connection_set_discard_cb(lp_connection_t *c, lp_discard_cb cb)
{
    c->discard = cb;
}

static void __discard_connection_data(lp_connection_t *c)
{
    __discard_list_data(c, &c->req_list);
    __discard_list_data(c, &c->rpl_list);
}

static void __discard_task_ticks(lp_connection_t *c)
{
    lp_clean_task_ticks(&c->task_ticks);
}

static void __discard_list_data(lp_connection_t *c, stk_list_t *list)
{
    lp_assert(c->discard != NULL);
    (c->discard)(list);
}

static int lp_connections_init()
{
    int i;
    lp_connection_t *next;

    __max_connections = lp_configs.max_connections;
    __lp_connections = (lp_connection_t *)
                        stk_calloc(__max_connections * sizeof(lp_connection_t));
    if (__lp_connections == NULL) {
        lp_log_error("create connections failed!");
        return LP_ERR;
    }

    __read_events = (event_t *)stk_calloc(__max_connections * sizeof(event_t));
    if (__read_events == NULL) {
        lp_log_error("create read events failed!");
        return LP_ERR;
    }

    __write_events = (event_t *)stk_calloc(__max_connections * sizeof(event_t));
    if (__write_events == NULL) {
        lp_log_error("create write events failed!");
        return LP_ERR;
    }

    pthread_mutex_init(&__conn_mutex, NULL);

    i = __max_connections;
    next = NULL;
    do {
        --i;

        __lp_connections[i].data = next;
        __lp_connections[i].rev = &__read_events[i];
        __lp_connections[i].wev = &__write_events[i];
        __lp_connections[i].idx = i;
        __lp_connections[i].fd = -1;
        stk_list_init(&__lp_connections[i].req_list);
        stk_list_init(&__lp_connections[i].rpl_list);
        stk_list_init(&__lp_connections[i].task_ticks);

        next = &__lp_connections[i];
    } while (i);

    __free_connections = next;
    __free_connections_n = __max_connections;

    return LP_OK;
}

static void lp_connections_exit()
{
    /* delete all the data in request and reply list */
    LP_MODULE_TRACE();
    int i;
    for (i = 0; i < __connections_n; ++i) {
        lp_close_connection(&__lp_connections[i]);
    }
    stk_free(__lp_connections);
    stk_free(__read_events);
    stk_free(__write_events);
}

stk_module_t lp_connection_module = {
    STK_MODULE_V1,
    "LP connection manager",
    lp_connections_init,
    lp_connections_exit
};
