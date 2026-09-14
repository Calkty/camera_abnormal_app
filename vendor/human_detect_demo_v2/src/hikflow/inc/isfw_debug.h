/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         isfw_debug.h
* @brief        debug module for test
*                       
* @date         2023-8-29
* @version      2.0.1
* @note     
*****************************************************************************/

#ifndef _ISFW_DEBUG_H_
#define _ISFW_DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

/** 
* @brief            initialize isfw_debug for receive debug information
*           
* @param[in]        name       module name for distinguish each other     
*
* @param[out]       none
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_debug_server_init(char *name);

/** 
* @brief            de-initialize isfw_debug 
*           
* @param[in]        none     
*
* @param[out]       none
* 
* @return           0 if successful, otherwise an error number returned
*/
int isfw_debug_server_deinit();

#ifdef __cplusplus
}
#endif

#endif

