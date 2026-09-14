#include "string.h"
#include <protocol_target_detection.h>
#include <config.h>
#include <cJSON.h>
#include <protocol_status.h>
#include "stdlib.h"
#include <syslog.h>


#ifndef SAFE_FREE
#define SAFE_FREE(x) do { if ((x) != NULL) { free((x)); (x) = NULL;} } while(0) ///< 安全释放动态分配的内存
#endif
#ifndef SAFE_FREE_JSON
#define SAFE_FREE_JSON(x) do { if ((x) != NULL) { cJSON_Delete((x)); (x) = NULL;} } while(0)
#endif

#define SECURITY_INFO_MAX_LEN	256			// 敏感信息最长长度，需要定成2的整数倍

op_devsdk_errno_code isapi_entry_callback(OP_DEVSDK_REQ_DES *p_req_des, OP_DEVSDK_RESP_DES *p_resp_des);


static opdevsdk_protoexten_callback g_opdevsdk_protoexten_context[] =
{
	{"cameraAbnormal/capabilities", 0, isapi_entry_callback},
    {"cameraAbnormal/config", 0, isapi_entry_callback},

	{"", 0, NULL},
};

INT32 json_resp_des(INT32 comb_status, WEB_DES*webinfo, CGI_PAGE* page)
{	
	const char *pstatus = NULL;
	int i = 0;
	INT32 size = 0;
	cJSON * root = NULL;
	INT32 status = 403;
	INT8 transformURLBuf[2 * SECURITY_INFO_MAX_LEN];
	char* p_json_resp_body = NULL;
	

	if(NULL == webinfo || NULL == page)
	{
		return ERROR;
	}

	memset(transformURLBuf, 0, sizeof(transformURLBuf));


	if(0 ==strlen(webinfo->resp->content_type))
	{
		strncpy(webinfo->resp->content_type, "application/json", sizeof(webinfo->resp->content_type)-1);

	}

	if(GET == webinfo->req->method  && PRO_OK == comb_status)
	{
		if(page != NULL && page->json_root != NULL)
		{
			if(page->json_root->string != NULL && 0 != strlen(page->json_root->string))
			{
				root = cJSON_CreateObject();
				cJSON_AddItemToObject(root, page->json_root->string, page->json_root);
				p_json_resp_body = cJSON_Print(root);
				if(NULL != p_json_resp_body)
				{
                    memcpy(webinfo->resp->httpbody, p_json_resp_body, strlen(p_json_resp_body));
					SAFE_FREE(p_json_resp_body);
				}

				SAFE_FREE_JSON(root);
			}
			else
			{
				p_json_resp_body = cJSON_Print(page->json_root);
				if(NULL != p_json_resp_body)
				{
                    memcpy(webinfo->resp->httpbody, p_json_resp_body, strlen(p_json_resp_body));
					SAFE_FREE(p_json_resp_body);
				}

			}
		}
		
		if( webinfo->resp->content_len <= 0 && NULL != webinfo->resp->httpbody)
		{
			//如果长度值没有设定，用响应内容的长度
			webinfo->resp->content_len = strlen(webinfo->resp->httpbody);
		}
        webinfo->resp->httpcode = 200;
	}
	else
	{
		root = cJSON_CreateObject();

		switch(comb_status)
		{
			case PRO_OK:
                status = 200;
				cJSON_AddNumberToObject(root,"statusCode",1); 
				cJSON_AddStringToObject(root,"statusString","OK");
                cJSON_AddStringToObject(root,"subStatusCode","ok"); 
				break;
			case PRO_DEV_ERR:
                status = 500;
				cJSON_AddNumberToObject(root,"statusCode",3); 
				cJSON_AddStringToObject(root,"statusString","Device Error");
                cJSON_AddStringToObject(root,"subStatusCode","device error"); 
				break;
			case PRO_MATHOD_NOT_ALLOWED:
                status = 405;
				cJSON_AddNumberToObject(root,"statusCode",7); 
				cJSON_AddStringToObject(root,"statusString","mathod not allowed"); 
                cJSON_AddStringToObject(root,"subStatusCode","mathod not support"); 
				break;
            case PRO_INVAILD_OPER:
                status = 400;
				cJSON_AddNumberToObject(root,"statusCode",4); 
				cJSON_AddStringToObject(root,"statusString","Invalid Operation"); 
                cJSON_AddStringToObject(root,"subStatusCode","Invalid Operation"); 
			default:
                opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "webs_response_status_json:error mainno:%d\n",comb_status);
				return ERROR;
		}
		
		
        webinfo->resp->httpcode = status;
		
		p_json_resp_body = cJSON_Print(root);
		if(NULL != p_json_resp_body)
		{
            memcpy(webinfo->resp->httpbody, p_json_resp_body,  strlen(p_json_resp_body));
			SAFE_FREE(p_json_resp_body);
		}

		SAFE_FREE_JSON(root);

		webinfo->resp->content_len = strlen(webinfo->resp->httpbody);

	}

	return OK;	 
}


