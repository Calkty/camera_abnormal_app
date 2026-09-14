#include <cgi_page.h>
#include <string.h>
#include <cJSON.h>
#include <protocol_target_detection_do.h>
#include <protocol_target_detection.h>
#include <protocol_status.h>
#include <syslog.h>

#define TRIGGER_NUM 1




static int get_target_detect_capa_json(cJSON *root)
{
    cJSON *js_targetDetectCapa = NULL;
    cJSON *js_targetDetectEnabled = NULL;
    cJSON *js_targetDetectRegionCap = NULL;
    cJSON *js_RegionCapX = NULL;
    cJSON *js_RegionCapY = NULL;
    cJSON *js_targetDetectLinkageCap = NULL;

    if (NULL == root)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "param is NULL\n");
        return PRO_DEV_ERR;
    }
    
    cJSON_AddItemToObject(root, "humanDetectDemoCap", js_targetDetectCapa = cJSON_CreateObject());
    cJSON_AddItemToObject(js_targetDetectCapa, "enabled", js_targetDetectEnabled = cJSON_CreateObject());
    cJSON_AddStringToObject(js_targetDetectEnabled, "@opt", "true,false");
    cJSON_AddStringToObject(js_targetDetectEnabled, "@def", "true");

    cJSON_AddItemToObject(js_targetDetectCapa, "RegionCap", js_targetDetectRegionCap = cJSON_CreateObject());
    cJSON_AddNumberToObject(js_targetDetectRegionCap, "minsize", 3);
    cJSON_AddNumberToObject(js_targetDetectRegionCap, "maxsize", 10);
    
    cJSON_AddItemToObject(js_targetDetectRegionCap, "x", js_RegionCapX = cJSON_CreateObject());
    cJSON_AddNumberToObject(js_RegionCapX, "@min", 0);
    cJSON_AddNumberToObject(js_RegionCapX, "@max", 1000);

    cJSON_AddItemToObject(js_targetDetectRegionCap, "y", js_RegionCapY = cJSON_CreateObject());
    cJSON_AddNumberToObject(js_RegionCapY, "@min", 0);
    cJSON_AddNumberToObject(js_RegionCapY, "@max", 1000);

    cJSON_AddItemToObject(js_targetDetectCapa, "LinkageCap", js_targetDetectLinkageCap = cJSON_CreateObject());
    cJSON_AddBoolToObject(js_targetDetectLinkageCap, "isSupportAudioOut", 1);

    cJSON_AddBoolToObject(js_targetDetectLinkageCap, "isSupportAlarmOut", 1);
    cJSON_AddBoolToObject(js_targetDetectLinkageCap, "isSupportCenter", 1);
    cJSON_AddBoolToObject(js_targetDetectLinkageCap, "isSupportStorage", 1);
    cJSON_AddBoolToObject(js_targetDetectLinkageCap, "isSupportRecord", 1);
    /*暂时不支持抓拍*/
    cJSON_AddBoolToObject(js_targetDetectLinkageCap, "isSupportCapture", 0);


    return PRO_OK;

}

