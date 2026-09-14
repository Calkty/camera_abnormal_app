#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <dirent.h>
#include <arpa/inet.h>
#include "opdevsdk_common_basic.h"
#include "hikflow_demo.h"
#include "cJSON.h"
#include "config.h"
#include "ixml.h"
#include "xmlextend.h"
#include <syslog.h>



#define APP_CFG_FILE  "/heop/package/cameraAbnormal/user_data/app_cfg_file.data"
#define APP_CFG_SAVE_MAGIC (0x4143534d) /*ACSM*/
extern int g_audioplay_index;
extern char *xml_getfirsttagval_byname( IN IXML_Node *parentnode, IN const char *item );
extern int32_t getAppRunningChan(int32_t *chan);
APP_CFG g_app_cfg[MAXCHAN];

#define CJSON_CREATE_OBJECT(node) \
	do{node = cJSON_CreateObject();\
		if(NULL == node)\
		{\
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"\r\nCJSON_CREATE_OBJECT %s	ERROR  [%s,%s,%d] !!!\r\n",#node,__FILE__,__FUNCTION__,__LINE__);\
            return ERROR;\
		}\
		}\
		while(0)
		
#define CJSON_ADD_ITEM_TO_OBJECT(node,name_string,name) \
	do{if(NULL == node)\
		{\
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"\r\nCJSON_ADD_ITEM_TO_OBJECT ERROR node = NULL,[%s,%s,%d] !!!\r\n",__FILE__,__FUNCTION__,__LINE__);\
            return ERROR;\
		}\
		name = cJSON_CreateObject();\
		if(NULL == name)\
		{\
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"\r\nCJSON_ADD_ITEM_TO_OBJECT %s	ERROR  [%s,%s,%d] !!!\r\n",name_string,__FILE__,__FUNCTION__,__LINE__);\
            return ERROR;\
		}\
		cJSON_AddItemToObject(node,name_string,name);\
		}\
		while(0)
	
#define CJSON_CREATE_ARRAY(node) \
	do{node = cJSON_CreateArray();\
		if(NULL == node)\
		{\
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"\r\nCJSON_CREATE_ARRAY %s	ERROR  [%s,%s,%d] !!!\r\n",#node,__FILE__,__FUNCTION__,__LINE__);\
			return ERROR;\
		}\
		}\
		while(0)

#define CJSON_ADD_ITEM_TO_ARRAY(node,name) \
	do{if(NULL == node)\
		{\
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"\r\nCJSON_ADD_ITEM_TO_ARRAY ERROR node = NULL,[%s,%s,%d] !!!\r\n",__FILE__,__FUNCTION__,__LINE__);\
            return ERROR;\
		}\
		name = cJSON_CreateObject();\
		if(NULL == name)\
		{\
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"\r\nCJSON_ADD_ITEM_TO_ARRAY %s	ERROR  [%s,%s,%d] !!!\r\n",#name,__FILE__,__FUNCTION__,__LINE__);\
			return ERROR;\
		}\
		cJSON_AddItemToArray(node,name);\
		}\
		while(0)

#define CJSON_GET_ARRAY_ITEM(web, node,index,name) \
	do{if(NULL == node)\
		{\
            opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "\r\nCJSON_GET_ARRAY_ITEM ERROR node = NULL,[%s,%s,%d] !!!\r\n",__FILE__,__FUNCTION__,__LINE__);\
            return ERROR;\
		}\
		name = cJSON_GetArrayItem(node,index);\
		if(NULL == name)\
		{\
			opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "\r\nCJSON_GET_ARRAY_ITEM %s %d %s ERROR  [%s,%s,%d] !!!\r\n",#name,index,#name,__FILE__,__FUNCTION__,__LINE__);\
			return ERROR;\
		}\
		}\
		while(0)

