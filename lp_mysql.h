/*
 *  Copyright (c) 2015 SINA Corporation, All Rights Reserved.
 *
 *  lp_mysql.h:  Yilong Zhao <yilong@sina.cn>
 *
 *  ldapproxy mysql module.
 */

#ifndef __LP_MYSQL_H
#define __LP_MYSQL_H

#define LP_MYSQL_DEST_LEN   128
#define LP_MYSQL_BASE_LEN   LP_MYSQL_DEST_LEN

typedef struct mysql_msg_s mysql_msg_t;
struct mysql_msg_s {
    int                     code;
    int                     msgid;
    char                    base[LP_MYSQL_BASE_LEN + 1];
    ber_tag_t               filter_type;
    union {
        present_t           present;
        equalityMatch_t     equalityMatch;
    };
};

int lp_mysql_push_task(lp_mysql_t *mysql, lp_task_t *task);
lp_mysql_t *lp_get_mysql();


#endif /*__LP_MYSQL_H*/
