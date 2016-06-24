/*
 *  Copyright (c) 2015 SINA Corporation, All Rights Reserved.
 *
 *  lp_log.c:  Yilong Zhao <yilong@sina.cn>
 *
 *  ldapproxy log.
 */

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#include "lp_log.h"
#include "lp_config.h"
#include "stk_module.h"

#define LP_PROGRAM_NAME     "ldapproxy"

typedef void (*lp_log_process_t)(char *fmt, va_list ap);

static void __log_error(char *fmt, va_list ap);
static void __log_info(char *fmt, va_list ap);
static void __log_debug(char *fmt, va_list ap);

typedef struct lp_log_module_s {
    lp_log_process_t error;
    lp_log_process_t info;
    lp_log_process_t debug;
} lp_log_module_t;

static lp_log_module_t __log_module = {
    .error = __log_error,
    .info = NULL,
    .debug = NULL,
};

static int config_log_level;

static int lp_log_init()
{
    /* XXX TODO: read this configuration from config file */
    config_log_level = lp_configs.log_level;

    openlog(LP_PROGRAM_NAME, LOG_NDELAY | LOG_PID, LOG_LOCAL2);

    __log_module.info =
        LP_LOG_INFO <= config_log_level ? __log_info : NULL;

    __log_module.debug =
        LP_LOG_DEBUG <= config_log_level ? __log_debug : NULL;

    return 0;
}

static void lp_log_exit()
{
    LP_MODULE_TRACE();
}

void __lp_log(lp_log_types log_type, const char *file, int line,
        const char *func, char *fmt, ...)
{
    va_list ap;
    char msg[1024] = {0};

    openlog(LP_PROGRAM_NAME, LOG_NDELAY | LOG_PID, LOG_LOCAL2);
    snprintf(msg, sizeof(msg), "%s[%d] %s: \"%s\"\n", file, line, func, fmt);

    va_start(ap, fmt);
#if 0
    printf("msg:[%s]\n", msg);
    printf("msg............");
    vprintf(msg, ap);
#endif
    switch (log_type) {
        case 1:
            if (__log_module.error)
                __log_module.error(msg, ap);
            break;
        case 2:
            if (__log_module.info)
                __log_module.info(msg, ap);
            break;
        case 3:
            if (__log_module.debug)
                __log_module.debug(msg, ap);
            break;
        default:
            stk_log_error("log_type(%d) not implemented", log_type);
    }
    va_end(ap);
}

static void __log_error(char *fmt, va_list ap)
{
#ifdef CONSOLE_OUT
    stk_log_vmsg(LIGHT | F_RED, fmt, ap); 
#else
    vsyslog(LOG_ERR, fmt, ap);
#endif
}

static void __log_info(char *fmt, va_list ap)
{
#ifdef CONSOLE_OUT
    stk_log_vmsg(LIGHT | F_PURPLE, fmt, ap);
#else
    vsyslog(LOG_INFO, fmt, ap);
#endif
}

static void __log_debug(char *fmt, va_list ap)
{
#ifdef CONSOLE_OUT
    stk_log_vmsg(LIGHT | F_BLUE, fmt, ap);
#else
    vsyslog(LOG_DEBUG, fmt, ap);
#endif
}

stk_module_t lp_log_module = {
    STK_MODULE_V1,
    "LP_LOG_MODULE",
    lp_log_init,
    lp_log_exit
};
