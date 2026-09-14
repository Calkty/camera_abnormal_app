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

#include "hikflow_demo_priv.h"

#define JSON_PROC_MAX_ATTACH_NUMS   (10)
#define JSON_PROC_MAX_PIC_NAME      (32)

#define JSON_PROC_MIN(a,b)		((a) > (b) ? (b) : (a))
#define JSON_PROC_SAFE_CLOSE(fd) do { if (-1 != fd) { close(fd); fd = -1; } } while(0)      /*!< safely close file descriptor */
#define JSON_PROC_SAFE_FREE(x) do { if ((x) != NULL) { free((x)); (x) = NULL;} } while(0)   /*!< safely releasing dynamically allocated memory */

typedef struct _JSON_PROC_CUSTOM_PIC_INFO_
{
	int   pic_index;
	int   pic_len;
	char  pic_name[JSON_PROC_MAX_PIC_NAME];
	char *pic_data;
}JSON_PROC_CUSTOM_PIC_INFO;

typedef struct _JSON_PROC_CUSTOM_ACCESS_ST_
{
    /*!< basic description json,transparent json of third-party event json messages,which is belong to heop technical specifications */
	cJSON                       *direct_json_describe;
	int                         json_len;
    /*!< image attachment information */
	JSON_PROC_CUSTOM_PIC_INFO   picInfo[JSON_PROC_MAX_ATTACH_NUMS-1];
	int                         pic_nums;
}JSON_PROC_CUSTOM_ACCESS_ST;

#ifndef JSON_PROC_MAKE_TIME
#define JSON_PROC_MAKE_TIME(year, month, day, hour, minute, second) \
    ((((year - 2000) & 63) << 26)   \
     | ((month & 15) << 22)  \
     | ((day & 31) << 17)  \
     | ((hour & 31) << 12)  \
     | ((minute & 63) << 6)  \
     | ((second & 63) << 0))
#endif

/*!< fill header */
static int json_proc_fill_head_info(cJSON *pHeadInfo)
{
    HIKFLOW_RET(pHeadInfo == NULL,HIKFLOW_DEMO_ERR_NULL_PTR);

    cJSON *json_re = cJSON_AddNumberToObject(pHeadInfo, "version", 1);
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_NULL_PTR,"version add number err");

   	json_re = cJSON_AddStringToObject(pHeadInfo, "intelligentType", "customAlarm_demo");
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_NULL_PTR,"version intelligentType add string number err");

	json_re = cJSON_AddStringToObject(pHeadInfo, "eventType", "customAlarm_demo");
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_NULL_PTR,"eventType add string err");
  
    json_re = cJSON_AddNumberToObject(pHeadInfo, "intelligentVersion", 1);
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_NULL_PTR,"intelligentVersion add number err");
 
    json_re = cJSON_AddNumberToObject(pHeadInfo, "subVersion", 20211015);
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_NULL_PTR,"subVersion add number err");

	json_re = cJSON_AddNumberToObject(pHeadInfo, "channel", 1);
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_NULL_PTR,"subVersion add number err");
    return OPDEVSDK_S_OK;
}

