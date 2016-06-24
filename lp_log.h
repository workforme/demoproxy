/*
 *  Copyright (c) 2015 SINA Corporation, All Rights Reserved.
 *
 *  lp_log.h:  Yilong Zhao yilong@sina.cn
 *
 *  ldapproxy log.
 */

#ifndef __LP_LOG_H
#define __LP_LOG_H

#include "stk_log.h"

typedef enum {
    LP_LOG_DEFAULT,
    LP_LOG_ERROR,
    LP_LOG_INFO,
    LP_LOG_DEBUG
} lp_log_types;

#define lp_log(log_type, fmt, ...)  \
    __lp_log(log_type, __FILE__, __LINE__, __func__, "<%lu> "fmt, pthread_self(), ##__VA_ARGS__)
#define lp_log_error(fmt, ...)  \
    lp_log(LP_LOG_ERROR, "[ERROR] "fmt, ##__VA_ARGS__)
#define lp_log_info(fmt, ...)   \
    lp_log(LP_LOG_INFO, "[INFO] "fmt, ##__VA_ARGS__)
#define lp_log_debug(fmt, ...)  \
    __lp_log(LP_LOG_DEBUG, __FILE__, __LINE__, __func__, "<%lu> [DEBUG] "fmt, pthread_self(), \
             ##__VA_ARGS__)

#define LP_MODULE_TRACE()   \
    do {stk_log_info("%s[%d], %s", __FILE__, __LINE__, __func__);}while(0)
#define TRACE() do{lp_log_debug("tracing");}while(0)

void __lp_log(lp_log_types log_type, const char *file, int line,
        const char *func, char *fmt, ...);

#endif /*__LP_LOG_H*/
