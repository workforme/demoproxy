/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_server.c:  Chen Huaying <chenhuaying@sina.cn>
 *
 *  ldapproxy server, listening.
 */

#include "portable.h"
#include <event.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h> 

#include "stk_socket.h"
#include "stk_module.h"

#include "lber.h"
#include "lber-int.h"
#include "ldap.h"

#include "lp_comm.h"
#include "lp_task.h"
#include "lp_log.h"
#include "lp_config.h"
#include "lp_ldap.h"
#include "lp_connection.h"

event_t listen_ev;

static event_base_t *server_base;

static void __lp_listening_handler(int fd, short event_type, void *arg);


static void __lp_listening_handler(int fd, short event_type, void *arg)
{
    struct sockaddr_in client_addr;

    stk_socket_t client_fd;

    /*
     * accept in loop, listening socket must be nonblock, 
     * otherwise accetp may block when no connection
     */
    do {
	client_fd = stk_accept(fd, &client_addr);
        
        if (client_fd < 0) {
            lp_log_info("ldapproxy accept new connection over...");
            return;
        }

        if (stk_nonblocking(client_fd)) {
            lp_log_error("ldapproxy set socket(%d) nonblock failed!", client_fd);
            close(client_fd);
            return;
        }
       if (stk_socket_keepalive(client_fd)) {
            lp_log_error("ldapproxy set socket(%d) keepalive failed!", client_fd);
            close(client_fd);
            return;
        }

        struct stat stat_buf = {0};
        fstat(client_fd, &stat_buf);
        lp_log_info("accept fd(%d), ready for register, inode(%d)", client_fd, stat_buf.st_ino);
        if (lp_ldap_io_register(client_fd) == LP_ERR) {
            lp_log_error("ldapproxy register io failed!");
            close(client_fd);
        }
    
	struct sockaddr_in his_addr;
    	socklen_t addrlen = sizeof(his_addr);

    	if (getpeername(client_fd, ((struct sockaddr *)(&his_addr)), &addrlen) < 0) 
    	{
		lp_log_error("get ip error");
		return;
    	}else{
		lp_log_info("ldapproxy accept new connection from %s succ\n",inet_ntoa(his_addr.sin_addr));
        }

    } while (1);
}

static int lp_server_init()
{
    char *ip = lp_configs.listening_ip;
    int port = lp_configs.listening_port;

    if ((server_base = event_init()) == NULL) {
        return LP_ERR;
    }

    stk_socket_t server_fd = stk_listening_init(ip, port);
    if (server_fd < 0) {
        lp_log_error("listening on %s:%d failed!",
                      ((ip == NULL) ? "(null)" : ip), port);
        return LP_ERR;
    }
    event_set(&listen_ev, server_fd, EV_READ | EV_PERSIST,
              __lp_listening_handler, NULL);

    event_base_set(server_base, &listen_ev);

    event_add(&listen_ev, NULL);
    
    return LP_OK;
}

void lp_server_start()
{
    lp_log_debug("start server............");
    event_base_dispatch(server_base);
}

static void lp_server_exit()
{
    LP_MODULE_TRACE();
}

stk_module_t lp_server_module = {
    STK_MODULE_V1,
    "LP_SERVER_MODULE",
    lp_server_init,
    lp_server_exit
};
