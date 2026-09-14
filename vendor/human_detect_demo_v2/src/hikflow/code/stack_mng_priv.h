/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         stack_mng_priv.h
* @brief        stack manage private interface
*
* @date         2023-8-24
* @version      2.0.0
* @note         
*               
*****************************************************************************/

#ifndef _STACK_MNG_PRIV_H_
#define _STACK_MNG_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief interface of C
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <malloc.h>
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <dirent.h>
#include <elf.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>   
#include <sys/resource.h>
#include <sys/un.h>
#include <sys/prctl.h>
#include <linux/fb.h>
#include <libgen.h>
#include <getopt.h>
#include <stdarg.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <linux/sockios.h> 
#include <syslog.h>

#include "stack_mng.h"
#include "isfw_log.h"

/**
* @brief  stack mng print
*/
#define STACK_MNG_DBG(arg...)                   isfw_log_print("ST_MNG",ISFW_LOG_LEVEL_DEBUG, __FILE__,__LINE__,##arg)
#define STACK_MNG_LOG(arg...)                   isfw_log_print("ST_MNG",ISFW_LOG_LEVEL_INFO, __FILE__,__LINE__,##arg)
#define STACK_MNG_ERR(arg...)                   isfw_log_print("ST_MNG",ISFW_LOG_LEVEL_ERROR, __FILE__,__LINE__,##arg)
#define STACK_MNG_WRN(arg...)                   isfw_log_print("ST_MNG",ISFW_LOG_LEVEL_WARN, __FILE__,__LINE__,##arg)

#define STACK_MNG_RET(c,r)                      if(c){typeof(r) _ret_=r;STACK_MNG_ERR("ret failed %s ret=0x%x\n",#c,(int)_ret_); return _ret_;}
#define STACK_MNG_NORET(c,r)                    if(c){typeof(r) _ret_=r;STACK_MNG_ERR("noret failed %s ret=0x%x\n",#c,(int)_ret_); return;}
#define STACK_MNG_ASSER(c,r)                    if(c){typeof(r) _ret_=r;STACK_MNG_ERR("assert failed %s ret=0x%x\n",#c,(int)_ret_);}
#define STACK_MNG_KEY_RET(c,r,keys)             if(c){typeof(r) _ret_=r;STACK_MNG_ERR("key ret failed %s KEY[%s] ret=0x%x\n",#c,keys,(int)_ret_); return _ret_;}
#define STACK_MNG_EXIT(c,r,exit)                if(c){typeof(r) _ret_=r;STACK_MNG_ERR("exit failed %s ret=0x%x\n",#c,(int)_ret_); goto exit;}
#define STACK_MNG_KEY_EXIT(c,r,exit,keys)	    if(c){typeof(r) _ret_=r;STACK_MNG_ERR("key exit failed %s KEY[%s]  ret=0x%zx\n",#c,keys,(size_t)_ret_); goto exit;}

/**
* @brief  os memory alloc or free function
*/
#define STACK_MNG_ALLOC(_SIZE_,_ALIGGN_)        memalign(_ALIGGN_,_SIZE_)
#define STACK_MNG_FREE(_PVOID_)                 free(_PVOID_)
#define STACK_MNG_MEM_ALIGN                     (64)

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef __cplusplus
}
#endif

#endif

