/***************************************************************************
* note 2012-2029 HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* @file         json_proc.h
* @brief        json process interface
*
* @date         2023-8-24
* @version      2.0.0
* @note         1. supports adding alarm json
*               2. supports parse json_file
*****************************************************************************/

#ifndef _JSON_PROC_H_
#define _JSON_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif


/** 
* @brief            add alarm json 
*
* @param[in] 		alarm_target    target will be alarmed     
* @param[in] 		json_buf        json buffer will be filled     
* @param[in] 		json_max_len    maximum size of json buffer     
* @param[in] 		time            global time
* @param[in] 		jpeg_addr       jpeg encoder stream address     
* @param[in] 		jpeg_len        jpeg encoder stream length     
* 
* @return           0 if successful, otherwise an error number returned
*/
int json_proc_add_content_json(OPDEVSDK_POS_TARGET_ST* alarm_target,char *json_buf,int json_max_len,void *time,char *jpeg_addr,int jpeg_len);

/** 
* @brief            parse hikflow_config.json 
*
* @param[in] 		arg             pointer of handle     
* 
* @return           0 if successful, otherwise an error number returned
*/
int json_proc_parse_config_json(void* arg);

/** 
* @brief            parse hikflow_attr.json 
*
* @param[in] 		arg             pointer of handle     
* 
* @return           0 if successful, otherwise an error number returned
*/
int json_proc_parse_net_attr_json(void* arg);

#ifdef __cplusplus
}
#endif

#endif

