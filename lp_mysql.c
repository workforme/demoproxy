/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_mysql.c:  Chen Huaying <yilong@sina.cn>
 *
 *  ldapproxy mysql module.
 */

#include "portable.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "lber.h"
#include "lber-int.h"
#include "ldap.h"

#include "stk_socket.h"
#include "stk_threads.h"
#include "stk_module.h"
#include "stk_pool.h"
#include "stk_list.h"

#include "lp_comm.h"
#include "lp_log.h"
#include "lp_task.h"
#include "lp_ldap_logic.h"
#include "lp_mysql.h"
#include "lp_ldap.h"
#include "lp_config.h"
#include "lp_connection.h"
//#include "libfedb.h"

#include "sinaentauth_api.h" //@yilong

#define TASK_SATUR  1000

typedef enum {
    LP_MYSQL_NONE,
    LP_MYSQL_ACTIVE,
    LP_MYSQL_BUSY,
} lp_mysql_state;

struct lp_mysql_s {
    int                 idx;
    unsigned            task_num;
    lp_mysql_state      state;
    stk_thread_t       *thread;
};

static lp_mysql_t *lp_mysqls;
static int __index;

static int __mysql_init(lp_mysql_t *mysql);
static void *__mysql_routine(void *arg);

int lp_mysql_push_task(lp_mysql_t *mysql, lp_task_t *task)
{
    /* NOTE: it should be in ldap module!!!!!! */
    lp_conn_register_task_tick(task->c, task);

    if (stk_thread_push_task(mysql->thread, task) < 0) {
        return LP_ERR;
    }
    ++mysql->task_num;
    if (mysql->task_num > TASK_SATUR) {
        mysql->state = LP_MYSQL_BUSY;
    }

    return LP_OK;
}

static lp_task_t *lp_mysql_pop_task(lp_mysql_t *mysql)
{
    lp_task_t *task;
    task = (lp_task_t *)stk_thread_pop_task(mysql->thread);
    --mysql->task_num;
    if (mysql->task_num < TASK_SATUR * 7 / 8) {
        mysql->state = LP_MYSQL_ACTIVE;
    }

    return task;
}

lp_mysql_t *lp_get_mysql()
{
    lp_mysql_t *mysql = &lp_mysqls[__index];
    ++__index;
    if (__index >= lp_configs.mysql_num) {
        __index = 0;
    }

    return mysql;
}

static int __mysql_init(lp_mysql_t *mysql)
{
    mysql->task_num = 0;
    mysql->state = LP_MYSQL_ACTIVE;
    if ((mysql->thread = stk_thread_get()) == NULL) {
        return LP_ERR;
    }

    return LP_OK;
}

