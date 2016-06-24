/*
 *  Copyright (c) 2015 SINA Corporation, All Rights Reserved.
 *
 *  test_connections.c:  Yilong Zhao <yilong@sina.cn>
 *
 *  test for connections.
 */

#include "portable.h"
#include <netinet/in.h>
#include <pthread.h>

#include "lber.h"
#include "lber-int.h"
#include "ldap.h"

#include "stk_list.h"
#include "stk_socket.h"
#include "lp_comm.h"
#include "lp_log.h"
#include "lp_task.h"
#include "lp_ldap.h"
#include "lp_connection.h"

void test_discard(stk_list_t *list)
{
    return;
}

void test1()
{
    lp_connection_t *c1;
    lp_connection_t *c2;
    c1 = lp_get_connection(100);
    lp_connection_set_discard_cb(c1, test_discard);
    lp_log_debug("c1: fd(%d) index(%d)", c1->fd, c1->idx);
    c2 = lp_get_connection(200);
    lp_connection_set_discard_cb(c2, test_discard);
    lp_log_debug("c2: fd(%d) index(%d)", c2->fd, c2->idx);
}

void test2()
{
    lp_connection_t *c1;
    lp_connection_t *c2;

    c1 = lp_get_connection(100);
    lp_connection_set_discard_cb(c1, test_discard);
    lp_log_debug("c1: fd(%d) index(%d)", c1->fd, c1->idx);
    lp_free_connection(c1);
    c2 = lp_get_connection(200);
    lp_connection_set_discard_cb(c2, test_discard);
    lp_log_debug("c2: fd(%d) index(%d)", c2->fd, c2->idx);
}