static int get_target_detect_cfg_json(cJSON *root, APP_CFG *p_target_detect_cfg)
{
    cJSON *js_targetDetect = NULL;
    cJSON *js_targetDetectRegion = NULL;
    cJSON *js_RegionCoordinates = NULL;
    cJSON *js_RegionPoint = NULL;
    cJSON *js_targetDetectTrigger = NULL;
    cJSON *js_targetDetectNotification = NULL;
    cJSON *js_Notification = NULL;
    cJSON *js_Center = NULL, *js_Storage = NULL, *js_alarmOut = NULL, *js_alarmOut_arr = NULL, *js_alarmOut_obj = NULL;
    cJSON *js_record = NULL, *js_record_arr = NULL, *js_record_obj = NULL;
    cJSON *js_cap = NULL, *js_cap_arr = NULL, *js_cap_obj = NULL;
    
    int i = 0;

    
    if (NULL == root || NULL == p_target_detect_cfg)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "param is NULL\n");
        return PRO_DEV_ERR;
    }
    
    cJSON_AddItemToObject(root, "humanDetectDemo", js_targetDetect = cJSON_CreateObject());
    cJSON_AddBoolToObject(js_targetDetect, "enabled", p_target_detect_cfg->enable);

    cJSON_AddItemToObject(js_targetDetect, "humanDetectDemoRegion", js_targetDetectRegion = cJSON_CreateObject());
    js_RegionCoordinates = cJSON_AddArrayToObject(js_targetDetectRegion, "RegionCoordinates");

    for (i = 0 ; i < p_target_detect_cfg->region.point_num ; i ++ )
    {
        js_RegionPoint = cJSON_CreateObject();
        cJSON_AddItemToArray(js_RegionCoordinates, js_RegionPoint);
        cJSON_AddNumberToObject(js_RegionPoint, "x", p_target_detect_cfg->region.pos[i].x);
        cJSON_AddNumberToObject(js_RegionPoint, "y", p_target_detect_cfg->region.pos[i].y);
    }
    
    cJSON_AddItemToObject(root, "EventTrigger", js_targetDetectTrigger = cJSON_CreateObject());
    js_targetDetectNotification = cJSON_AddArrayToObject(js_targetDetectTrigger, "EventTriggerNotification");
    if(p_target_detect_cfg->voiceLinkage)
    {
        for (i = 0 ; i < TRIGGER_NUM ; i ++ )
        {  
            js_Notification = cJSON_CreateObject();
            cJSON_AddItemToArray(js_targetDetectNotification, js_Notification);
            cJSON_AddStringToObject(js_Notification, "id", "audioOut");
            cJSON_AddStringToObject(js_Notification, "notificationMethod", "audioOut");
            cJSON_AddStringToObject(js_Notification, "notificationRecurrence", "beginning");
            
        }
    }

    if(p_target_detect_cfg->is_center_enable)
    {
        js_Center = cJSON_CreateObject();
        cJSON_AddItemToArray(js_targetDetectNotification, js_Center);
        cJSON_AddStringToObject(js_Center, "id", "uploadCenter");
        cJSON_AddStringToObject(js_Center, "notificationMethod", "uploadCenter");
        cJSON_AddStringToObject(js_Center, "notificationRecurrence", "beginning");    
        
        cJSON_AddStringToObject(js_Center, "enable", "true");    
    }

    if(p_target_detect_cfg->is_storage_enable)
    {   
        js_Storage = cJSON_CreateObject();
        cJSON_AddItemToArray(js_targetDetectNotification, js_Storage);
        cJSON_AddStringToObject(js_Storage, "id", "Storage");
        cJSON_AddStringToObject(js_Storage, "notificationMethod", "Storage");
        cJSON_AddStringToObject(js_Storage, "notificationRecurrence", "beginning");    
    
        cJSON_AddStringToObject(js_Storage, "enable", "true");    
    }  

    
    js_alarmOut = cJSON_CreateObject();
    cJSON_AddItemToArray(js_targetDetectNotification, js_alarmOut);
    cJSON_AddStringToObject(js_alarmOut, "id", "alarmOut");
    cJSON_AddStringToObject(js_alarmOut, "notificationMethod", "IO");
    cJSON_AddStringToObject(js_alarmOut, "notificationRecurrence", "beginning");    

    cJSON_AddStringToObject(js_alarmOut, "enable", "true");  
    js_alarmOut_arr = cJSON_AddArrayToObject(js_alarmOut, "outputIOPortID");
    for (i = 0 ; i < MAX_ALARMOUT_NUMS ; i ++ )
    {        
        if(p_target_detect_cfg->alarmOutInfo[i].enable)   
        {
            js_alarmOut_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(js_alarmOut_obj, "id", p_target_detect_cfg->alarmOutInfo[i].id);
            cJSON_AddItemToArray(js_alarmOut_arr, js_alarmOut_obj);   
        }
    }


    js_record = cJSON_CreateObject();
    cJSON_AddItemToArray(js_targetDetectNotification, js_record);
    cJSON_AddStringToObject(js_record, "id", "Record");
    cJSON_AddStringToObject(js_record, "notificationMethod", "Record");
    cJSON_AddStringToObject(js_record, "notificationRecurrence", "beginning");    

    cJSON_AddStringToObject(js_record, "enable", "true");  
    js_record_arr = cJSON_AddArrayToObject(js_record, "recordChan");
    for (i = 0 ; i < MAX_RECORD_NUMS ; i ++ )
    {        
        if(p_target_detect_cfg->alarmRecordInfo[i].enable)   
        {
            js_record_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(js_record_obj, "chan", p_target_detect_cfg->alarmRecordInfo[i].id);
            cJSON_AddItemToArray(js_record_arr, js_record_obj); 
        }
    }

    js_cap = cJSON_CreateObject();
    cJSON_AddItemToArray(js_targetDetectNotification, js_cap);
    cJSON_AddStringToObject(js_cap, "id", "Capture");
    cJSON_AddStringToObject(js_cap, "notificationMethod", "Capture");
    cJSON_AddStringToObject(js_cap, "notificationRecurrence", "beginning");    

    cJSON_AddStringToObject(js_cap, "enable", "true");  
    js_cap_arr = cJSON_AddArrayToObject(js_cap, "captureChan");
    for (i = 0 ; i < MAX_CAPTURE_NUMS ; i ++ )
    {        
        if(p_target_detect_cfg->alarmCapInfo[i].enable)   
        {
            js_cap_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(js_cap_obj, "chan", p_target_detect_cfg->alarmCapInfo[i].id);
            cJSON_AddItemToArray(js_cap_arr, js_cap_obj);
        }
    }

    return PRO_OK;

}

