/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_ldap.h:  Chen Huaying <chenyilong@sina.cn>
 *
 *  ldapproxy ldap module.
 */

#ifndef __LP_LDAP_H
#define __LP_LDAP_H

#define LDAP_TEST
#undef  LDAP_TEST
#ifdef  LDAP_TEST
typedef struct ldap_rpl_data_s {
    char            data[1024];
    int             len;
    stk_list_t      list;
} ldap_rpl_data_t;
#else
typedef struct ldap_data_s ldap_data_t;
struct ldap_data_s {
    BerElement *ber;
    stk_list_t list;
};
#endif

int lp_ldap_push_task(lp_ldap_t *ldap, lp_task_t *task);
lp_ldap_t *lp_get_ldap();
int lp_ldap_io_register(stk_socket_t sockfd);
ldap_data_t *ldap_data_new();
void ldap_data_destroy(ldap_data_t *data);
int lp_ldap_io_unregister(lp_connection_t *c);

#define LP_SB_MAX_INCOMING_DEFAULT ((1<<18) - 1)

#endif /*__LP_LDAP_H*/
