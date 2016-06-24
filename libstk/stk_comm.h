/*
 *  Copyright (c) 2010 SINA Corporation, All Rights Reserved.
 *
 *  comm.h:  Chen Huaying chenhuaying@sina.cn
 *
 *  simple tool kit: common use.
 */

#ifndef __COMM_H
#define __COMM_H

#define STK_TRUE         1
#define STK_FALSE        0

#define STK_OK           0
#define STK_ERR         -1
#define STK_DECLINED    -2
#define STK_AGAIN       -3

int daemonize(int nochdir, int noclose);


#endif /*__COMM_H*/
