/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  lp_ldap_logic.h:  Chen Huaying <chenhuaying@sina.cn>
 *
 *  ldapproxy ldap logic process.
 */

#ifndef __LP_LDAP_LOGIC_H
#define __LP_LDAP_LOGIC_H

#define LP_ATTR_LEN     128
#define LP_ASSERT_LEN   LP_ATTR_LEN
#define LP_DN_LEN       (LP_ATTR_LEN * 2)

typedef enum { 
    LP_OP_BIND = 0,
    LP_OP_UNBIND,
    LP_OP_SEARCH,
    LP_OP_COMPARE,
    LP_OP_MODIFY,
    LP_OP_MODRDN,
    LP_OP_ADD,
    LP_OP_DELETE,
    LP_OP_ABANDON,
    LP_OP_EXTENDED,
    LP_OP_LAST
} lp_op_t;

typedef struct equalityMatch_s {
    char    attribute[LP_ATTR_LEN + 1];
    char    assertionValue[LP_ATTR_LEN + 1];
} equalityMatch_t;

typedef struct present_s {
    char    present[LP_ATTR_LEN + 1];
} present_t;

typedef int lp_op_func(BerElement *ber, lp_connection_t *c, ber_int_t msgid);

lp_op_t lp_req2op(ber_tag_t tag);
int lp_search_reply(lp_task_t *task, lp_connection_t *c);

extern lp_op_func *opfunc[];

#endif /*__LP_LDAP_LOGIC_H*/
