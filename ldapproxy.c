/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  ldapproxy.c:  Chen Huaying <chenyilong@sina.cn>
 *
 *  ldapproxy daemon.
 */

#include <stdlib.h>
#include <unistd.h>

#include "stk_comm.h"
#include "stk_module.h"
#include "stk_log.h"
#include "stk_signal.h"

#include "lp_comm.h"
#include "lp_log.h"
#include "lp_config.h"

static int debug_mode;
static int daemon_mode = 0;

extern void lp_server_start();

void lp_signal_handler(int signo)
{
    exit(EXIT_SUCCESS);
}

stk_signal_t signals[] = {
    { SIGINT,   "SIGINT, EXIT",     "", lp_signal_handler },
    { SIGTERM,  "SIGTERM, EXIT",    "", lp_signal_handler },
    { SIGPIPE,  "SIGPIPE, SIG_IGN", "", SIG_IGN },
    { SIGHUP,   "SIGHUP, SIG_IGN",  "", SIG_IGN },
    { 0, NULL, "", NULL}
};

int main(int argc, char *argv[])
{
    int i;

    while ((i = getopt(argc, argv, "tdf:")) != -1) {
        switch (i) {
            case 't':
                debug_mode = 1;
                daemon_mode = 0;
                break;
            case 'd':
                daemon_mode = 1;
                break;
            case 'f':
                lp_config_file = optarg;
                stk_log_info("config file: %s", lp_config_file);
                break;
            default:
                stk_log_info("Usage: ./ldapproxy [-t | -d]");
                exit(EXIT_FAILURE);
        }
    }
    if (daemon_mode) {
        stk_log_info("daemonizing.............");
        daemonize(1, 0);
    }
    stk_init_signals();
    if (stk_init_modules() == STK_MODULE_ERROR) {
        stk_log_error("stk_init_modules() failed!");
        stk_exit_modules();
        exit(1);
    }
    while (true) {
        lp_server_start();
        sleep(3);
    }
    stk_exit_modules();

    return 0;
}
