/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_ldap.c:  Chen Huaying <yilong@sina.cn>
 *
 *  ldapproxy ldap module.
 */

#include <event.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "stk_list.h"
#include "stk_module.h"
#include "stk_socket.h"
#include "stk_threads.h"

#include "portable.h"
#include "lber.h"
#include "lber-int.h"
#include "ldap.h"

#include "lp_comm.h"
#include "lp_log.h"
#include "lp_task.h"
#include "lp_ldap_logic.h"
#include "lp_ldap.h"
#include "lp_mysql.h"
#include "lp_config.h"
#include "lp_connection.h"
#include "lp_ldap_logic.h"
#include "utils.h"

#ifdef  LDAP_TEST
 #include <string.h>
#endif

typedef enum {
    LP_LDAP_UNREADY,
    LP_LDAP_CONNECTED,
    LP_LDAP_CLOSE
} lp_ldap_state;

struct lp_ldap_s {
    int                     idx;
    union {
        int                 pipes[2];
        struct {
            int             pipe_read;
            int             pipe_write;
        };
    };

    event_base_t           *ev_base;

    int                     urandom;
    lp_ldap_state           state;
    stk_thread_t           *thread;

    pthread_mutex_t         mutex;
    stk_list_t              queue;
};

#ifndef LDAP_TEST
ldap_data_t *ldap_data_new()
{
    ldap_data_t *data = (ldap_data_t *)stk_alloc(sizeof(ldap_data_t));
    if (data == NULL) {
        lp_log_error("alloc ldap data failed!");
        return NULL;
    }

    BerElement *ber = ber_alloc_t(LBER_USE_DER);
    if (ber == NULL) {
        lp_log_error("alloc BerElement failed!");
        stk_free(data);
        return NULL;
    }

    data->ber = ber;

    return data;
}

void ldap_data_destroy(ldap_data_t *data)
{
    ber_free(data->ber, 1);
    stk_free(data);
}

void lp_connection_discard_cb(stk_list_t *list)
{
    ldap_data_t *itr;

    while (!stk_list_empty(list)) {
        itr = stk_list_pop_entry(list, ldap_data_t, list);
        ldap_data_destroy(itr);
    }
}
#endif

static lp_ldap_t *lp_ldaps;
static int __index;

static int __ldap_init(lp_ldap_t *ldap);
//static int __process_task_manager_task(lp_task_t *task);
static int __process_mysql_task(lp_ldap_t *ldap, lp_task_t *task);
static int __process_server_task(lp_ldap_t *ldap, lp_task_t *task);
//static int __ldap_io_unregister(lp_task_manager_t *task_manager,
//                                lp_connection_t *c);
static int __ldap_write(lp_connection_t *c);
static int __ldap_read(lp_connection_t *c);
static int __ldap_get_ber(lp_connection_t *c);
static int __ldap_process_request(lp_connection_t *c);
static lp_task_t *__pop_task(lp_ldap_t *ldap);
static void lp_ldap_io_event_handler(int fd, short event_type, void *arg);
static int __task_process(lp_ldap_t *ldap, lp_task_t *task);
static int __io_unregister(lp_connection_t *c);

static void __ldap_notify_cb(int fd, short event_type, void *arg)
{
    lp_assert(arg != NULL);
    lp_task_t *task;
    int n;
    char buf[128];

    lp_ldap_t *ldap = (lp_ldap_t *)arg;

    while (true) {
        n = read(fd, buf, sizeof(buf));
        if (n <= 0) {
            if (n == 0 || errno != EAGAIN || errno != EWOULDBLOCK) {
                lp_log_error("read notify pipe failed!");
                break;
            }
            break;
        }
    }

    while (true) {
        task = __pop_task(ldap);
        if (task == NULL) {
            break;
        }

        lp_assert(task->receiver == ldap);

        if (__task_process(ldap, task) == LP_ERR) {
            /* just write a log, continue to process the next task */
            lp_log_error("process register IO task failed!");
        }
    }

}

/* Don't need accuracy */
lp_ldap_t *lp_get_ldap()
{
    lp_ldap_t *ldap;
    if (__index >= lp_configs.ldap_num) {
        __index = 0;
    }
    
    ldap = &lp_ldaps[__index];
    ++__index;

    return ldap;
}