static int put_target_detect_cfg_json(cJSON *root, APP_CFG *p_target_detect_cfg)
{
    cJSON *targetDetectDemoNode = NULL;
    cJSON *targetDetectEnable = NULL;
    cJSON *targetDetectRegion = NULL;
    cJSON *RegionCoordinatesArr = NULL;
    cJSON *RegionCoordinates = NULL;
    
    cJSON *RegionX = NULL;
    cJSON *RegionY = NULL;
    cJSON *EventTriggerNode = NULL;
    cJSON *NotificationArr = NULL;
    cJSON *Notification = NULL;
    cJSON *NotificationMethod = NULL;

    cJSON *alarmOutArr = NULL, *alarmOutObj = NULL;
    cJSON *recordArr = NULL, *recordObj = NULL;
    cJSON *capArr = NULL, *capObj = NULL;
    cJSON *tmpValueObj = NULL;
    
    int i = 0, j = 0;
    int region_size = 0, alarmOutSize = 0, recordSize = 0, capSize = 0;
    
    if(NULL == root || NULL == p_target_detect_cfg)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "put_target_detect_cfg_json error, NULL config\n");
        return PRO_DEV_ERR;
    }

    targetDetectDemoNode = cJSON_GetObjectItem(root, "humanDetectDemo");
    targetDetectEnable = cJSON_GetObjectItem(targetDetectDemoNode, "enabled");

    if (1 == targetDetectEnable->type)
    {
        p_target_detect_cfg->enable = 0;
    }
    else
    {
        p_target_detect_cfg->enable = 1;
    }

    targetDetectRegion = cJSON_GetObjectItem(targetDetectDemoNode, "humanDetectDemoRegion");
    RegionCoordinatesArr = cJSON_GetObjectItem(targetDetectRegion, "RegionCoordinates");
    region_size = cJSON_GetArraySize(RegionCoordinatesArr);
    p_target_detect_cfg->region.point_num = region_size;

    for (i = 0 ; i < region_size ; i ++)
    {
        RegionCoordinates = cJSON_GetArrayItem(RegionCoordinatesArr, i);
        if (NULL != RegionCoordinates)
        {
            RegionX = cJSON_GetObjectItem(RegionCoordinates, "x");
            if (NULL != RegionX)
            {
                p_target_detect_cfg->region.pos[i].x = RegionX->valueint;
            }
            RegionY = cJSON_GetObjectItem(RegionCoordinates, "y");
            if (NULL != RegionY)
            {
                p_target_detect_cfg->region.pos[i].y = RegionY->valueint;
            }
        }
    }

    EventTriggerNode = cJSON_GetObjectItem(root, "EventTrigger");
    NotificationArr = cJSON_GetObjectItem(EventTriggerNode, "EventTriggerNotification");

    region_size = cJSON_GetArraySize(NotificationArr);

    for (i = 0 ; i < region_size ; i ++)
    {
        Notification = cJSON_GetArrayItem(NotificationArr, i);
        
        if (NULL != Notification)
        {
            NotificationMethod = cJSON_GetObjectItem(Notification, "notificationMethod");
            if (0 == strcmp(NotificationMethod->valuestring, "audioOut"))
            {
                p_target_detect_cfg->voiceLinkage = 1;
            }
            else if(0 == strcmp(NotificationMethod->valuestring, "uploadCenter"))
            {
                p_target_detect_cfg->is_center_enable = 1;
            }
            else if(0 == strcmp(NotificationMethod->valuestring, "Storage"))
            {
                p_target_detect_cfg->is_storage_enable = 1;
            }    
            else if(0 == strcmp(NotificationMethod->valuestring, "IO"))
            {
                alarmOutArr = cJSON_GetObjectItem(Notification, "outputIOPortID");
                alarmOutSize = cJSON_GetArraySize(alarmOutArr);
                for(j = 0; j < alarmOutSize; j++)
                {
                    alarmOutObj = cJSON_GetArrayItem(alarmOutArr, j);
                    if(cJSON_IsObject(alarmOutObj))
                    {
                        tmpValueObj = cJSON_GetObjectItem(alarmOutObj, "id");
                        if(cJSON_IsNumber(tmpValueObj))
                        {
                            p_target_detect_cfg->alarmOutInfo[j].id = tmpValueObj->valueint;
                            p_target_detect_cfg->alarmOutInfo[j].enable = true;
                        }
                    }
                }
            }  
            else if(0 == strcmp(NotificationMethod->valuestring, "Record"))
            {
                recordArr = cJSON_GetObjectItem(Notification, "recordChan");
                recordSize = cJSON_GetArraySize(recordArr);
                for(j = 0; j < recordSize; j++)
                {
                    recordObj = cJSON_GetArrayItem(recordArr, j);
                    if(cJSON_IsObject(recordObj))
                    {
                        tmpValueObj = cJSON_GetObjectItem(recordObj, "chan");
                        if(cJSON_IsNumber(tmpValueObj))
                        {
                            p_target_detect_cfg->alarmRecordInfo[j].id = tmpValueObj->valueint;
                            p_target_detect_cfg->alarmRecordInfo[j].enable = true;
                        }
                    }
                }
            }    
            else if(0 == strcmp(NotificationMethod->valuestring, "Capture"))
            {
                capArr = cJSON_GetObjectItem(Notification, "captureChan");
                capSize = cJSON_GetArraySize(capArr);
                for(j = 0; j < capSize; j++)
                {
                    capObj = cJSON_GetArrayItem(capArr, j);
                    if(cJSON_IsObject(capObj))
                    {
                        tmpValueObj = cJSON_GetObjectItem(capObj, "chan");
                        if(cJSON_IsNumber(tmpValueObj))
                        {
                            p_target_detect_cfg->alarmCapInfo[j].id = tmpValueObj->valueint;
                            p_target_detect_cfg->alarmCapInfo[j].enable = true;
                        }
                    }
                }
            }          
        }
    }
    //TODO:暂时写死，待协议商定出标准的再赋值
    p_target_detect_cfg->osd_enable = 0;
    p_target_detect_cfg->streamWithVca = 0;
    p_target_detect_cfg->threshold = 100;

    return PRO_OK;

}