#define CJSON_GET_OBJECT_ITEM(web, node,name_string,name,json_type,is_need) \
	do{if(NULL == node)\
		{\
			if(is_need)\
			{\
				opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "\r\nCJSON_GET_OBJECT_ITEM %s	ERROR [%s,%s,%d] !!!\r\n",name_string,__FILE__,__FUNCTION__,__LINE__);\
				return COMBSTA_INVAL_JSON_CONTENT;\
			}\
			else\
			{\
	            opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"\r\nCJSON_GET_OBJECT_ITEM ignor '%s',[%s,%s,%d] !!!\r\n",name_string,__FILE__,__FUNCTION__,__LINE__);\
	            break;\
	        }\
		}\
		name  = cJSON_GetObjectItem(node,name_string);\
		if(NULL == name )\
		{\
			if(is_need)\
			{\
				opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "\r\nCJSON_GET_OBJECT_ITEM %s	ERROR [%s,%s,%d] !!!\r\n",name_string,__FILE__,__FUNCTION__,__LINE__);\
				return COMBSTA_INVAL_JSON_FORMAT;\
			}\
			else\
			{\
	            opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"\r\nCJSON_GET_OBJECT_ITEM ignor '%s',[%s,%s,%d] !!!\r\n",name_string,__FILE__,__FUNCTION__,__LINE__);\
	            break;\
			}\
		}\
		int TYPE = json_type;\
		switch(TYPE)\
		{\
			case CJSON_BOOL:\
				if(cJSON_True != name->type && cJSON_False != name->type)\
				{\
					isapi_errMsg_debug(RT_ERROR, web, COMBSTA_INVAL_JSON_FORMAT, "\r\nCJSON_GET_OBJECT_ITEM %s type %s	 ERROR [%s,%s,%d] !!!\r\n",name_string,#json_type,__FILE__,__FUNCTION__,__LINE__);\
					return COMBSTA_INVAL_JSON_FORMAT;\
				}\
				break;\
			case cJSON_Number:\
			case cJSON_Array:\
			case cJSON_Object:\
			case cJSON_Float:\
				if(TYPE != name->type)\
				{\
					isapi_errMsg_debug(RT_ERROR, web, COMBSTA_INVAL_JSON_FORMAT, "\r\nCJSON_GET_OBJECT_ITEM %s type %s	 ERROR [%s,%s,%d] !!!\r\n",name_string,#json_type,__FILE__,__FUNCTION__,__LINE__);\
					return COMBSTA_INVAL_JSON_FORMAT;\
				}\
				break;\
			case cJSON_String:\
				if(TYPE != name->type || NULL == name->valuestring)\
				{\
					isapi_errMsg_debug(RT_ERROR, web, COMBSTA_INVAL_JSON_FORMAT, "\r\nCJSON_GET_OBJECT_ITEM %s type %s	 ERROR [%s,%s,%d] !!!\r\n",name_string,#json_type,__FILE__,__FUNCTION__,__LINE__);\
					return COMBSTA_INVAL_JSON_FORMAT;\
				}\
				break;\
			default:\
				printf("\r\nCJSON_GET_OBJECT_ITEM %s default type %s	 ERROR [%s,%s,%d] !!!\r\n",name_string,#json_type,__FILE__,__FUNCTION__,__LINE__);\
				break;\
		}\
		}\
		while(0)

#define CJSON_GET_ARRAY_SIZE(size,node) \
	do{	*size = 0;\
		if(NULL == node)\
		{\
			printf("\r\nCJSON_GET_OBJECT_ITEM ignor 'node',[%s,%s,%d] !!!\r\n",__FILE__,__FUNCTION__,__LINE__);\
			break;\
		}\
		*size = cJSON_GetArraySize(node);\
		}\
		while(0)

/**@brief	检查文件是否存在
 * @param[in]  char *file_name 数据库名
 * @param[out] 无
 * @return	  OK  	文件存在
 *			  ERROR	文件不存在或者出错
 */
int check_file_exist(const char* file_name)
{
	int ret = -1;
	
	if(file_name != NULL)
	{
		ret = access(file_name, R_OK | W_OK);	
	}

	return (ret >= 0) ? OK : ERROR;
}


/**@brief  保存配置到文件
* @param[in] INT8 *file_name 文件名称
* @param[in] INT32 size  文件大小
* @param[in] INT8 *file_data 文件内容
* @param[out] 
* @return  OK  成功  ERROR 失败
*/
static INT32 write_file(INT8 *file_name, INT32 size, INT8 *file_data)
{
	INT32 file_fd = -1;
	INT32 write_len = 0;
	
	if ((NULL == file_name) || (NULL == file_data) || (size <= 0))
	{
		printf("param in error\n");	
		return ERROR;
	}
	
	file_fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0777);
	if (file_fd < 0)
	{
		printf("open file faile\n");	
		return ERROR;
	}

    struct flock lock;
    lock.l_type = F_WRLCK;  // 写锁
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;         // 锁定整个文件
	if (fcntl(file_fd, F_SETLKW, &lock) == -1) 
	{  
	// F_SETLKW 表示如果锁不可用则等待
		close(file_fd);
		printf("fcntl get write lock fail\n");	
		return ERROR;
	}
	

	write_len = write(file_fd, file_data, size);
	if (write_len < size)
	{
		printf("write_len %d, file_size %d\n", write_len, size);	
		close(file_fd);
		return ERROR;

	}

    // 解锁
    lock.l_type = F_UNLCK;  // 解锁
    if (fcntl(file_fd, F_SETLK, &lock) == -1) 
	{
		printf("fcntl write lock release fail\n");	
        close(file_fd);
        return ERROR;
    }
	
	close(file_fd);
	return OK;
}

/*@brief    读取配置从文件
* @param[in] INT8 *file_name 文件名称
* @param[in] INT32 size  文件大小
* @param[out] INT8 *file_data 文件内容
* @return	byteNum:字节数			
*/
static UINT32 read_file(INT8* file_name, INT32 size, INT8 *file_data)
{
	INT32 file = -1;
	INT32 read_len = 0;

	if (file_name == NULL || size <= 0 || file_data == NULL)
	{
		printf("param in error\n");	
		return ERROR;
	}
	
    file = open(file_name, O_RDONLY);
    if (file == -1)
    {
    	printf("open file fail.\n");
        return ERROR;
    }

	
	read_len = read(file, file_data, size);
	
    close(file);
    return read_len;
}

/*@brief  计算校验和
* @param[in] UINT8 *pbuf 数据内容
* @param[in] INT32 len   数据长度
* @return checksum值			
*/
static UINT32 checksum_8u(UINT8 *pbuf, INT32 len)
{
	UINT8 *pdata_end = pbuf + len;
	UINT32 sum = 0;

	if ((NULL == pbuf) || (len <= 0))
	{
		return sum; 
	}

	while (pbuf < pdata_end)
	{
		sum += (UINT8)*pbuf++;
	}

	return sum;
}

/**	@brief 保存APP配置
 *	@param[in] APP_CFG *p_app_cfg APP配置信息
 *	@param[out] 
 *	@return OK/ERROR
 */
static INT32 save_app_cfg(APP_CFG *p_app_cfg, int chan)
{
	INT32 ret = -1;
    INT8 syscmd[128] = {0};
	APP_CFG_SAVE app_cfg_save;

	if (NULL == p_app_cfg)
	{
		printf("save_app_cfg p_app_cfg is NULL.\n");
		return ERROR;
	}
	
	if(chan < FIRST_CHAN_NO || chan > MAXCHAN)
	{
		printf("save_app_cfg, param is invalid.\n");
		return ERROR;
	}

	memset(&app_cfg_save, 0, sizeof(app_cfg_save));
	ret = read_file(APP_CFG_FILE, sizeof(app_cfg_save), (char *)&app_cfg_save);
	if (ret < 0)
	{
		printf("read_file fail.\n");
		return ERROR;
	}
		
	memcpy(&(app_cfg_save.app_cfg[chan-1]), p_app_cfg, sizeof(APP_CFG));
	app_cfg_save.magic_number = APP_CFG_SAVE_MAGIC;  
	app_cfg_save.check_sum = checksum_8u((UINT8 *)&app_cfg_save.app_cfg[0], MAXCHAN * sizeof(APP_CFG));
	app_cfg_save.length = MAXCHAN * sizeof(APP_CFG);
	app_cfg_save.version = 1;	

	ret = write_file(APP_CFG_FILE, sizeof(APP_CFG_SAVE), (char *)&app_cfg_save);
	if (ret < 0)
	{
		printf("failed to save app cfg to file.\n");	
		return ERROR;
	}

    memset(syscmd, 0, sizeof(syscmd));
	strcpy(syscmd, "sync");
	system(syscmd);

	return OK;
}

static INT32 save_app_cfg_all(APP_CFG *p_app_cfg)
{
	INT32 ret = -1;
    INT8 syscmd[128] = {0};
	APP_CFG_SAVE app_cfg_save;

	if (NULL == p_app_cfg)
	{
		printf("save_app_cfg p_app_cfg is NULL.\n");
		return ERROR;
	}
	

	memcpy(app_cfg_save.app_cfg, p_app_cfg, MAXCHAN * sizeof(APP_CFG));
	app_cfg_save.magic_number = APP_CFG_SAVE_MAGIC;  
	app_cfg_save.check_sum = checksum_8u((UINT8 *)&app_cfg_save.app_cfg[0], MAXCHAN * sizeof(APP_CFG));
	app_cfg_save.length = MAXCHAN * sizeof(APP_CFG);
	app_cfg_save.version = 1;	

	ret = write_file(APP_CFG_FILE, sizeof(APP_CFG_SAVE), (char *)&app_cfg_save);
	if (ret < 0)
	{
		printf("failed to save app cfg to file.\n");	
		return ERROR;
	}

    memset(syscmd, 0, sizeof(syscmd));
	strcpy(syscmd, "sync");
	system(syscmd);

	return OK;
}


/**	@brief 读取APP配置
 *	@param[in] APP_CFG *p_app_cfg APP配置信息
 *	@param[out] 
 *	@return OK/ERROR
 */
static INT32 read_app_cfg(APP_CFG *p_app_cfg, int chan)
{	
	INT32 ret = -1;
	APP_CFG_SAVE app_cfg_save;

	if (NULL == p_app_cfg)
	{
		printf("read_app_cfg p_app_cfg is NULL.\n");
		return ERROR;
	}
	
	memset(&app_cfg_save, 0, sizeof(app_cfg_save));


	ret = read_file(APP_CFG_FILE, sizeof(app_cfg_save), (char *)&app_cfg_save);
	if (ret < 0)
	{
		printf("read_file fail.\n");
		return ERROR;
	}

	if (APP_CFG_SAVE_MAGIC != app_cfg_save.magic_number)
	{
		printf("invalid app_cfg_save magic: 0x%x \n", app_cfg_save.magic_number);
		return ERROR;
	}

	if (checksum_8u((UINT8 *)&(app_cfg_save.app_cfg[0]), MAXCHAN * sizeof(APP_CFG)) != app_cfg_save.check_sum)
	{
		printf("app_cfg_save.check_sum is %d, not match\n", app_cfg_save.check_sum);
		return ERROR;
	}
		
	if (MAXCHAN * sizeof(APP_CFG) != app_cfg_save.length)
	{
		printf("invalid length, length: %d, %zu \n", app_cfg_save.length, sizeof(APP_CFG));
		return ERROR;
	}

	memcpy(p_app_cfg, &(app_cfg_save.app_cfg[chan-1]), sizeof(APP_CFG));
	

    return OK;        
}


static INT32 set_default_cfg(APP_CFG *p_app_cfg)
{	
	int i = 0;

	if (p_app_cfg == NULL)
	{
		printf("set app default cfg, param is invalid.\n");
		return ERROR;
	}

	for(i = 0; i < MAXCHAN; i++)
	{
		p_app_cfg[i].enable = 1;
		p_app_cfg[i].osd_enable = 1;
		p_app_cfg[i].streamWithVca = 1;
		p_app_cfg[i].voiceLinkage = 1;
		p_app_cfg[i].voiceIndex = 1;
		p_app_cfg[i].threshold = 90;
		p_app_cfg[i].region.point_num = 4;
		p_app_cfg[i].region.pos[0].x = 0;
		p_app_cfg[i].region.pos[0].y = 0;
		p_app_cfg[i].region.pos[1].x = 1000;
		p_app_cfg[i].region.pos[1].y = 0;
		p_app_cfg[i].region.pos[2].x = 1000;
		p_app_cfg[i].region.pos[2].y = 1000;
		p_app_cfg[i].region.pos[3].x = 0;
		p_app_cfg[i].region.pos[3].y = 1000;
	}
				
    return OK;
}


INT32 set_app_cfg(APP_CFG *p_app_cfg)
{
    INT32 ret = OK;
	
	if (p_app_cfg == NULL)
	{
		printf("set_app_cfg, param is invalid.\n");
		return ERROR;
	}

    memcpy(&g_app_cfg[0], p_app_cfg, sizeof(APP_CFG));

	/* 配置后处理规则 */
	ret = hikflow_demo_param_set((void*)(&p_app_cfg->region), HIKFLOW_DEMO_PARAM_SET_TYPE_RULE);
	if (HIKFLOW_DEMO_OK != ret)
	{
		printf("demo_smart_hikflow_param_set rule_set failed.\n");
		return ret;
	}
	
	/* 配置规则使能 */
	ret = hikflow_demo_param_set((void*)(&p_app_cfg->enable), HIKFLOW_DEMO_PARAM_SET_TYPE_EN);
	if (HIKFLOW_DEMO_OK != ret)
	{
		printf("demo_smart_hikflow_param_set smart enable failed.\n");
		return ret;
	}

	ret = save_app_cfg(&g_app_cfg[0], FIRST_CHAN_NO);
	if (OK != ret)
	{
		printf("save_app_cfg failed.\n");
		return ret;
	}
    return OK;
}

INT32 set_app_cfg_V2(APP_CFG *p_app_cfg, int chan)
{
    INT32 ret = OK;
	
	if (p_app_cfg == NULL)
	{
		printf("set_app_cfg, param is invalid.\n");
		return ERROR;
	}
	
	if(chan < FIRST_CHAN_NO || chan > MAXCHAN)
	{
		printf("get_app_cfg_V2, param is invalid.\n");
		return ERROR;
	}

	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "set_app_cfg_V2 chan=%d\n", chan);
    memcpy(&g_app_cfg[chan-1], p_app_cfg, sizeof(APP_CFG));

	/* 配置后处理规则 */
	ret = hikflow_demo_param_set((void*)(&p_app_cfg->region), HIKFLOW_DEMO_PARAM_SET_TYPE_RULE);
	if (HIKFLOW_DEMO_OK != ret)
	{
		printf("demo_smart_hikflow_param_set rule_set failed.\n");
		return ret;
	}
	
	/* 配置规则使能 */
	ret = hikflow_demo_param_set((void*)(&p_app_cfg->enable), HIKFLOW_DEMO_PARAM_SET_TYPE_EN);
	if (HIKFLOW_DEMO_OK != ret)
	{
		printf("demo_smart_hikflow_param_set smart enable failed.\n");
		return ret;
	}

	ret = save_app_cfg(&g_app_cfg[chan-1], chan);
	if (OK != ret)
	{
		printf("save_app_cfg failed.\n");
		return ret;
	}

    return OK;
}

INT32 get_app_cfg(APP_CFG *p_app_cfg)
{
    INT32 ret = OK;
	
	if (p_app_cfg == NULL)
	{
		printf("get_app_cfg, param is invalid.\n");
		return ERROR;
	}

    memcpy(p_app_cfg, &g_app_cfg[0], sizeof(APP_CFG));

    return OK;
}

INT32 get_app_cfg_V2(APP_CFG *p_app_cfg, int chan)
{
    INT32 ret = OK;
	
	if(chan < FIRST_CHAN_NO || chan > MAXCHAN)
	{
		printf("get_app_cfg_V2, param is invalid.\n");
		return ERROR;
	}

	if (p_app_cfg == NULL)
	{
		printf("get_app_cfg_V2, param is invalid.\n");
		return ERROR;
	}
	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "get_app_cfg_V2 chan=%d\n", chan);
    memcpy(p_app_cfg, &g_app_cfg[chan -1], sizeof(APP_CFG));

    return OK;
}