int lp_ldap_io_register(stk_socket_t sockfd)
{
    TRACE();
    lp_connection_t *c;
    event_t *rev, *wev;
    lp_ldap_t *ldap;
    int cnt=0; 
    for(cnt=0;cnt<3;cnt++) 
    {
      if ((c = lp_get_connection(sockfd)) == NULL) {
        lp_log_info("GOT_CONN_NULL,retry %d'st time",cnt);
        //return LP_ERR;
        if(cnt==2)
           return LP_ERR;
	else
	   sleep(1);
	   continue;
      }else{
	break;
      }
    }
    
    lp_assert(c->conn_active == 0);
    lp_assert(c->conn_closing == 0);
    lp_assert(c->conn_closed == 0);

    if ((c->sb = ber_sockbuf_alloc()) == NULL) {
        lp_log_error("create lber Sockbuf failed!");
        lp_free_connection(c);
        return LP_ERR;
    }

    {
        //ber_len_t max = LP_SB_MAX_INCOMING_DEFAULT;
        ber_sockbuf_add_io(c->sb, &ber_sockbuf_io_tcp,
                           LBER_SBIOD_LEVEL_PROVIDER, (void *)(&(c->fd)));
        ber_sockbuf_ctrl(c->sb, LBER_SB_OPT_SET_NONBLOCK, (void *)1);
        //ber_sockbuf_ctrl(c->sb, LBER_SB_OPT_SET_MAX_INCOMING, &max);
    }

    c->cur_data = NULL;

    rev = c->rev;
    wev = c->wev;


    lp_task_t *task = lp_task_new();
    if (task == NULL) {
        lp_log_error("create connection io task failed!");
        lp_free_connection(c);
        return LP_ERR;
    }

    ldap = lp_get_ldap();

    lp_assert(ldap != NULL);

    c->task_manager = NULL;
    c->ldap = ldap;
    //
    lp_connection_set_discard_cb(c, lp_connection_discard_cb);
    c->conn_active = 1;

    c->uuid = gen_uuid(ldap->urandom, ldap->idx);


    task->sender = NULL;
    task->sender_source = LP_TASK_SOURCE_SERVER;
    task->receiver = ldap;
    task->receiver_source = LP_TASK_SOURCE_LDAP;
    task->task_type = LP_TASK_IO_ALL_REG;
    task->c = c;
    task->c_uuid = c->uuid;

    if (lp_ldap_push_task(ldap, task) == LP_ERR) {
        lp_log_error("push ldap io register task to task manager failed!");
        lp_task_destroy(task);
        lp_free_connection(c);
        return LP_ERR;
    }

    return LP_OK;
}

static int __ldap_notify(lp_ldap_t *ldap)
{
    lp_assert(ldap != NULL);

    int n;

    n = write(ldap->pipe_write, "lp", 1);
    if (n <= 0) {
        if (n == 0 || errno != EAGAIN || errno != EWOULDBLOCK) {
            lp_log_error("task manager notify failed!");
            return LP_ERR;
        }
    }

    return LP_OK;
}

/* push task use thread push */
#if 0
int lp_ldap_push_task(lp_ldap_t *ldap, lp_task_t *task)
{
    lp_assert(task != NULL);
    lp_log_debug("ldap addr 0x%lx", ldap);
    if (ldap == NULL) {
        lp_task_destroy(task);
        return LP_ERR;
    }
    if (stk_thread_push_task(ldap->thread, task) < 0) {
        lp_task_destroy(task);
        return LP_ERR;
    }
    lp_log_debug("ldap addr 0x%lx", ldap);
    return LP_OK;
}
#endif

int lp_ldap_push_task(lp_ldap_t *ldap, lp_task_t *task)
{
    if (ldap == NULL) return LP_ERR;
    //lp_assert(ldap != NULL);
    lp_assert(task != NULL);

    pthread_mutex_lock(&ldap->mutex);
    stk_list_add_tail(&task->list, &ldap->queue);
    pthread_mutex_unlock(&ldap ->mutex);

    if (__ldap_notify(ldap) == LP_ERR) {
        return LP_ERR;
    }

    return LP_OK;
}

