/*
 *  Copyright (c) 2015 SINA Corporation, All Rights Reserved.
 *
 *  lp_ldap_logic.c:  Yilong Zhao <yilong@sina.cn>
 *
 *  ldapproxy ldap logic process.
 */

#include <string.h>
#include <sys/socket.h>
#include <pthread.h>

#include "stk_socket.h"

#include "portable.h"
#include "lber.h"
#include "lber-int.h"
#include "ldap.h"

#include "lp_comm.h"
#include "lp_log.h"
#include "lp_task.h"
#include "lp_ldap_logic.h"
#include "lp_ldap.h"
#include "lp_mysql.h"
#include "lp_connection.h"
#include "lp_ldap_logic.h"

int lp_response_bind(lp_connection_t *c, ber_int_t msgid);

lp_op_t lp_req2op(ber_tag_t tag)
{
    switch (tag) {
        case LDAP_REQ_BIND:
            return LP_OP_BIND;
        case LDAP_REQ_UNBIND:
            return LP_OP_UNBIND;
        case LDAP_REQ_ADD:
            return LP_OP_ADD;
        case LDAP_REQ_DELETE:
            return LP_OP_DELETE;
        case LDAP_REQ_MODRDN:
            return LP_OP_MODRDN;
        case LDAP_REQ_MODIFY:
            return LP_OP_MODIFY; 
        case LDAP_REQ_COMPARE:  
            return LP_OP_COMPARE;
        case LDAP_REQ_SEARCH:
            return LP_OP_SEARCH;
        case LDAP_REQ_ABANDON:
            return LP_OP_ABANDON;
        case LDAP_REQ_EXTENDED:
            return LP_OP_EXTENDED;
    }

    return LP_OP_LAST;
}

int lp_do_bind(BerElement *ber, lp_connection_t *c, ber_int_t msgid)
{
    TRACE();
    ber_tag_t tag;
    ber_len_t len;
    ber_tag_t method;
    ber_int_t version;
    //char *name = NULL;
    int ret;

    if ((tag = ber_peek_tag(ber, &len)) == LBER_ERROR) {
        lp_log_error("ber_peek_tag returns 0x%lx", tag);
        return LP_ERR;
    }
    lp_assert(tag == LDAP_REQ_BIND);

    //tag = ber_scanf(ber, "{iat", &version, &name, &method);
    tag = ber_scanf(ber, "{it", &version, &method);
    if (tag == LBER_ERROR) {
        lp_log_error("ber_scanf failed!");
        return LP_ERR;
    }
    //lp_log_debug("vesrion: %d,\tname: %s, method: 0x%lx",
    //              version, name, method);
    //ber_memfree(name);

    ret = lp_response_bind(c, msgid);

    return ret;
}

int lp_response_bind(lp_connection_t *c, ber_int_t msgid)
{
    TRACE();

    int ret;
    ber_tag_t tag;
    ber_int_t code;
    BerElement *ber;
    ldap_data_t *data;

    if ((data = ldap_data_new()) == NULL) {
        return LP_ERR;
    }

    ber = data->ber;
    tag = LDAP_RES_BIND;
    code = 0;

    ret = ber_printf(ber, "{it{ess}}",
               msgid, tag, code, "", "");
    if (ret < 0) {
        lp_log_error("create bind response ber failed!");
        return LP_ERR;
    }
    
	lp_log_info("add a ber to reply list!");
    stk_list_add_tail(&data->list, &c->rpl_list);

    return LP_OK;
}

int lp_do_search(BerElement *ber, lp_connection_t *c, ber_int_t msgid)
{
    TRACE();

    int ret;
    ber_tag_t tag;
    ber_len_t len;
    char *base;
    char *attribute;
    char *assertionValue;
    int scope;
    int deref;
    int slimit;
    int tlimit;
    int attrsonly;

    if ((tag = ber_peek_tag(ber, &len)) == LBER_ERROR) {
        lp_log_error("ber_peek_tag returns 0x%lx", tag);
        return LP_ERR;
    }
    lp_assert(tag == LDAP_REQ_SEARCH);

    tag = ber_scanf(ber, "{aeeiib", &base, &scope, &deref, &slimit,
                          &tlimit, &attrsonly);
    if (tag == LBER_ERROR) {
        lp_log_error("ber_scanf failed!");
        return LP_ERR;
    }

    stk_log_info("base:[%s], scope:[%d], deref[%d], slimit:[%d], "
                 "tlimit:[%d], attrsonly:[%d]",
                  base, scope, deref, slimit, tlimit, attrsonly);

    if ((tag = ber_peek_tag(ber, &len)) == LBER_ERROR) {
        lp_log_error("ber_peek_tag returns 0x%lx", tag);
        return LP_ERR;
    }


    lp_task_t *task = lp_task_new();
    mysql_msg_t *query = stk_pcalloc(task->pool, sizeof(mysql_msg_t));
    strncpy(query->base, base, strlen(base));
    query->msgid = msgid;
    lp_mysql_t *mysql = lp_get_mysql();

    if (tag == LDAP_FILTER_EQUALITY) {
        query->filter_type = tag;

        /* obtain the content of the filter and restore in task */
        if (ber_scanf(ber, "{aa", &attribute, &assertionValue) == LBER_ERROR) {
            lp_log_error("ber_scanf filter failed!");
            return LP_ERR;
        }

        lp_log_info("attribute:[%s], assertionValue:[%s]",
                     attribute, assertionValue);
        strncpy(query->equalityMatch.attribute, attribute, strlen(attribute));
        strncpy(query->equalityMatch.assertionValue,
                assertionValue, strlen(assertionValue));
        ber_memfree(attribute);
        ber_memfree(assertionValue);
    }

    task->data = query;
    task->c = c;
    task->sender = c->ldap;
    task->sender_source = LP_TASK_SOURCE_LDAP;
    task->receiver = mysql;
    task->receiver_source = LP_TASK_SOURCE_MYSQL;
    task->task_type = LP_TASK_LDAP_MSG;

    ret = lp_mysql_push_task(mysql, task);

    ber_memfree(base);
    if (ret == LP_ERR) {
        lp_log_error("push query task to mysql failed!");
    }

    return ret;
}