op_devsdk_errno_code isapi_entry_callback(OP_DEVSDK_REQ_DES *p_req_des, OP_DEVSDK_RESP_DES *p_resp_des)
{
	INT32 op_ret_val = op_devsdk_ok;
	CGI_PAGE page;
	char *tmppath = NULL;
	int len = 0;
	
	node_t *pp = NULL;
	node_t **getnode = &pp;
	char querytmpPath[64] = {0};
	char *chanStr = NULL;
	

	int  statuscode = PRO_OK;
	WEB_DES web;
	int chan = FIRST_CHAN_NO;
	

	
	if (NULL == p_req_des || NULL == p_resp_des)
	{
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "param NULL\n");
		return op_devsdk_err_invalid_param;
	}

	memset(&page, 0, sizeof(page));

	memset(&web, 0, sizeof(web));
	//memset(page_buf, 0, sizeof(page_buf));

	
	web.req = p_req_des;
	web.resp = p_resp_des;

	//web.resp->id = -1;
	web.resp->httpcode = 404;
	web.resp->detail_statcode = -1;
	
	memset(web.resp->detail_statstr, 0, 128);

    page.json_root = cJSON_CreateObject();
    
    if((NULL != p_req_des->httpbody) && (p_req_des->content_len != 0))
    {
        p_req_des->jsonDoc = cJSON_Parse(p_req_des->httpbody);
        if(NULL == p_req_des->jsonDoc)
        {
            /* 因为支持json和xml后，防止json和xml混乱发送导致的设备崩溃问题，我们在带有body的情况下直接解析，
                如果解析xml/json格式失败，直接返回失败，不再进入协议内部处理了 */
                
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "req->jsonDoc is NULL\n");
            
            if(ERROR == json_resp_des(PRO_INVAILD_OPER, &web, &page))
            {
                opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "webs_response_status_with_type error\n");
            }
    
            return op_ret_val;
        }
    }

	if(p_req_des->url == NULL)
	{
		return op_devsdk_err_invalid_param;
	}

	tmppath = p_req_des->url;
	len = strlen(tmppath);
	if(tmppath[len -1]=='/')
	{
		tmppath[len-1] = 0;
	}
	
	if(strncmp(tmppath, "/ISAPI", 6) == 0)
	{
		tmppath +=6;
	}
    opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "tmppath:%s\n",tmppath);

	memcpy(querytmpPath, p_req_des->uri_query, MIN(sizeof(querytmpPath), strlen(p_req_des->uri_query)));
	/* url匹配 */
	if((NULL != strstr(tmppath, "/Custom/OpenPlatform/extern/cameraAbnormal/")))
	{
        tmppath += strlen("/Custom/OpenPlatform/extern/cameraAbnormal/");
        opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "remainpath = %s, query_path=%s\n",tmppath, querytmpPath);
        if (0 == strcmp(tmppath, "capabilities"))
        {
			/*暂时不用chan 所有通道都是写死的能力 等新增需求提上来后需要*/
            statuscode = isapi_target_detect_capabilities(*getnode, &web, tmppath, &page);	
        }
        else if (0 == strcmp(tmppath, "config"))
        {
			chanStr = strstr(querytmpPath, "&chanID=");
			if(chanStr != NULL) 
			{
				chanStr = chanStr + strlen("&chanID=");
				chan = atoi(chanStr);
				opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "config chanid:%d\n",chan);
				statuscode = isapi_target_detect_ext_V2(*getnode, &web, tmppath, &page, chan);
			}
			else
			{
            	statuscode = isapi_target_detect_ext(*getnode, &web, tmppath, &page);
			}
        }
	}
	
	else
	{
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "==system_process is not found [%s]...\n",web.req->url);
		statuscode = PRO_INVAILD_OPER;
	}
	
	if(0 != statuscode)
	{
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "statuscode : COMBSTA_BUF_NOT_ENOUGH, [%s]\n",web.req->url);

		// json:释放内存
		if(p_req_des->b_json)
		{
			SAFE_FREE_JSON(p_req_des->jsonDoc);
			SAFE_FREE_JSON(page.json_root);
		
		}
			
		return op_devsdk_err_not_support;
	}


	
	if(ERROR == json_resp_des(statuscode, &web, &page))
	{
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "webs_response_status_with_type error\n");
	}
	

	// json:释放内存
	if(p_req_des->b_json)
	{
		
		SAFE_FREE_JSON(p_req_des->jsonDoc);
		SAFE_FREE_JSON(page.json_root);
		
	}
    opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "isapi_entry_callback, %s succ!\n", web.req->url);


	return op_ret_val;
	
}


INT32 target_detect_opdevsdk_init(char *app_name_chan)
{
	UINT8  op_callback_num = 0;
	UINT32  opdevsdk_ret = op_devsdk_ok;

	if(NULL == app_name_chan)
	{
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "opdevsdk_init failed, target_detect_opdevsdk_init NULL param\n");
		return -1 ;
	}

	opdevsdk_ret = opdevsdk_init(app_name_chan, OPDEVSDK_MODE_PROTO_EXTEN);
	if( op_devsdk_ok != opdevsdk_ret)
	{
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "[%s] opdevsdk_init failed, ret；[%d]\n","humanDetect", opdevsdk_ret);
		return -1 ;
	}
	op_callback_num = ( sizeof(g_opdevsdk_protoexten_context) / sizeof(opdevsdk_protoexten_callback) ) - 1;
	opdevsdk_ret = opdevsdk_protoexten_set_callback((opdevsdk_protoexten_callback*)g_opdevsdk_protoexten_context, op_callback_num);
	if(op_devsdk_ok != opdevsdk_ret)
	{
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "[%s] opdevsdk_protoexten_set_callback failed, ret: [%d]\n","humanDetect", opdevsdk_ret);
		return -1;
	}

	


    opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "student_stoodup_opdevsdk_init ok\n");
	return 0;

}
    