static int __io_register(lp_ldap_t *ldap, event_t *event)
{
    TRACE();
    event_base_set(ldap->ev_base, event);

    struct timeval tv={lp_configs.timeout,0};
    
    if (event_add(event, &tv) < 0) {
        lp_log_error("add io event failed!");
        exit(2);
        return LP_ERR;
    }

    return LP_OK;
}

int lp_ldap_io_unregister(lp_connection_t *c)
{
    TRACE();

    lp_assert(c != NULL);
    
    return __io_unregister(c);
}

static int __ldap_init(lp_ldap_t *ldap)
{
    lp_assert(ldap != NULL);

    if (pipe(ldap->pipes) < 0) {
        return LP_ERR;
    }
    
    if (stk_nonblocking(ldap->pipe_read)) {
        return LP_ERR;
    }
    if (stk_nonblocking(ldap->pipe_write)) {
        return LP_ERR;
    }

    if ((ldap->ev_base = event_init()) == NULL) {
        return LP_ERR;
    }

    event_t *ev = (event_t *)stk_calloc(sizeof(event_t));
    if (ev == NULL) {
        return LP_ERR;
    }

    event_set(ev, ldap->pipe_read,
              EV_READ | EV_PERSIST, __ldap_notify_cb, ldap);
    event_base_set(ldap->ev_base, ev);
    event_add(ev, NULL);

    if ((ldap->thread = stk_thread_get()) == NULL) {
        return LP_ERR;
    }
    int urandom = open_urandom();
    if (urandom < 0) return LP_ERR;
    ldap->urandom = urandom;

    if (pthread_mutex_init(&ldap->mutex, NULL)) {
        TRACE();
        return LP_ERR;
    }

    stk_list_init(&ldap->queue);

    return LP_OK;
}

static int __ldap_get_ber(lp_connection_t *c)
{
    TRACE();

    ber_tag_t tag;
    ber_len_t len;
    //ber_int_t msgid;
    BerElement *ber;

    char err_buf[LP_ERR_STR_LEN] = {0};
    
    if (c->cur_data == NULL &&
        (c->cur_data = ldap_data_new()) == NULL) {
        lp_log_error("create ldap data failed!");
        return LP_ERR;
    }
    ber = c->cur_data->ber;

    tag = ber_get_next(c->sb, &len, ber);
    if (tag != LDAP_TAG_MESSAGE) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            strerror_r(errno, err_buf, LP_ERR_STR_LEN);
            lp_log_error("ber_get_next on fd:[%d] failed, errno:[%d] (%s)",
                          c->fd, errno, err_buf);
            ldap_data_destroy(c->cur_data);
            c->cur_data = NULL;
            return -1;
        }
        lp_log_info("ber_get_next store some bytes in connection's ber, "
                    "wait for the end of ber package!");
        return 1;
    }

#if 0
    if ((tag = ber_get_int(ber, &msgid)) != LDAP_TAG_MSGID) {
        ldap_data_destroy(c->cur_data);
        c->cur_data = NULL;
        return -1;
    }

#endif

    lp_log_info("add a ber to request list!");
    stk_list_add_tail(&c->cur_data->list, &c->req_list);
    c->cur_data = NULL;

    return LP_OK;
}

static int __ldap_read(lp_connection_t *c)
{
    TRACE();

    int ret;
    char err_buf[LP_ERR_STR_LEN] = {0};

    do {
        ret = __ldap_get_ber(c);
    } while (!ret);
    
    if (ret < 0) {
        strerror_r(errno, err_buf, LP_ERR_STR_LEN);
        lp_log_error("get BerElement from socket error! errno:(%d) %s",
                      errno, err_buf);
    }

    return ret;
}

static int __ldap_write(lp_connection_t *c)
{
    TRACE();

    stk_list_t *send_list;
    ldap_data_t *itr;
    BerElement *ber;

    send_list = &c->rpl_list;

    while (!stk_list_empty(send_list)) {
        itr = stk_list_first_entry(send_list, ldap_data_t, list);
        ber = itr->ber;
        
		while (true) {
            if (ber_flush2(c->sb, ber, LBER_FLUSH_FREE_NEVER) == 0) {
                lp_log_info("send a BerElement complete.");
                break;
            }
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                lp_log_error("connection(%d), socket(%d) write failed! "
                        "closing connection!", c->idx, c->fd);
                return LP_ERR;
            }

            lp_log_info("send part of a BerElement, wait for socket valid!");
            return LP_AGAIN;
        }
        itr = stk_list_pop_entry(send_list, ldap_data_t, list);
        ldap_data_destroy(itr);
    }

    return LP_OK;
}