/*!< fill body */
static int json_proc_fill_body_info(cJSON *pBodyInfo, OPDEVSDK_POS_TARGET_ST* alarm_target,OPDEVSDK_MEDIADRV_TIME_ST *time)
{
    HIKFLOW_RET(pBodyInfo == NULL,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(time == NULL,HIKFLOW_DEMO_ERR_NULL_PTR);

	cJSON *json_re = NULL;
    int abs_time = JSON_PROC_MAKE_TIME(time->year, time->month, time->day, \
        time->hour, time->minute, time->second);

    json_re = cJSON_AddNumberToObject(pBodyInfo, "absTime", abs_time);
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_FAILED,"absTime add number err");

    json_re = cJSON_AddNumberToObject(pBodyInfo, "millisecond", time->milliSecond);
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_FAILED,"millisecond add number err");

    json_re = cJSON_AddStringToObject(pBodyInfo, "bodyType", "humanDetect_demo");
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_FAILED,"bodyType add string err");

	cJSON *rect = cJSON_CreateObject();
    HIKFLOW_KEY_RET(json_re == NULL,HIKFLOW_DEMO_ERR_FAILED,"rect creat err");

    json_re = cJSON_AddNumberToObject(rect, "x", alarm_target->region.point[0].x);
    HIKFLOW_KEY_EXIT(json_re == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"rect x add number err");

    json_re = cJSON_AddNumberToObject(rect, "y", alarm_target->region.point[0].y);
    HIKFLOW_KEY_EXIT(json_re == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"rect y add number err");
    
    json_re = cJSON_AddNumberToObject(rect, "w", alarm_target->region.point[2].x - alarm_target->region.point[0].x);
    HIKFLOW_KEY_EXIT(json_re == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"rect w add number err");
    json_re = cJSON_AddNumberToObject(rect, "h", alarm_target->region.point[2].y - alarm_target->region.point[0].y);
    HIKFLOW_KEY_EXIT(json_re == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"rect h add number err");
    
    cJSON_AddItemToObject(pBodyInfo, "Rect", rect);
    HIKFLOW_DBG("json_proc_fill_body_info ok\n");
    return HIKFLOW_DEMO_OK;
err0:
    cJSON_Delete(rect);
    HIKFLOW_ERR("json_proc_fill_body_info err\n");
    return HIKFLOW_DEMO_ERR_FAILED;
}