static int __search_succeed(lp_task_t *task, lp_connection_t *c)
{
    TRACE();

    int msgid;
    BerElement *ber;
    ber_tag_t tag;
    int code;
    struct berval objectName;
    int ret;

    mysql_msg_t *reply = (mysql_msg_t *)task->data;
    msgid = reply->msgid;


    ldap_data_t *reply_data = ldap_data_new();
    if (reply_data == NULL) {
        lp_log_error("create reply_data failed!");
        return LP_ERR;
    }
    ber = reply_data->ber;

    tag = LDAP_RES_SEARCH_ENTRY;
    struct berval type = {11, "objectClass"};
    struct berval type_vals1 = {8, "dcObject"};
    struct berval type_vals2 = {12, "organization"};

    if (reply->filter_type == LDAP_FILTER_EQUALITY) {
        char dn[LP_DN_LEN + 1] = {0};
        /* mail is a partialAttribute */
        struct berval attr;
        struct berval attr_val;

        ret = snprintf(dn, LP_DN_LEN, "%s=%s,%s", reply->equalityMatch.attribute,
                                   reply->equalityMatch.assertionValue,
                                   reply->base);
        objectName.bv_len = ret;
        objectName.bv_val = stk_pcalloc(task->pool, objectName.bv_len);
        strncpy(objectName.bv_val, dn, objectName.bv_len);

        attr.bv_len = strlen(reply->equalityMatch.attribute);
        attr.bv_val = stk_pcalloc(task->pool, attr.bv_len);
        strncpy(attr.bv_val, reply->equalityMatch.attribute, attr.bv_len);

        attr_val.bv_len = strlen(reply->equalityMatch.assertionValue);
        attr_val.bv_val = stk_pcalloc(task->pool, attr_val.bv_len);
        strncpy(attr_val.bv_val, reply->equalityMatch.assertionValue, attr_val.bv_len);

        ret = ber_printf(ber, "{it{O{{O[O]}{O[OO]}}}}", msgid, tag, &objectName,
                               &attr, &attr_val, &type, &type_vals1, &type_vals2);

    } else {
        char *entry = reply->base;
        lp_assert(entry != NULL);
        objectName.bv_len = strlen(entry);
        objectName.bv_val = stk_pcalloc(task->pool, objectName.bv_len);
        strncpy(objectName.bv_val, entry, objectName.bv_len);
        ret = ber_printf(ber, "{it{O{{O[OO]}}}}", msgid, tag, &objectName,
                               &type, &type_vals1, &type_vals2);
    }
    if (ret == LBER_ERROR) {
        ldap_data_destroy(reply_data);
        lp_log_error("create search entry ber failed!");
        return LP_ERR;
    }

    /* 
     * note if the result entries just have one,
     * this is right, otherwise is fault.
     */
    tag  = LDAP_RES_SEARCH_RESULT;
    code = LDAP_SUCCESS;
    ret = ber_printf(ber, "{it{ess}}", msgid, tag, code, "", "");
    if (ret == LBER_ERROR) {
        lp_log_error("create search done ber failed!");
        return LP_ERR;
    }

    stk_list_add_tail(&reply_data->list, &c->rpl_list);
    
    return LP_OK;
}

static int __search_failed(lp_task_t *task, lp_connection_t *c)
{
    lp_assert(task != NULL);
    int         ret;
    int         msgid;
    int         code;
    ber_tag_t   tag;
    BerElement *ber;

    tag  = LDAP_RES_SEARCH_RESULT;
    /*
     * ironport's query base domain is aways right, mysql query
     * failed, just return no search entry with an success query
     * done ber
     * code = LDAP_NO_SUCH_OBJECT means base domain no exist;
     */
    code = LDAP_SUCCESS;


    mysql_msg_t *reply = (mysql_msg_t *)task->data;
    msgid = reply->msgid;

    ldap_data_t *reply_data = ldap_data_new();
    if (reply_data == NULL) {
        lp_log_error("create reply_data failed!");
        return LP_ERR;
    }

    ber = reply_data->ber;

    ret = ber_printf(ber, "{it{ess}}", msgid, tag, code, "", "");
    if (ret == LBER_ERROR) {
        lp_log_error("create search done ber failed!");
        return LP_ERR;
    }

    stk_list_add_tail(&reply_data->list, &c->rpl_list);

    return LP_OK;
}

int lp_search_reply(lp_task_t *task, lp_connection_t *c)
{
    TRACE();

    int ret;
    mysql_msg_t *reply = (mysql_msg_t *)task->data;
    int code = reply->code;
    switch (code) {
        case 0:
            ret = __search_succeed(task, c);
            break;
        case -1:
	    //printf("in search reply ret %d\n",code);
            ret = __search_failed(task, c);
            break;
        default:
            lp_log_error("not implemented code number");
    }

    return ret;
}

int lp_do_unbind(BerElement *ber, lp_connection_t *c, ber_int_t msgid)
{
    TRACE();
	lp_log_debug("will do io_unreg in unbind");
    return lp_ldap_io_unregister(c);
}

lp_op_func *opfunc[] = {
    lp_do_bind,
    lp_do_unbind,
    lp_do_search,
    NULL
};