static int __ldap_process_request(lp_connection_t *c)
{
    TRACE();

    int             ret;
    BerElement     *ber;
    ber_tag_t       tag;
    ber_len_t       len;
    ldap_data_t    *itr;
    lp_op_t         op_idx = LP_OP_LAST;
    ber_int_t       msgid;

    while (!stk_list_empty(&c->req_list)) {
        itr = stk_list_pop_entry(&c->req_list, ldap_data_t, list);
        ber = itr->ber;

#if 1
        if ((tag = ber_get_int(ber, &msgid)) != LDAP_TAG_MSGID) {
            lp_log_error("ber_get_int returns 0x%lx, not a valid ldap ber", tag);
            return -1;
        }
        lp_log_info("ber package msgid(%d)", msgid);
#endif

        if ((tag = ber_peek_tag(ber, &len)) == LBER_ERROR) {
            lp_log_error("ber_peek_tag returns 0x%lx", tag);
            ldap_data_destroy(itr);
            continue;
        }
        op_idx = lp_req2op(tag);
        lp_assert(op_idx != LP_OP_LAST);
        
        if (op_idx > LP_OP_SEARCH) {
            lp_log_error("not implemented request (%d)", op_idx);
            continue;
        }
	//will do search(push task to mysql) here        
        ret = (opfunc[op_idx])(ber, c, msgid);
        if (ret == LP_ERR) {
            lp_log_error("process opfunc[%d] failed!", op_idx);
        }
        ldap_data_destroy(itr);
    }

   // lp_log_debug("before ldap write, __ldap_process_request");
  //  __ldap_write(c);
   // lp_log_debug("fater ldap wrote,__ldap_process_request ");

    return LP_OK;
}

static int __io_unregister(lp_connection_t *c)
{
    TRACE();
    lp_assert(c != NULL);

    event_t *reg_ev = c->rev;

    c->conn_closing = 1;

    if (c->conn_closed == 1) {
        lp_log_info("connection has been closed, discard task");
        return LP_OK;
    }

    if (event_del(reg_ev) < 0) {
        lp_log_error("delete io register event failed!");
        return LP_ERR;
    }

    if (c->wev_active) {
        reg_ev = c->wev;
        if (event_del(reg_ev) < 0) {
            lp_log_error("delete io write event failed!");
            return LP_ERR;
        }
    }

    c->conn_closed = 1;

    return LP_OK;
}

/* XXX TODO: delete all task from the task queue */
#if 0
static int __ldap_io_unregister(lp_task_manager_t *task_manager,
                                lp_connection_t *c)
{
    TRACE();
    lp_task_t *task = lp_task_new();
    
    if (task == NULL) {
        lp_log_error("create task failed!");
        return LP_ERR;
    }

    c->conn_closing = 1;

    task->sender = c->ldap;
    task->sender_source = LP_TASK_SOURCE_LDAP;
    task->receiver = task_manager;
    task->receiver_source = LP_TASK_SOURCE_TASK_MANAGER;
    task->task_type = LP_TASK_IO_UNREG;
    task->c = c;
    task->c_uuid = c->uuid;

    if (lp_task_manager_push_task(task_manager, task) == LP_ERR) {
        lp_log_error("push ldap io unregister task to task manager failed!");
        lp_task_destroy(task);
        return LP_ERR;
    }

    return LP_OK;
}
#endif