int isapi_target_detect_capabilities(node_t *dstnode, WEB_DES *webinfo, char *remain_path, CGI_PAGE *page)
{
    OP_DEVSDK_REQ_DES *req = NULL;
    int ret = 0;

    req = webinfo->req;


    if (GET == req->method)
    {
        ret = get_target_detect_capa_json(page->json_root);
        if (PRO_OK != ret)
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "get_target_detect_capa_json error!\n");
            return PRO_DEV_ERR;
        }
    }
    else
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "method not allowed !\n");
        return PRO_MATHOD_NOT_ALLOWED;
    }
    return PRO_OK;

}

int isapi_target_detect_ext(node_t *dstnode, WEB_DES *webinfo, char *remain_path, CGI_PAGE *page)
{
    OP_DEVSDK_REQ_DES *req = NULL;

    APP_CFG target_detect_cfg;


    if( NULL == webinfo || NULL == page || NULL == remain_path )
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "isapi_target_detect_ext cfg null\n");
        return PRO_DEV_ERR;
    }

    req = webinfo->req;

    memset(&target_detect_cfg, 0, sizeof(target_detect_cfg));
    
    if ( GET == req->method )
    {  
        
        if(PRO_OK != isapi_get_target_detect_cfg(&target_detect_cfg))
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "isapi_get_target_detect_cfg error\n");
            return PRO_DEV_ERR; 
        }

        return get_target_detect_cfg_json(page->json_root, &target_detect_cfg);
        
    }
    else if ( PUT == req->method )
    {       
        if (PRO_OK != put_target_detect_cfg_json(req->jsonDoc, &target_detect_cfg))
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "put_target_detect_cfg_json error\n");
            return PRO_DEV_ERR;
        }
        
        return isapi_put_target_detect_cfg(&target_detect_cfg);
    }
    else
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "method not allowed\n");
        return PRO_MATHOD_NOT_ALLOWED;
    }
    return PRO_OK;
}





int isapi_target_detect_ext_V2(node_t *dstnode, WEB_DES *webinfo, char *remain_path, CGI_PAGE *page, int chan)
{
    OP_DEVSDK_REQ_DES *req = NULL;

    APP_CFG target_detect_cfg;


    if( NULL == webinfo || NULL == page || NULL == remain_path )
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "isapi_target_detect_ext cfg null\n");
        return PRO_DEV_ERR;
    }

    req = webinfo->req;

    memset(&target_detect_cfg, 0, sizeof(target_detect_cfg));
    
    if ( GET == req->method )
    {  
        
        if(PRO_OK != isapi_get_target_detect_cfg_V2(&target_detect_cfg, chan))
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "isapi_get_target_detect_cfg error\n");
            return PRO_DEV_ERR; 
        }

        return get_target_detect_cfg_json(page->json_root, &target_detect_cfg);
        
    }
    else if ( PUT == req->method )
    {       
        if (PRO_OK != put_target_detect_cfg_json(req->jsonDoc, &target_detect_cfg))
        {
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "put_target_detect_cfg_json error\n");
            return PRO_DEV_ERR;
        }

        return isapi_put_target_detect_cfg_V2(&target_detect_cfg, chan);
    }
    else
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "method not allowed\n");
        return PRO_MATHOD_NOT_ALLOWED;
    }
    return PRO_OK;
}