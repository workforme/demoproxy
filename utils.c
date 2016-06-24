/*
 *  Copyright (c) 1996 - 2014 SINA Corporation, All Rights Reserved.
 *
 *  utils.c:  Huaying <yilong@staff.sina.com.cn>
 *  
 *  uitls.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open_urandom()
{
    int u = open("/dev/urandom", O_RDONLY);
    if (u < 0)
        return -1;
    return u;

}

int gen_uuid(int urandom, int offset)
{
    int uuid;
    ssize_t n = pread(urandom, &uuid, sizeof(uuid), offset);
    if (n < 0) return -1;
    
    return uuid;
}