static void lp_ldap_io_event_handler(int fd, short event_type, void *arg)
{
    TRACE();
    lp_ldap_t *ldap;


    lp_log_info("task type: %s, %s",
                (event_type & EV_READ) ? "IO READ" : "...",
                (event_type & EV_WRITE) ? "IO WRITE" : "...");
    
    lp_connection_t *c = (lp_connection_t *)arg;
    lp_assert(fd = c->fd);


    if (c->conn_closing == 1) {
        lp_log_info("connection[%d], socket(%d) closing, discard event",c->idx, c->fd);
        //lp_log_info("task type: %s, %s",(event_type & EV_READ) ? "IO READ" : "...",(event_type & EV_WRITE) ? "IO WRITE" : "...");
        return;
    }
    ldap = c->ldap;
    //
    if(event_type & EV_TIMEOUT){
            lp_log_info("EVENT TIMEOUT");
            __io_unregister(c);
            goto close_connection;
    }

#if 0
    if (c->conn_active != 1 || c->conn_closing == 1 || c->conn_closed == 1) {
        lp_log_error("connection was not active! %d, %d, %d",
                      c->conn_active, c->conn_active, c->conn_closed);
        return LP_ERR;
    }
#endif

    if (c->conn_active != 1) {
        lp_log_error("connection was not active! %d, %d, %d",
                      c->conn_active, c->conn_active, c->conn_closed);
        return;
    }

    if (c->conn_closing == 1) {
        if (c->conn_closed == 1) {
            goto close_connection;
        } else {
            lp_log_info("connection[%d] was closing(fd:%d), discard the task!",
                        c->idx, c->fd);
            return;
        }
    }


    if (event_type & EV_READ) {
        if (__ldap_read(c) < 0) {
            __io_unregister(c);
            //虽然client会发unbind,但read到error就提前close_conn了
            //unbind opfunc里在io_unregister时判断closed就返回了
            //不过似乎这样太依赖read的时序?应该在unbind里做一次lp_close_conn
        }
        if (__ldap_process_request(c) < 0) {
            return;
        }
    }
    if (event_type & EV_WRITE) {
        if (__ldap_write(c) < 0) {
            __io_unregister(c);
        }
    }
    
close_connection:
    if (c->conn_closed == 1) {
        lp_log_info("close connection(%d), socket: %d", c->idx, c->fd);
        lp_close_connection(c);
    }
    return;
}

#ifdef LDAP_TEST
static int __process_mysql_task(lp_task_t *task)
{
    TRACE();
    lp_assert(task != NULL);
    lp_assert(task->c != NULL);

    __ldap_write(task->c);

    return LP_OK;
}
#else
static int __process_mysql_task(lp_ldap_t *ldap, lp_task_t *task)
{
    TRACE();
    lp_assert(ldap != NULL);
    lp_assert(task != NULL);
    
    if (task->c == NULL || ldap != task->c->ldap)
        return LP_ERR;

    lp_assert(task->c != NULL);
    lp_assert(ldap == task->c->ldap);

    lp_connection_t *c;

    c = task->c;

    int ret = lp_search_reply(task, c);

    if (ret == LP_OK) {
		lp_log_debug("....will process mysql task");
        __ldap_write(task->c);
    }

    lp_task_destroy(task);

    return ret;
}
#endif

static int __process_server_task(lp_ldap_t *ldap,
                                 lp_task_t *task)
{
    lp_assert(ldap != NULL);
    lp_assert(task != NULL);
    lp_assert(task->c != NULL);
    int ret;

    lp_assert(task->task_type & LP_TASK_IO_ALL_REG);

    lp_connection_t *c = task->c;

    event_t *rev, *wev;
    rev = c->rev;
    wev = c->wev;
    event_set(rev, c->fd, EV_READ | EV_ET | EV_TIMEOUT|EV_PERSIST, lp_ldap_io_event_handler, c);
    event_set(wev, c->fd, EV_WRITE | EV_ET | EV_PERSIST, lp_ldap_io_event_handler, c);

    struct stat stat_buf = {0};
    if (fstat(c->fd, &stat_buf) < 0) {
        lp_log_error("connection(%d) client fd (%d) error(%d) %m", c->idx, c->fd, errno);
        exit(2);
    }

    if ((ret = __io_register(ldap, rev)) == LP_ERR) {
        __io_unregister(c);
        lp_task_destroy(task);
        return LP_ERR;
    }

    ret = __io_register(ldap, wev);
    if (ret == LP_OK) {
        c->wev_active = 1;
    } else {
        __io_unregister(c);
    }

    lp_task_destroy(task);
    
    return ret;
}

