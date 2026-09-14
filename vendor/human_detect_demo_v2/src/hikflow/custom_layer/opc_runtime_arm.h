/***************************************************************************************************
*
*Copyright Information: Copyright (c) 2010-2018, Hangzhou Hikvision Software Co., Ltd., All Rights Reserved
*
* File name: opc_runtime_arm.h
* Abstract: declaration file for driver function under OPC framework and ARM platform-specific
* Description:
***************************************************************************************************/
#ifndef _OPC_RUNTIME_ARM_H_
#define _OPC_RUNTIME_ARM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************
* Function: Sets the number of threads at runtime
* Params: thread    - I               The number of threads
* Return: if result is success, it would return 0; else return err code
* Note: 
***********************************************************************************/
int OPC_RUNTIME_set_thread_num(int _thread_num);


#ifdef __cplusplus
}
#endif

#endif