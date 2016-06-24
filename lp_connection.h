/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_connection.h:  Chen Huaying <chenhuaying@sina.cn>
 *
 *  ldapproxy connection manager.
 */

#ifndef __LP_CONNECTION_H
#define __LP_CONNECTION_H

typedef void (*lp_discard_cb)(stk_list_t *list);

typedef enum {
    LP_CONN_NONE,
    LP_CONN_LISTEN,
    LP_CONN_LDAP_CLIENT,
    LP_CONN_MYSQL_CLIENT
} lp_conn_type;

struct lp_connection_s {
    char               ip[LP_IP_LEN + 1];          /* remote ip */
    int                port;                       /* remote port */
    int                fd;

    lp_conn_type       type;
    int                idx;                        /* index in the global connections, for monitor */

    Sockbuf           *sb;
    ldap_data_t       *cur_data;                   /* store current BerElement */

    void              *data;                       /* used to make the connetions into a chain */

    union {
        event_t       *listen_ev;                  /* listening socket event */
        event_t       *rev;                        /* socket read event */
    };
    event_t           *wev;                        /* socket write event */
    unsigned           conn_active:1;              /* connection was using by a socket */
    unsigned           conn_closing:1;             /* connection was closing by worker */
    unsigned           conn_closed:1;              /* connection was closed, rev & wev deleted */
    unsigned           wev_active:1;               /* if the write event is added */

    lp_task_manager_t *task_manager;
    lp_ldap_t         *ldap;
    lp_mysql_t        *mysql;
    
    stk_list_t         task_ticks;                 /* task ticks for the query task to other thread */
    int                uuid;                      /* connection uuid */

    lp_discard_cb      discard;                    /* callback to fre request and reply data */
    stk_list_t         req_list;                   /* list header of request */
    stk_list_t         rpl_list;                   /* list header of reply */
};

lp_connection_t *lp_get_connection(int sockfd);
/*
 * before add event succeed, just use this function.
 * This will discard all off the request and reply packages,
 * but not close the socket of the connection.
 */
void lp_free_connection(lp_connection_t *c);
/*
 * call this function after read and write event is deleted.
 * This will close fd, free connection, buf don't delete event,
 * take the task_manager to do this
 */
void lp_close_connection(lp_connection_t *c);
void lp_connection_set_discard_cb(lp_connection_t *c, lp_discard_cb cb);


#endif /*__LP_CONNECTION_H*/