INT32 multi_pdc_config_init(VOID)
{
    INT32 ret = OK;
	APP_CFG app_cfg;
	int chan = 0,valid_chan_num = 1;
	int chanInfo[MAXCHAN] = {0};
	int i = 0;
	
	memset(&g_app_cfg, 0, sizeof(g_app_cfg));
	memset(&app_cfg, 0, sizeof(app_cfg));

	ret = set_default_cfg(g_app_cfg);
	if (OK != ret)
	{
		printf("set_default_cfg failed.\n");
		return ret;
	}
	
	ret = get_app_chan(HUMAN_APP_ID, chanInfo, &valid_chan_num, NULL);
	if(HIKFLOW_DEMO_OK!=ret || valid_chan_num>MAXCHAN || valid_chan_num<1 || chanInfo[0]<1)
	{
	    opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"get_app_chan err ret %d, valid_chan_num %d, chanInfo[0] %d\n",ret, valid_chan_num, chanInfo[0]);
    	chan = 1;
	}
	else
	{
		chan = chanInfo[0];
	}

	/*首次导入没有文件数据库*/
	ret = check_file_exist(APP_CFG_FILE);
	if(OK != ret)
	{
		printf("check_file_exist not exist, create db doc.\n");
		ret = save_app_cfg_all(g_app_cfg);
		if (OK != ret)
		{
			printf("save_app_cfg failed.\n");
			return ret;
		}				
	}
	else
	{
		printf("file already exist.\n");
	}

	ret = read_app_cfg(&app_cfg, chan);
	if (OK != ret)
	{
		printf("read_app_cfg failed.\n");
		return ret;
	}

	memcpy(&g_app_cfg[chan-1], &app_cfg, sizeof(APP_CFG));
	

	if (0 != g_audioplay_index)
	{
		g_app_cfg[chan-1].voiceIndex = g_audioplay_index;
	}
	else
	{
		g_audioplay_index = g_app_cfg[chan-1].voiceIndex;
	}

	ret = set_app_cfg_V2(&g_app_cfg[chan-1], chan);
	if (OK != ret)
	{
		printf("set_app_cfg_V2 failed.\n");
		return ret;
	}
			
	return OK;
}