#ifdef  LDAP_TEST
static int __process_ldap_task(lp_mysql_t *mysql, lp_task_t *task)
{
    int ret;

    lp_connection_t *c = task->c;
    lp_assert(c != NULL);

    ldap_rpl_data_t *data = (ldap_rpl_data_t *)(task->data);

    lp_log_debug("content:[%s], len:(%d)", data->data, data->len);


    ldap_rpl_data_t *rpl_data = stk_calloc(sizeof(ldap_rpl_data_t));

    snprintf(rpl_data->data, 1024 - 1, "mysql(%d):%s", mysql->idx, data->data);
    rpl_data->len = strlen(rpl_data->data);
    stk_list_add_tail(&rpl_data->list, &c->rpl_list);
    lp_log_debug("mysql reply:[%s], len(%d)", rpl_data->data, rpl_data->len);


    lp_task_t *rpl_task = lp_task_new();

    rpl_task->sender = mysql;
    rpl_task->sender_source = LP_TASK_SOURCE_MYSQL;
    rpl_task->receiver = task->sender;
    rpl_task->receiver_source = LP_TASK_SOURCE_LDAP;
    rpl_task->c = c;
    rpl_task->task_type = LP_TASK_MYSQL_MSG;

    ret = lp_ldap_push_task((lp_ldap_t *)rpl_task->receiver, rpl_task);

    return ret;
}
#else
static inline void __set_tick2query(lp_task_t *task, lp_task_t *q_task)
{
    task->tick = q_task->tick;
}
static int __process_ldap_query(lp_mysql_t *mysql, lp_task_t *task)
{
    TRACE();
    
    lp_assert(task->c != NULL);

    int ret = 0;
    lp_connection_t *c;

    mysql_msg_t *query = (mysql_msg_t *)task->data;
    c = task->c;
    
    lp_log_debug("query base is [%s]", query->base);

    if (query->filter_type == LDAP_FILTER_EQUALITY) {
        if (strcasecmp("mail", query->equalityMatch.attribute) == 0) {
            
            char *mail = query->equalityMatch.assertionValue;
            lp_log_info("mail:[%s]", query->equalityMatch.assertionValue);
            
	    ret= ent_UserExist(mail);

    	    lp_log_debug("ent_UserExist(%s)  ret: [%d]",mail,ret);

            if(ret){
		ret=smaMailListExist(mail);
    	    	lp_log_debug("smaMailListExist(%s)  ret: [%d]",mail,ret);
	    }

        } else if(strcasecmp("domain", query->equalityMatch.attribute) == 0) {

	    ret=0;

        } else {
            ret = -1;
        }

        if (ret != 0) {
            lp_log_info("request(%s:[%s]) is not in local domain, error(%d)",
                query->equalityMatch.attribute,
                query->equalityMatch.assertionValue, ret);
        } else {
            lp_log_info("request(%s:[%s]) is in local domain!", 
                query->equalityMatch.attribute,
                query->equalityMatch.assertionValue);
        }
    }


    lp_task_t *res_task = lp_task_new();

    mysql_msg_t *reply = stk_pcalloc(res_task->pool, sizeof(mysql_msg_t));

    if ( ret == 0) {
        reply->code = LP_OK;
    } else {
        reply->code = LP_ERR;
    }

#if 0
    reply->code = LP_OK;
#endif

    strncpy(reply->base, query->base, LP_MYSQL_DEST_LEN);
    reply->msgid = query->msgid;
    reply->filter_type = query->filter_type;

    if (query->filter_type == LDAP_FILTER_EQUALITY) {
        lp_log_info("query filter attribute:[%s], assertionValue:[%s]",
                     query->equalityMatch.attribute,
                     query->equalityMatch.assertionValue);
        strncpy(reply->equalityMatch.attribute, query->equalityMatch.attribute,
                strlen(query->equalityMatch.attribute));
        strncpy(reply->equalityMatch.assertionValue,
                query->equalityMatch.assertionValue,
                strlen(query->equalityMatch.assertionValue));
    }


    res_task->c = c;
    res_task->data = reply;
    res_task->sender = mysql;
    res_task->sender_source = LP_TASK_SOURCE_MYSQL;
    res_task->receiver = c->ldap;
    res_task->receiver_source = LP_TASK_SOURCE_LDAP;
    res_task->task_type = LP_TASK_MYSQL_MSG;
    __set_tick2query(res_task, task);
#if 0 
    sleep(1);
#endif

    if ((ret = lp_ldap_push_task(c->ldap, res_task)) == LP_ERR) {
        lp_log_error("push query result to ldap failed!");
    }

    return ret;
}
static int __process_ldap_task(lp_mysql_t *mysql, lp_task_t *task)
{
    TRACE();
    int ret;

    switch (task->task_type) {
        case LP_TASK_LDAP_MSG:
            if ((ret = __process_ldap_query(mysql, task)) == LP_ERR) {
                lp_log_error("process ldap query error!");
            }
            break;
        default:
            lp_log_error("not implemented task type(%d)", task->task_type);
            abort();
            return LP_ERR;
    }

    return ret;
}
#endif

static void *__mysql_routine(void *arg)
{
    lp_mysql_t *mysql;
    lp_task_t *task;

    mysql = (lp_mysql_t *)arg;

    while (true) {
        TRACE();
        task = lp_mysql_pop_task(mysql);
        lp_assert(task != NULL);
        // lp_assert(task->receiver == mysql);

        // In the function lp_task_destroy of some where else,a task will be cleared
        // This may occured after lp_assert(task != NULL) and before lp_assert(task->receiver == mysql)
        // Then the second assert will cause the program to exit!!!
        // So we change the second assert to the if-checking showing as follow:

        if(task->receiver != mysql){
		lp_log_error("GOT task->receiver (%p) != mysql (%p)",task->receiver,mysql);
		lp_task_destroy(task);
		continue;
	}

        switch (task->sender_source) {
            case LP_TASK_SOURCE_LDAP:
                if (__process_ldap_task(mysql, task) == LP_ERR) {
                    lp_log_error("process ldap task failed!");
                }
                lp_log_info("destroy ldap task.................................");
                lp_task_destroy(task);//in __process_ldap_task will new task for reply from mysql to ldap
                break;

            default:
                lp_log_error("mysql module not implemented task source");
                lp_task_destroy(task);
        }
    }

    return NULL;
}

static int lp_mysql_init()
{
    int i;
    lp_mysql_t *mysql;
    lp_mysqls = stk_calloc(sizeof(lp_mysql_t) * lp_configs.mysql_num);
    if (lp_mysqls == NULL) {
        lp_log_error("alloc memory of mysqls failed!");
        return LP_ERR;
    }

    for (i = 0; i < lp_configs.mysql_num; ++i) {
        mysql = &lp_mysqls[i];
        mysql->idx = i;
        if (__mysql_init(mysql) == LP_ERR) {
            lp_log_error("init mysql failed!");
            return LP_ERR;
        }
        if (stk_create_thread(mysql->thread, __mysql_routine, mysql) == LP_ERR) {
            lp_log_error("create mysql thread failed!");
            return LP_ERR;
        }
    }

    return LP_OK;
}

static void lp_mysql_exit()
{
    LP_MODULE_TRACE();
    stk_free(lp_mysqls);
}

stk_module_t lp_mysql_module = {
    STK_MODULE_V1,
    "LP_MYSQL_MODULE",
    lp_mysql_init,
    lp_mysql_exit,
};
