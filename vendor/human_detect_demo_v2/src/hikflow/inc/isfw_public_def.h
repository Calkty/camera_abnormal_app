/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         isfw_public_def.h
* @brief        general macro definition
*
* @date         2022/10/21
* @version      1.0.0
* @note         none 
*****************************************************************************/

#ifndef __ISFW_PUBLIC_DEF_H_
#define __ISFW_PUBLIC_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ISFW_PUBLIC_SUCCESS	0           /* succeed */
#define ISFW_PUBLIC_FAILURE	(-1)        /* fail */

#define ISFW_PUBLIC_LIB_PRT	printf("<ISFW_PUBLIC> File:<%s> Fun:[%s] Line:%d ", __FILE__, __FUNCTION__, __LINE__); printf


typedef /*signed*/ char	ISFW_INT8;
typedef unsigned char ISFW_UINT8;
typedef /*signed*/ short ISFW_INT16;
typedef unsigned short ISFW_UINT16;
typedef /*signed*/ int ISFW_INT32;
typedef unsigned int ISFW_UINT32;
typedef void *ISFW_VOIDPTR;
typedef void ISFW_VOID;
typedef /*signed*/ long	ISFW_LONG;
typedef unsigned long ISFW_ULONG;
typedef /*signed*/ long long ISFW_INT64;
typedef unsigned long long ISFW_UINT64;
typedef /*signed*/ char	ISFW_STRING;
typedef /*signed*/ long long ISFW_INT64;
typedef unsigned long long ISFW_UINT64;

#ifdef __cplusplus
}
#endif

#endif