static int32_t GetMethodIsapiRetContent(char* url, OP_DEVSDK_MIME_UNIT_ST *in_buf, char* buf, unsigned int bufLen)
{
#define MAX_REQ_URL_LEN 2048
	unsigned int ret = 0;
	OP_DEVSDK_DATATRANS_INPUT_ST stIntput = {0};
	OP_DEVSDK_DATATRANS_OUTPUT_ST stOutput = {0};
	POP_DEVSDK_MIME_UNIT_ST out_data = NULL;
	POP_DEVSDK_MIME_UNIT_ST p_mime_st = NULL;

	if(NULL == url ||
		strlen(url) > MAX_REQ_URL_LEN ||
		NULL == buf ||
		0 == bufLen)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "[GetMethodIsapiRetContent] input param err!\n");
		return -1;
	}
	
	stIntput.p_request_url = url ;
	stIntput.request_url_len = strlen(url);
	stIntput.p_in_buffer = (char *)in_buf;
	stIntput.in_buffer_num = 1;
	stIntput.recv_time_out = 0;	

	out_data = (POP_DEVSDK_MIME_UNIT_ST)malloc(1 * sizeof(OP_DEVSDK_MIME_UNIT_ST));
	if(NULL == out_data)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR ,"[GetMethodIsapiRetContent] malloc fail!\n");
		ret = -1;
		goto EXIT1;
	}
	memset(out_data, 0 , sizeof(OP_DEVSDK_MIME_UNIT_ST));
	//out_buffer_num must >0  
	stOutput.out_buffer_num = 1;
	stOutput.p_out_buffer = (char*)out_data;
	out_data->p_content = buf;
	out_data->content_len = bufLen;

	if(0 != opdevsdk_dataTrans_sendDataShort(&stIntput,&stOutput))
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"[GetMethodIsapiRetContent] opdevsdk_dataTrans_sendDataShort exec fail!\n");
		ret = -1;
		goto EXIT2;
	}
 
 	opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "out_data->returned_size:%d\n", out_data->returned_size);
	opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "out_data->content_len:%d\n", out_data->content_len);
	opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "stOutput.returned_buffer_num:%d\n", stOutput.returned_buffer_num);
    opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "out_data->p_content:\n%s\n", out_data->p_content);

	p_mime_st = (POP_DEVSDK_MIME_UNIT_ST)(stOutput.p_out_buffer);

	if(NULL != p_mime_st && 
		NULL != p_mime_st->p_content &&
		bufLen > p_mime_st->returned_size)
	{
		memcpy(buf, p_mime_st->p_content, p_mime_st->returned_size);
	}
	else
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR ,"[GetMethodIsapiRetContent] p_mime_st param err!\n");
		ret = -1;
	}