static int json_proc_custom_access_demo(JSON_PROC_CUSTOM_ACCESS_ST *accessInfo,char *jpeg_addr,int jpeg_len)
{
	OP_DEVSDK_MIME_UNIT_ST stInfo[5];
	OP_DEVSDK_MESSAGEPUBLISH_INPUT_ST message_input;

	char topic[16] = {0};
	char *basicJson = NULL, *picInfo = NULL;
	int fd = -1, jsonLen = 0, picLen = 0, ret = 0;
	cJSON *root = NULL, *eventInfoObj = NULL , *LinkageInfoObj = NULL , *ftpInfoArr= NULL, *ftpInfoObj= NULL, *EmailInfo= NULL, *EmailAttachInfoArr= NULL, *EmailAttachInfoObj= NULL ,* VoiceInfoObj = NULL; 
	cJSON *VoiceInfoArr= NULL, *WhiteLightInfoArr= NULL, *WhiteLightInfoObj= NULL, *irLightInfoArr= NULL, *irLightInfoObj= NULL, *protocolTypeObj = NULL;	
	int ftpAttachNums = 0, EmailAttachNums = 0, VoiceAttachNums = 0, WhiteLightAttachNums = 0, irLightAttachNums = 0;

	memset(&message_input,0,sizeof(message_input));
	memset(stInfo,0,sizeof(stInfo));
	memset(topic,0,sizeof(topic));

	memcpy(topic, "HEOPCustomEvent", strlen("HEOPCustomEvent"));
	message_input.p_topic = topic;

    /*!< in_buffer_num refers to the total number of attachments connected to third-party alarms, which must have a basic json information description, 
    that is, the following root is used to describe the type (eventType), channel number (channelID), and platform/output/peripheral devices to be linked (linkageInfo) of this alarm.
	The other one here is an image, which refers to the alarm picture. Of course, you can also use audio files, images, and so on;
    messagePublish_inputParam description can be found in the interface input parameter structure descriptions of the third-party external publishing protocol document */

	message_input.in_buffer_num = 2;
	message_input.p_in_buffer = (char *)(&stInfo[0]);

	root = cJSON_CreateObject();
    HIKFLOW_KEY_RET(root == NULL,HIKFLOW_DEMO_ERR_FAILED,"root creat err");

	eventInfoObj = cJSON_CreateObject();
    HIKFLOW_KEY_EXIT(eventInfoObj == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"eventInfoObj creat err");

	cJSON_AddNumberToObject(eventInfoObj, "channelID", 1);
	cJSON_AddStringToObject(eventInfoObj, "dateTime", "2004-05-03T17:30:08+08:00");
	cJSON_AddStringToObject(eventInfoObj, "eventType", "HEOPCustomEvent");
	cJSON_AddStringToObject(eventInfoObj, "eventDescription", "HEOP Custom Event");
	cJSON_AddStringToObject(eventInfoObj, "eventState", "active");
	cJSON_AddStringToObject(eventInfoObj, "subEventType", "event1");
	cJSON_AddItemToObject(root, "eventInfo", eventInfoObj);

	/*!< IMPORTANT!!! this HEOPCustomEvent refers to the actual message content used by the customer, which can be transparently transmitted under this node */
	cJSON_AddItemToObject(eventInfoObj, "HEOPCustomEvent", accessInfo->direct_json_describe);

	LinkageInfoObj = cJSON_CreateObject();
    HIKFLOW_KEY_EXIT(LinkageInfoObj == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"LinkageInfoObj creat err");
	cJSON_AddItemToObject(root, "linkageInfo", LinkageInfoObj);
    
	ftpInfoArr = cJSON_CreateArray();
    HIKFLOW_KEY_EXIT(ftpInfoArr == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"ftpInfoArr creat err");
	cJSON_AddItemToObject(LinkageInfoObj, "FTP", ftpInfoArr);

	/*!< this is the number of alarm ftps */
	ftpAttachNums = 1;
	for (int i = 0; i < JSON_PROC_MIN(16, ftpAttachNums); i++)
	{
        ftpInfoObj = cJSON_CreateObject();
        HIKFLOW_KEY_EXIT(ftpInfoObj == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"ftpInfoObj creat err"); 
		cJSON_AddItemToArray(ftpInfoArr, ftpInfoObj);      
		cJSON_AddNumberToObject(ftpInfoObj, "id", 1);
        /*!< this refers to the user's attachment information, for example, this example is an alarm picture,
        after it is transmitted to the PC, what do you want his name to be? the path refers to the root directory set by FTP, but you can also specify a directory */
		cJSON_AddStringToObject(ftpInfoObj, "fileName", "test.jpg");
		cJSON_AddStringToObject(ftpInfoObj, "path", "./");
	}

    EmailInfo = cJSON_CreateObject();
    HIKFLOW_KEY_EXIT(EmailInfo == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"EmailInfo creat err"); 
	cJSON_AddItemToObject(LinkageInfoObj, "email", EmailInfo);
	cJSON_AddStringToObject(EmailInfo, "title", "custom demo titile test");
	cJSON_AddStringToObject(EmailInfo, "content", "custom demo content test");
	
    EmailAttachInfoArr = cJSON_CreateArray();
    HIKFLOW_KEY_EXIT(EmailAttachInfoArr == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"EmailAttachInfoArr creat err"); 
	cJSON_AddItemToObject(EmailInfo, "attachmentInfo", EmailAttachInfoArr);

    /*!< this is the number of alarm emails */
	EmailAttachNums = 1;
	for (int i = 0; i < JSON_PROC_MIN(16, EmailAttachNums); i++)
	{
        EmailAttachInfoObj = cJSON_CreateObject();
        HIKFLOW_KEY_EXIT(EmailAttachInfoObj == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"EmailAttachInfoObj creat err"); 

		cJSON_AddNumberToObject(EmailAttachInfoObj, "id", 1);
		cJSON_AddStringToObject(EmailAttachInfoObj, "fileName", "test.jpg");
		cJSON_AddItemToArray(EmailAttachInfoArr, EmailAttachInfoObj);
	}

  
    WhiteLightInfoArr = cJSON_CreateArray();
    HIKFLOW_KEY_EXIT(WhiteLightInfoArr == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"WhiteLightInfoArr creat err"); 
	cJSON_AddItemToObject(LinkageInfoObj, "whiteLight", WhiteLightInfoArr);

    /*!< this is the number of alarm white lights */
	WhiteLightAttachNums = 1;
	for (int i = 0; i < JSON_PROC_MIN(16, WhiteLightAttachNums); i++)
	{
        WhiteLightInfoObj = cJSON_CreateObject();
        HIKFLOW_KEY_EXIT(WhiteLightInfoObj == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"WhiteLightInfoObj creat err"); 
		cJSON_AddNumberToObject(WhiteLightInfoObj, "id", 1);
		cJSON_AddNumberToObject(WhiteLightInfoObj, "duration", 5);
		cJSON_AddStringToObject(WhiteLightInfoObj, "frequencyLevel", "high");
		cJSON_AddItemToArray(WhiteLightInfoArr, WhiteLightInfoObj);
	}

    irLightInfoArr = cJSON_CreateArray();
    HIKFLOW_KEY_EXIT(irLightInfoArr == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"irLightInfoArr creat err"); 
	cJSON_AddItemToObject(LinkageInfoObj, "whiteLight", irLightInfoArr);
    
    /*!< this is the number of alarm IR light */
	irLightAttachNums = 1;
	for (int i = 0; i < JSON_PROC_MIN(16, irLightAttachNums); i++)
	{
        irLightInfoObj = cJSON_CreateObject();
        HIKFLOW_KEY_EXIT(irLightInfoObj == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"irLightInfoObj creat err"); 
		cJSON_AddNumberToObject(irLightInfoObj, "id", 1);
		cJSON_AddNumberToObject(irLightInfoObj, "duration", 5);
		cJSON_AddStringToObject(irLightInfoObj, "frequencyLevel", "high");
		cJSON_AddItemToArray(irLightInfoArr, irLightInfoObj);
	}
	
    add_link_json_object_by_config(LinkageInfoObj);
    
	basicJson = cJSON_Print(root);
	jsonLen = strlen(basicJson);
	HIKFLOW_DBG("basicJson[%d]:\n%s\n", jsonLen, basicJson);
	
	/*!< the first one is the json description information, which users need to construct int according to the format in the HEOP published technical message. 
    The transparent message is added to the HEOPCustomimEvent node, while the rest remains unchanged */
	memcpy(stInfo[0].name, "heopCustom", strlen("heopCustom"));
	memcpy(stInfo[0].content_type, "application/json", strlen("application/json"));
	stInfo[0].p_content = basicJson;
	stInfo[0].content_len = jsonLen;

	memcpy(stInfo[1].name, "test", strlen("test"));
	memcpy(stInfo[1].content_type, "image/jpeg", strlen("image/jpeg"));
	stInfo[1].p_content = jpeg_addr;	
	stInfo[1].content_len = jpeg_len;    
	ret = opdevsdk_message_publish(&message_input);
    HIKFLOW_KEY_EXIT(ret != 0,HIKFLOW_DEMO_ERR_FAILED,err0,"opdevsdk_message_publish err"); 

	JSON_PROC_SAFE_FREE(basicJson);
	cJSON_Delete(root);
    HIKFLOW_DBG("json_proc_custom_access_demo ok\n");
	return HIKFLOW_DEMO_OK;
err0:
	cJSON_Delete(root);
    root = NULL;
    HIKFLOW_ERR("json_proc_custom_access_demo err\n");
	return HIKFLOW_DEMO_ERR_FAILED;
}

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
int json_proc_add_content_json(OPDEVSDK_POS_TARGET_ST* alarm_target,char *json_buf,int json_max_len,void *time,char *jpeg_addr,int jpeg_len)
{
    HIKFLOW_RET(alarm_target == NULL,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(json_buf == NULL,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(0 >= json_max_len,HIKFLOW_DEMO_ERR_INV_PARAM);
    HIKFLOW_RET(time == NULL,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(jpeg_addr == NULL,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_RET(0 >= jpeg_len,HIKFLOW_DEMO_ERR_INV_PARAM);

	int ret = 0;
	JSON_PROC_CUSTOM_ACCESS_ST access_info;
    cJSON *pHeadInfo = NULL;
    cJSON *pAlarmJson = NULL;
    cJSON *pBodyInfo = NULL;
    OPDEVSDK_MEDIADRV_TIME_ST *global_time = (OPDEVSDK_MEDIADRV_TIME_ST *)time;

	memset(&access_info, 0, sizeof(access_info));
    pAlarmJson = cJSON_CreateObject();
    HIKFLOW_KEY_RET(pAlarmJson == NULL,HIKFLOW_DEMO_ERR_FAILED," pAlarmJson cJSON_CreateObject err");

	pHeadInfo = cJSON_CreateObject();
    HIKFLOW_KEY_EXIT(pHeadInfo == NULL,HIKFLOW_DEMO_ERR_FAILED,err0,"pHeadInfo cJSON_CreateObject err");

	ret = json_proc_fill_head_info(pHeadInfo);
    HIKFLOW_KEY_EXIT(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,err1,"demo_smart_hikflow_json_head_info err");
    cJSON_AddItemToObject(pAlarmJson, "HeadInfo", pHeadInfo);
    pHeadInfo = NULL;

	pBodyInfo = cJSON_CreateObject();
    HIKFLOW_KEY_EXIT(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,err1,"pBodyInfo cJSON_CreateObject err");

	ret = json_proc_fill_body_info(pBodyInfo, alarm_target,global_time);
    HIKFLOW_KEY_EXIT(ret != HIKFLOW_DEMO_OK,HIKFLOW_DEMO_ERR_FAILED,err2,"json_proc_fill_body_info err");

    cJSON_AddItemToObject(pAlarmJson, "BodyInfo", pBodyInfo);
    pBodyInfo = NULL;
    
	/*!< use third-party alarm line custom_alarm_access_demo to access */
	access_info.direct_json_describe = pAlarmJson;
	(void)json_proc_custom_access_demo(&access_info,jpeg_addr,jpeg_len);
    HIKFLOW_DBG("json_proc_fill_body_info ok\n");
	return OPDEVSDK_S_OK;
err2:    
    if(pBodyInfo)
    {
        cJSON_Delete(pBodyInfo);
        pBodyInfo = NULL;
    } 
err1:
    if(pHeadInfo)
    {
        cJSON_Delete(pHeadInfo);
        pHeadInfo = NULL;
    }
err0:

    if(pAlarmJson)
    {
        cJSON_Delete(pAlarmJson);
        pAlarmJson = NULL;
    }
    HIKFLOW_ERR("json_proc_fill_body_info err\n");
    return HIKFLOW_DEMO_ERR_FAILED;    
}


static int json_proc_get_int_val(cJSON *parent, char *name, int def_val)
{
	cJSON *sub_obj = cJSON_GetObjectItem(parent, name);
	if(sub_obj &&cJSON_IsNumber(sub_obj))
	{
		return sub_obj->valueint;
	}

    HIKFLOW_ERR("json_proc_get_int_val name(%s) err\n",name);
	return def_val;
}   

static double json_proc_get_double_val(cJSON *parent, char *name, double def_val)
{
	cJSON *sub_obj = cJSON_GetObjectItem(parent, name);
	if(sub_obj &&cJSON_IsNumber(sub_obj))
	{
		return sub_obj->valuedouble;
	}

    HIKFLOW_ERR("json_proc_get_double_val name(%s) err\n",name);
	return def_val;
} 

static void* json_proc_get_string_val(cJSON *parent, char *name, void* def_val)
{
	cJSON *sub_obj = cJSON_GetObjectItem(parent, name);
	if(sub_obj &&cJSON_IsString(sub_obj))
	{
		return sub_obj->valuestring;
	}

    HIKFLOW_ERR("json_proc_get_string_val name(%s) err\n",name);
	return def_val;
} 

static int json_proc_change_fps(float fps)
{
    int fps_idx = 5;
    if(fps >= 30)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_30;
    }
    else if(fps >= 25)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_25;
    }
    else if(fps >= 20)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_15;
    }
    else if(fps >= 15)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_15;
    }
    else if(fps >= 12.5)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_12_5;
    }
    else if(fps >= 10)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_10;
    }
    else if(fps == 8.33)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_8_33;
    }
    else if(fps >= 8)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_8;
    }
    else if(fps >= 6.25)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_6_25;
    }
    else if(fps >= 1)
    {
        fps_idx= OPDEVSDK_VIDEO_FRAME_RATE_1;
    }
    else if(fps >= 0)
    {
        fps_idx = OPDEVSDK_VIDEO_FRAME_RATE_FULL;
    }

    return fps_idx;
}
/** 
* @brief            parse hikflow_config.json 
*
* @param[in] 		arg             pointer of handle     
* 
* @return           0 if successful, otherwise an error number returned
*/
int json_proc_parse_config_json(void* arg)
{
    HIKFLOW_RET(NULL == arg,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_DEMO_CTRL *pCtrl = (HIKFLOW_DEMO_CTRL *)arg;
    HIKFLOW_DEMO_CONFIGURATION_ST *config_data = &pCtrl->hikflow_config;
    
    int     ret  = 0,len = 0;
    FILE   *fp  = NULL;
    char *buf = NULL;

    char file[HIKFLOW_DEMO_MAX_STRING_LEN_EX] = {0};
    snprintf(file,HIKFLOW_DEMO_MAX_STRING_LEN_EX,"%s/%s",pCtrl->path,HIKFLOW_DEMO_CONFIG_FILE);

    /*!< open json file:hikflow_config.json */
    ret = stack_mng_open_file(file,&buf,&len);
    HIKFLOW_LOG("open json file %s\n", file);
    HIKFLOW_KEY_EXIT(HIKFLOW_DEMO_OK!= ret || buf == NULL || len == 0,HIKFLOW_DEMO_ERR_FAILED,err0,"stack_mng_open_file err");
    
    cJSON *json = NULL,*net_info = NULL,*file_info = NULL;

    /*!< parse json */
    json = cJSON_Parse(buf);
    HIKFLOW_KEY_EXIT(json == NULL,HIKFLOW_DEMO_ERR_FAILED,err1,"cJSON_Parse err");

    net_info = cJSON_GetObjectItem(json, "net");
    HIKFLOW_KEY_EXIT(net_info == NULL,HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetObjectItem net err");

    int arry_size = cJSON_GetArraySize(net_info);
    HIKFLOW_KEY_EXIT((arry_size < 1),HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetArraySize net err");

    for(int i = 0; i < arry_size; i++)
    {
        cJSON *info_json  = cJSON_GetArrayItem(net_info, i);
        HIKFLOW_KEY_EXIT((NULL == info_json),HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetArrayItem net err");

        /*!< model path*/
        char *model_info = json_proc_get_string_val(info_json, "model_path",NULL);
        HIKFLOW_KEY_EXIT(model_info == NULL ,HIKFLOW_DEMO_ERR_FAILED,err2,"json_proc_get_string_val model_path err");
        snprintf(config_data->model_path,HIKFLOW_DEMO_MAX_STRING_LEN_EX,"%s/%s",pCtrl->path,model_info);
        HIKFLOW_LOG("config_data->model_path %s\n", model_info);

        /*!< read frame rate,pay attention: fps will be changed to value in OPDEVSDK_VIDEO_FRAME_RATE_EN */
        float fps  = json_proc_get_double_val(info_json, "fps",HIKFLOW_DEMO_DEF_ALG_FPS);
        config_data->net_input.fps = json_proc_change_fps(fps);

        /*!< read video buffer counts */
        int vb_cnt  = json_proc_get_int_val(info_json, "vb_cnt",HIKFLOW_DEMO_DEF_VB_CNT);
        config_data->net_input.vb_cnt = vb_cnt;
        break;
    }

    /*!< read alarm information */    
    cJSON *alarm_json  = cJSON_GetObjectItem(json, "alarm");
    HIKFLOW_KEY_EXIT((NULL == alarm_json),HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetArrayItem net err");

    /*!< control whether to make alarm ,def is true */    
    int b_alarm  = json_proc_get_int_val(alarm_json, "b_alarm",1);
    config_data->b_alarm = b_alarm;

    /*!< control how long to make alarm information,def is 1 second */    
    int alarm_interval  = json_proc_get_int_val(alarm_json, "alarm_interval",HIKFLOW_DEMO_DEF_ALARM_INTER);
    config_data->alarm_intrval = alarm_interval;

    /*!< control whether to select attribute to make text */    
    int sel_class  = json_proc_get_int_val(alarm_json, "sel_class",-1);
    config_data->sel_class = sel_class;

    /*!< for test in FILE mode */    
    char *image_list = json_proc_get_string_val(json, "image_list",NULL);
    HIKFLOW_KEY_EXIT(image_list == NULL ,HIKFLOW_DEMO_ERR_FAILED,err0,"json_proc_get_string_val image_list err");
    snprintf(config_data->image_list,HIKFLOW_DEMO_MAX_STRING_LEN_EX,"%s/%s",pCtrl->path,image_list);

    if(json)
    {
        cJSON_Delete(json); 
        json = NULL;
    }    
    if(buf)
    {
        stack_mng_close_file(buf); 
        buf = NULL;
    }
    
    HIKFLOW_DBG("json_proc_parse_json ok \n");   
    return HIKFLOW_DEMO_OK;
err2:
    if(json)
    {
        cJSON_Delete(json); 
        json = NULL;
    }    
err1:
    if(buf)
    {
        stack_mng_close_file(buf); 
        buf = NULL;
    }
err0:
    HIKFLOW_ERR("json_proc_parse_config_json failed\n");
    return HIKFLOW_DEMO_ERR_FAILED;  
}

/** 
* @brief            parse hikflow_attr.json 
*
* @param[in] 		arg             pointer of handle     
* 
* @return           0 if successful, otherwise an error number returned
*/
int json_proc_parse_net_attr_json(void* arg)
{
    HIKFLOW_RET(NULL == arg,HIKFLOW_DEMO_ERR_NULL_PTR);
    HIKFLOW_DEMO_CTRL *pCtrl = (HIKFLOW_DEMO_CTRL *)arg;
    HIKFLOW_DEMO_CONFIGURATION_ST *config_data = &pCtrl->hikflow_config;
    
    int     ret  = 0,len = 0;
    FILE   *fp  = NULL;
    char *buf = NULL;

    char file[HIKFLOW_DEMO_MAX_STRING_LEN_EX] = {0};
    snprintf(file,HIKFLOW_DEMO_MAX_STRING_LEN_EX,"%s/%s",pCtrl->path,HIKFLOW_DEMO_ATTR_FILE);

    /*!< open json file:hikflow_attr.json */
    ret = stack_mng_open_file(file,&buf,&len);
    HIKFLOW_LOG("open json file %s\n", file);
    HIKFLOW_KEY_EXIT(HIKFLOW_DEMO_OK!= ret || buf == NULL || len == 0,HIKFLOW_DEMO_ERR_FAILED,err0,"stack_mng_open_file err");
    
    cJSON *json = NULL,*net_info = NULL,*file_info = NULL;

    /*!< parse json */
    json = cJSON_Parse(buf);
    HIKFLOW_KEY_EXIT(json == NULL,HIKFLOW_DEMO_ERR_FAILED,err1,"cJSON_Parse err");

    net_info = cJSON_GetObjectItem(json, "model");
    HIKFLOW_KEY_EXIT(net_info == NULL,HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetObjectItem net err");

    int arry_size = cJSON_GetArraySize(net_info);
    HIKFLOW_KEY_EXIT((arry_size < 1),HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetArraySize net err");
    int break_flg = 0;
    HIKFLOW_DBG("arry_size %d\n", arry_size);
    for(int i = 0; i < arry_size; i++)
    {
        cJSON *info_json  = cJSON_GetArrayItem(net_info, i);
        HIKFLOW_KEY_EXIT((NULL == info_json),HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetArrayItem model err");

        /*!< read model path*/
        char *model_info = json_proc_get_string_val(info_json, "name",NULL);
        HIKFLOW_KEY_EXIT(model_info == NULL ,HIKFLOW_DEMO_ERR_FAILED,err2,"json_proc_get_string_val model_path err");
        char name_file[HIKFLOW_DEMO_MAX_STRING_LEN_EX] = {0};
        snprintf(name_file,HIKFLOW_DEMO_MAX_STRING_LEN_EX,"%s/%s",pCtrl->path,model_info);
        HIKFLOW_DBG("i %d name_file %s\n", i,name_file);
        
        /*!< get the same model*/
        if(strcmp(name_file,config_data->model_path)!= 0)
        {
            HIKFLOW_LOG("idx %d %s is not %s\n", i,name_file,config_data->model_path);
            continue;
        }
        else
        {
            break_flg = 1;
        }
        
        /*!< read attribute information */    
        cJSON *attr_json  = cJSON_GetObjectItem(info_json, "attribute");
        HIKFLOW_KEY_EXIT((NULL == attr_json),HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetObjectItem attribute err");
        
        int attr_size = cJSON_GetArraySize(attr_json);
        HIKFLOW_KEY_EXIT((attr_size < 1),HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetArraySize attribute err");

        int idx = 0;
        HIKFLOW_DBG("attr_size %d\n", attr_size);
        for(int j = 0; j < attr_size && idx < HIKFLOW_DEMO_MAX_ATTR_NUM; j++)
        {
            cJSON *attr_array_json  = cJSON_GetArrayItem(attr_json, j);
            HIKFLOW_KEY_EXIT((NULL == attr_array_json),HIKFLOW_DEMO_ERR_FAILED,err2,"cJSON_GetArrayItem attribute err");
            
            int attr_idx  = json_proc_get_int_val(attr_array_json, "index",-1);
            HIKFLOW_KEY_EXIT(attr_idx == -1,HIKFLOW_DEMO_ERR_FAILED,err2,"json_proc_get_int_val index err");

            char *attr_name = json_proc_get_string_val(attr_array_json, "name",NULL);
            HIKFLOW_KEY_EXIT(attr_name == NULL ,HIKFLOW_DEMO_ERR_FAILED,err2,"json_proc_get_string_val name err");

            pCtrl->net_info.attr_list.attr[idx].idx = attr_idx;
            snprintf(pCtrl->net_info.attr_list.attr[idx].name,64,"%s",attr_name);
            HIKFLOW_LOG("j %d name%s id %d\n", j,pCtrl->net_info.attr_list.attr[idx].name,pCtrl->net_info.attr_list.attr[idx].idx);

            idx++;
        }
        pCtrl->net_info.attr_list.attr_num = idx;
        HIKFLOW_LOG("pCtrl->net_info.attr_list.attr_num %d\n", pCtrl->net_info.attr_list.attr_num);
        
        if(break_flg)
        {
            break;
        }
    }

    if(json)
    {
        cJSON_Delete(json); 
        json = NULL;
    }    
    if(buf)
    {
        stack_mng_close_file(buf); 
        buf = NULL;
    }
    HIKFLOW_DBG("json_proc_parse_net_attr_json(%d)attr ok \n",pCtrl->net_info.attr_list.attr_num);   
    return HIKFLOW_DEMO_OK;
err2:
    if(json)
    {
        cJSON_Delete(json); 
        json = NULL;
    }    
err1:
    if(buf)
    {
        stack_mng_close_file(buf); 
        buf = NULL;
    }
err0:
    HIKFLOW_ERR("json_proc_parse_net_attr_json failed\n");
    return HIKFLOW_DEMO_ERR_FAILED;  
}

