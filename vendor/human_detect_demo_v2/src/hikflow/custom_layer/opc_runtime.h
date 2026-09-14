/***************************************************************************************************
*
*Copyright Information: Copyright (c) 2010-2018, Hangzhou Hikvision Software Co., Ltd., All Rights Reserved
*
* File name: opc_runtime.h
* Abstract: platform general driver functions and macro definitions, etc
* Description:
***************************************************************************************************/


#ifndef _OPC_RUNTIME_H_
#define _OPC_RUNTIME_H_

#include <stddef.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))


#if (defined (_WIN64) || defined (_LINUX64) || defined (ARCHS_STANDARD_64_BIT))
    typedef  unsigned long long SIZE_T;
#else
    typedef unsigned int        SIZE_T;
#endif

/***************************************************************************************************
* Alignment calculations
***************************************************************************************************/
#define OPC_SIZE_ALIGN(size, align)  ((((unsigned int)(size)) + (((unsigned int)(align)) - 1)) & (~(((unsigned int)(align)) - 1)))
#define OPC_SIZE_ALIGN_64(size)      OPC_SIZE_ALIGN(size, 64)
#define OPC_SIZE_ALIGN_128(size)     OPC_SIZE_ALIGN(size, 128)

/*********************************************************************************
* Function: apply DDR memory
* Params: mem_addr    - I               DDR memory start address
*         offset      - I               effective address offset value
*         mem_size    - I               apply memory size
* Return: if result is success, it would return memory point; else return NULL
* Note:
***********************************************************************************/
void* OPC_RUNTIME_malloc(void* mem_addr, unsigned int offset, unsigned int mem_size);

/*********************************************************************************
* Funtion: release DDR memory
* Params: ptr    - I                memory point
* Return: if result is success, it would return 0; else return -1
* Note:
***********************************************************************************/
int OPC_RUNTIME_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif // _OPC_RUNTIME_H_