EXIT2:
	free(out_data);
EXIT1:
	return ret;	
}


INT32 get_app_chan(char *appID, int32_t chanInfo[], int32_t *validChanNum, void *data)
{
#define FORM_TYPE_JSON  1	
#define HEOP_APP_GET_BODY "{\
	\"appID\":	\"%s\",\
	\"engineID\":	\"%s\"\
	}"

	int32_t ret = -1, i = 0;
	char buf_json[1024];
	char recv_buf[1024*4];
	int32_t in_buf_len = 0;
	OP_DEVSDK_MIME_UNIT_ST in_data;
	cJSON *recvRoot = NULL, *chanArray = NULL, *chanNum = NULL;
	const char *engineID = NULL;
	int32_t chan = -1;

	/* 2. register a listen port */
	memset(&in_data, 0, sizeof(in_data));
	memset(&buf_json, 0, sizeof(buf_json));
	memset(recv_buf, 0, sizeof(recv_buf));

	if(OK == getAppRunningChan(&chan))
	{
		chanInfo[0] = chan;
		return 0;
	}


	engineID = getenv("ENGINE_ID");
    if(NULL == engineID)
    {
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"getenv engineID failed.\n");
		in_buf_len += snprintf(buf_json, sizeof(buf_json), HEOP_APP_GET_BODY, appID, "1");
    }
	else
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"getenv engineID succ:%s.\n", engineID);
		in_buf_len += snprintf(buf_json, sizeof(buf_json), HEOP_APP_GET_BODY, appID, engineID);
	}
	
	in_data.data_type = FORM_TYPE_JSON;
	in_data.p_content = buf_json;
	in_data.content_len = in_buf_len;
	
	/*Send a protocol below:*/
	ret = GetMethodIsapiRetContent("POST /ISAPI/Custom/OpenPlatform/searchAppChannels?format=json", &in_data, recv_buf, sizeof(recv_buf));
	if(0 != ret)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"[GetMethodIsapiRetContent] executed failed!\n");
		return -1;
	}

	
	opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "[HTTPServer] get AppChannel info: %s\n", recv_buf);

	recvRoot = cJSON_Parse(recv_buf);
	if(!cJSON_IsObject(recvRoot))
	{
		opdevsdk_write_log(OPDEVSDK_LOG_DEBUG,"[recv_buf] NULL[%d]\n",__LINE__);
		return -1;
	}

	chanArray = cJSON_GetObjectItem(recvRoot, "channelList");
	if(!cJSON_IsArray(chanArray))
	{
		opdevsdk_write_log(OPDEVSDK_LOG_DEBUG,"[chanItem] NULL[%d]\n",__LINE__);
		cJSON_Delete(recvRoot);
		return -1;
	}	

	
	for(i = 0; i < MIN(cJSON_GetArraySize(chanArray), *validChanNum);i++)
	{
		chanNum = cJSON_GetArrayItem(chanArray, i);
		if(NULL != chanNum)
		{
			chanInfo[i] = chanNum->valueint;
		}
	}
	*validChanNum = cJSON_GetArraySize(chanArray);	
	cJSON_Delete(recvRoot);

	return 0;
}