static int __task_process(lp_ldap_t *ldap, lp_task_t *task)
{
    lp_assert(task != NULL);
    lp_assert(ldap != NULL);

    int ret;

    if (task->sender_source == LP_TASK_SOURCE_MYSQL &&
            (lp_conn_check_task_tick(task->c, task) != LP_OK))
    {
	//lp_free_conn里会释放ticks，这里的检查防止链接失效后，mysql推回无效task
	//因为链接已经释放，且之前event已经注销，这里只需释放task
        lp_log_error("task time tick not valid! Drop the task!");
        lp_task_destroy(task);
        return LP_ERR;
    }

    switch (task->sender_source) {
        case LP_TASK_SOURCE_SERVER:
            ret = __process_server_task(ldap, task);
            break;

        case LP_TASK_SOURCE_MYSQL:
            ret = __process_mysql_task(ldap, task);
            break;
        default:
            lp_log_error("task process not implement source type");
            ret = LP_ERR;
    }

    return ret;
}

static void *__ldap_routine(void *arg)
{
    lp_ldap_t *ldap;
    ldap = (lp_ldap_t *)arg;

    lp_log_debug("start ldap process.............");
    event_base_dispatch(ldap->ev_base);

    /* fatal */
    exit(1);
    return NULL;
}

#if 0
static void *__ldap_routine(void *arg)
{
    lp_task_t *task;
    lp_ldap_t *ldap;
    
    ldap = (lp_ldap_t *)arg;

    while (true) {
        TRACE();
        task = (lp_task_t *)stk_thread_pop_task(ldap->thread);

        lp_assert(task->c != NULL);
        if (task->sender_source == LP_TASK_SOURCE_MYSQL &&
            (lp_conn_check_task_tick(task->c, task) != LP_OK))
        {
            lp_log_error("task time tick not valid! Drop the task!");
            lp_task_destroy(task);
            continue;
        }

        lp_assert(task->receiver == ldap);

        switch (task->sender_source) {
            case LP_TASK_SOURCE_SERVER:
                if (__process_server_task(task) == LP_ERR) {
                    lp_log_error("process server task failed!");
                }
                lp_log_debug("ldap index[%d],process complete, and delete task(%p)", ldap->idx, task);
                lp_task_destroy(task);
                break;
            case LP_TASK_SOURCE_TASK_MANAGER:
                if (__process_task_manager_task(task) == LP_ERR) {
                    lp_log_error("process task manager task failed!");
                }
                lp_log_debug("ldap index[%d],process complete, and delete task(%p)", ldap->idx, task);
                lp_task_destroy(task);
                break;
            case LP_TASK_SOURCE_MYSQL:
                if (__process_mysql_task(task) == LP_ERR) {
                    lp_log_error("process mysql task failed!");
                }
                lp_log_info("destroy mysql task...............................");
                lp_task_destroy(task);
                break;
            default:
                lp_log_error("ldap module not implemented task source");
                lp_task_destroy(task);
        }
    }
    return NULL;
}
#endif

static lp_task_t *__pop_task(lp_ldap_t *ldap)
{
    lp_assert(ldap!= NULL);
    lp_task_t *task;
    pthread_mutex_lock(&ldap->mutex);
    task = stk_list_pop_entry(&ldap->queue, lp_task_t, list);
    pthread_mutex_unlock(&ldap->mutex);

    return task;
}

static int lp_ldap_init()
{
    int i;
    lp_ldap_t *ldap;

    lp_ldaps = stk_calloc(sizeof(lp_ldap_t) * lp_configs.ldap_num);
    if (lp_ldaps == NULL) {
        lp_log_error("alloc memory of lp_ldaps failed!");
        return LP_ERR;
    }

    for (i = 0; i < lp_configs.ldap_num; ++i) {
        ldap = &lp_ldaps[i];
        ldap->idx = i;
        if (__ldap_init(ldap) == LP_ERR) {
            lp_log_error("init ldap failed!");
            return LP_ERR;
        }
        if (stk_create_thread(ldap->thread, __ldap_routine, ldap) == LP_ERR) {
            lp_log_error("create ldap thread failed!");
            return LP_ERR;
        }
    }

    //int ival = -1;
    //ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &ival );

    return LP_OK;
}

static void lp_ldap_exit()
{
    LP_MODULE_TRACE();
    stk_free(lp_ldaps);
    return;
}

stk_module_t lp_ldap_module = {
    STK_MODULE_V1,
    "LP_LDAP_MODULE",
    lp_ldap_init,
    lp_ldap_exit
};