int32_t add_link_json_object_by_config(cJSON *js_linkageInfo)
{
	int32_t chan = 0;
	APP_CFG app_cfg;
	cJSON *js_optarray_1 = NULL;
	cJSON *js_alarmOut = NULL;
	cJSON *js_storage = NULL;
	cJSON *js_optarray_2 = NULL;
	cJSON *js_capture = NULL;
	cJSON *js_optarray_3 = NULL;
	cJSON *js_record = NULL;
	cJSON *js_optarray_4 = NULL;
	cJSON *js_voice = NULL;
	cJSON *js_protocolType = NULL;
	INT32 i = 0;
	int ret = ERROR;
    int chanInfo[MAXCHAN] = {0};
    int valid_chan_num = 1;;

	memset(&app_cfg, 0, sizeof(app_cfg));

	if(NULL == js_linkageInfo)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_DEBUG,"replace_json_object_by_config NULL param[%d]\n",__LINE__);
		return ERROR;
	}

	ret = get_app_chan(HUMAN_APP_ID, chanInfo, &valid_chan_num, NULL);
	if(OK!=ret || valid_chan_num>MAXCHAN || valid_chan_num<1 || chanInfo[0]<1)
	{
	    opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "get_app_chan err ret %d, valid_chan_num %d, chanInfo[0] %d\n",ret, valid_chan_num, chanInfo[0]);
    	chan = 1;
	}
	else
	{
		chan = chanInfo[0];
	}
	

	ret= get_app_cfg_V2(&app_cfg, chan);
    if (0 != ret)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "replace_json_object_by_config get_app_cfg_V2 error\n");
        return ERROR;
    }


	CJSON_CREATE_ARRAY(js_optarray_1);
	cJSON_AddItemToObject(js_linkageInfo, "alarmOut", js_optarray_1);
	for(i = 0; i < MAX_ALARMOUT_NUMS; i++)
	{
		if(app_cfg.alarmOutInfo[i].enable)
		{
			CJSON_ADD_ITEM_TO_ARRAY(js_optarray_1, js_alarmOut);
			cJSON_AddNumberToObject(js_alarmOut, "id", app_cfg.alarmOutInfo[i].id);
			cJSON_AddNumberToObject(js_alarmOut, "duration", 5);
		}
	}
	
	if(app_cfg.is_storage_enable)
	{
		CJSON_ADD_ITEM_TO_OBJECT(js_linkageInfo, "storage", js_storage);
		cJSON_AddBoolToObject(js_storage, "storageEnabled", true);
	}

	CJSON_CREATE_ARRAY(js_optarray_2);
	cJSON_AddItemToObject(js_linkageInfo, "capture", js_optarray_2);
	for(i = 0; i < MAX_CAPTURE_NUMS; i++)
	{
		if(app_cfg.alarmCapInfo[i].enable)
		{
			CJSON_ADD_ITEM_TO_ARRAY(js_optarray_2, js_capture);
			cJSON_AddNumberToObject(js_capture, "targetChan", app_cfg.alarmCapInfo[i].id);
		}
	}
	CJSON_CREATE_ARRAY(js_optarray_3);
	cJSON_AddItemToObject(js_linkageInfo, "record", js_optarray_3);
	for(i = 0; i < MAX_RECORD_NUMS; i++)
	{
		if(app_cfg.alarmRecordInfo[i].enable)
		{
			CJSON_ADD_ITEM_TO_ARRAY(js_optarray_3, js_record);
			cJSON_AddNumberToObject(js_record, "targetChan", app_cfg.alarmRecordInfo[i].id);
			cJSON_AddNumberToObject(js_record, "duration", 5);
		}
	}



    if (app_cfg.voiceLinkage)
    {
		CJSON_CREATE_ARRAY(js_optarray_4);
        cJSON_AddItemToObject(js_linkageInfo, "voice", js_optarray_4);			
		/*!< this is the number of alarm voices */
        for (int i = 0; i < 1; i++)
        {
			js_voice = cJSON_CreateObject();
            cJSON_AddNumberToObject(js_voice, "id", 13);
            cJSON_AddNumberToObject(js_voice, "duration", 5);
            cJSON_AddItemToArray(js_optarray_4, js_voice);
        }
    }

	
	if(app_cfg.is_center_enable)
	{
		CJSON_ADD_ITEM_TO_OBJECT(js_linkageInfo, "protocolType", js_protocolType);
		cJSON_AddBoolToObject(js_protocolType, "SDKEnabled", true);
		cJSON_AddBoolToObject(js_protocolType, "ISAPIEnabled", true);
		cJSON_AddBoolToObject(js_protocolType, "ISUPEnabled", true);
		cJSON_AddBoolToObject(js_protocolType, "EzvizEnabled", true);
		cJSON_AddBoolToObject(js_protocolType, "ONVIFEnabled", true);
		cJSON_AddBoolToObject(js_protocolType, "1400Enabled", true);
		cJSON_AddBoolToObject(js_protocolType, "OTAPEnabled", true);
	}

	return OK;

}


