/****************************************************************************** 
 *  @file audioplay.c
 *  @note HangZhou Hikvision Digital Technology Co., Ltd. All Rights Reserved.
 *  @brief 
 *
 *  @author   zhouwenjie7@hikvision.com.cn
 *  @date     2022-1-12
 *
 *  @note History:
 *  @note
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include "opdevsdk_common_basic.h"
#include "cJSON.h"
#include "audioplay.h"

#define UPLOAD_HEAD "\
POST /ISAPI/Event/triggers/notifications/AudioAlarm/customAudio?format=json HTTP/1.1\r\n\
Authorization: Basic YWRtaW46YWJjZDEyMzQ=\r\n\
User-Agent: NS-HTTP/1.0\r\n\
Host: %s\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\
Content-Type: multipart/form-data; boundary=---------------------------7e52883470056\r\n\r\n\
"

#define MIME_PART "\
-----------------------------7e52883470056\r\n\
Content-Disposition: form-data; name=\"CustomAudioInfo\"\r\n\r\n\
{\"CustomAudioInfo\":{\"customAudioName\":\"test\"}}\r\n\
-----------------------------7e52883470056\r\n\
Content-Disposition: form-data; name=\"file\"; filename=\"test.wav\"\r\n\
Content-Type: audio/wav\r\n\r\n\
"

#define AUDIO_TEST_MSG "\
PUT /ISAPI/Event/triggers/notifications/AudioAlarm/%d/test?format=json HTTP/1.1\r\n\
Authorization: Basic YWRtaW46YWJjZDEyMzQ=\r\n\
User-Agent: NS-HTTP/1.0\r\n\
Connection: keep-alive\r\n\
Content-Length: 0\r\n\r\n\
"

#define BODY_SIZE   (1024*1024*2)
#define BASE_IDX    12
#define IP_STR_LEN  64

#define AUDIOPLAY_SERVER_PORT  80
char g_audioplay_server_ip_str[IP_STR_LEN];
int g_audioplay_index;

static void inet_ntop4(const unsigned char* p_src, char* p_dst, int isize)
{
	static const char sz_fmt[] = "%u.%u.%u.%u";
	char sz_tmp[sizeof "255.255.255.255"];

	if ((NULL == p_src) || (NULL == p_dst) || (isize <= 0))
	{
		return;
	}

	sprintf(sz_tmp, sz_fmt, p_src[0], p_src[1], p_src[2], p_src[3]);

	if ((int)strlen(sz_tmp) > isize-1)
	{
		return;
	}

	strcpy(p_dst, sz_tmp);

	return;
}

/* @fn static connect_to_web_server(char *p_ip)
 * @brief 连接设备主机接口
 * @param[in] char *p_ip 设备主机IP
 * @return 0 正常/-1 异常
 */
static int connect_to_web_server(char *p_ip)
{
    struct sockaddr_in tcp_server_addr;
    int socket_fd = -1;
    OP_DEVSDK_NET_CFG net_cfg;	
	int ret = -1;
	
    memset(&net_cfg, 0, sizeof(net_cfg));
	
    
    ret = opdevsdk_get_network_cfg(&net_cfg);
	if (0 != ret)
	{
	    opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "opdevsdk_get_network_cfg failed\n");
        return -1;
	}   
	
    memset(&tcp_server_addr, 0, sizeof(struct sockaddr_in)); 
    
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_addr.s_addr = inet_addr(p_ip);
    tcp_server_addr.sin_port = htons(net_cfg.net_port.http_port);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "socket error\n");
        return -1;
    }
    
    if (connect(socket_fd, (struct sockaddr*)&tcp_server_addr,sizeof(tcp_server_addr)) < 0)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "connect Failed!!!!\n");
        return -1;
    }
	
    return socket_fd;
}

/* @fn static int parse_resp_msg(char resp_msg[], int* p_index)
 * @brief 语音上传应答报文解析接口
 * @param[in] char resp_msg[] 应答报文
 * @param[out] int* p_index 自定义语音index
 * @return 0 正常/-1 异常
 */
static int parse_resp_msg(char resp_msg[], int* p_index)
{
    cJSON *p_root = NULL;
    cJSON *js_custom_id = NULL;
    char *resp_body = NULL;
    char *p_param_json = NULL;

    if (NULL == resp_msg)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "NULL pointer\n");
        return -1;
    }

    resp_body = strstr(resp_msg, "\r\n\r\n");
    resp_body +=  strlen("\r\n\r\n");

    p_root = cJSON_Parse(resp_body);
    if (p_root == NULL)
    {
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "p_root null\n");
        return -1;
    }

    js_custom_id = cJSON_GetObjectItem(p_root, "customAudioID");
    if (js_custom_id == NULL)
    {
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "customAudioID null\n");
        return -1;
    }

    *p_index = js_custom_id->valueint;
    
    return 0;
}

/* @fn static int audio_upload(char* ip_str, int* p_index)
 * @brief 自定义语音上传接口
 * @param[in] char* ip_str 语音播报服务IP
 * @param[out] int* p_index 协议返回的自定义语音index
 * @return 0 正常/-1 异常
 */
static int audio_upload(char* ip_str, int* p_index)
{
    int sock_fd = -1;
    char *mime_body = NULL;
    long fsize = 0;
    int temp_len = 0;
    int ret = 0;
    char http_head[1024];
    char receive_buf[1024*4];

    memset(http_head, 0, sizeof(http_head));
    memset(receive_buf, 0, sizeof(receive_buf));

    if (NULL == ip_str)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "NULL pointer\n");
        return -1;
    }

    sock_fd = connect_to_web_server(ip_str);
	if (sock_fd < 0)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "connect web server failed!!!\n");
        return -1;
	}

    /************************construct mime part********************************/
    mime_body = malloc(BODY_SIZE);
	if (NULL == mime_body)
	{
    	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "malloc mime_body failed!!!\n");
        return -1;
	}
    memset(mime_body, 0, BODY_SIZE);

    FILE *f = fopen("/heop/package/cameraAbnormal/test.wav", "rb");
    if (f == NULL)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "open file error!!!\n");
        free(mime_body);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET); //same as rewind(f);

    temp_len += snprintf(mime_body, BODY_SIZE, MIME_PART);

    if (temp_len + fsize > BODY_SIZE)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "file too large\n");
        free(mime_body);
        close(sock_fd);
        fclose(f);
        return -1;
    }

    fread(mime_body + temp_len, fsize, 1, f);
    temp_len += fsize;
    temp_len += snprintf(mime_body + temp_len, BODY_SIZE - temp_len, "\r\n\r\n-----------------------------7e52883470056--\r\n\r\n");

    /************************construct head********************************/
    snprintf(http_head, 1024, UPLOAD_HEAD, ip_str, temp_len);

    /************************send http head********************************/
    ret = write(sock_fd, http_head, strlen(http_head));
    if (ret < 0)
    {
    	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "write http head err! ret=%d\n", ret);
    	close(sock_fd);
        free(mime_body);
        fclose(f);
        return -1;
    }

    /************************send http body********************************/
    ret = write(sock_fd, mime_body, temp_len);
    if (ret < 0)
    {
    	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "write http body! ret=%d\n", ret);
    	close(sock_fd);
        free(mime_body);
        fclose(f);
        return -1;
    }

    ret = read(sock_fd, receive_buf, sizeof(receive_buf));
 	if (ret <= 0)
 	{
 		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "read http response body error\n");
        fclose(f);
        close(sock_fd);
        free(mime_body);
        return -1;
 	}

    if (strstr(receive_buf, "403"))
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "audioplay not supporot\n");
        fclose(f);
        close(sock_fd);
        free(mime_body);
        return -1;
    }

    if (0 != parse_resp_msg(receive_buf, p_index))
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "parse_resp error\n");
        fclose(f);
        close(sock_fd);
        free(mime_body);
        return -1;
    }

    fclose(f);
    close(sock_fd);
    free(mime_body);

    return 0;
}

/* @fn int audio_play(void)
 * @brief 自定义语音播报接口
 * @param[in] 无
 * @param[out] 无
 * @return 0 正常/-1 异常
 */
int audio_play(void)
{
    char http_msg[1024*4];
    char recv_buf[1024*4];

    int ret = 0;
    int sock_fd = -1;

    memset(http_msg, 0, sizeof(http_msg));
    memset(recv_buf, 0, sizeof(recv_buf));

    sock_fd = connect_to_web_server(g_audioplay_server_ip_str);
	if (sock_fd < 0)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "connect web server failed!!!\n");
        return -1;
	}

    snprintf(http_msg, 4 * 1024, AUDIO_TEST_MSG, g_audioplay_index + BASE_IDX);
    opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "http_msg :%s\n", http_msg);

    //send http body
    ret = write(sock_fd, http_msg, strlen(http_msg));
    if (ret < 0)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "write http body error! ret === %d\n", ret);
    	close(sock_fd);
        return -1;
    }

    ret = read(sock_fd, recv_buf, sizeof(recv_buf));
    if (ret <= 0)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "read http body error! ret === %d\n", ret);
    }

	opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "PUT /ISAPI/Event/triggers/notifications/AudioAlarm/%d/test?format=json HTTP/1.1:\n%s\n", index + BASE_IDX, recv_buf);
    if (strstr(recv_buf, "403"))
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "audioplay not supporot\n");
        close(sock_fd);
        return -1;
    }

    close(sock_fd);

    return 0;
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


static int32_t SetMechineVoiceParam(void)
{
	int32_t ret = -1;
	char buf_json[1024];
	char recv_buf[1024*4];
	int32_t in_buf_len = 0;
    OP_DEVSDK_MIME_UNIT_ST in_data;
#define FORM_TYPE_JSON  1	
#define HEOP_VOICE_BODY "\
<TwoWayAudioChannel>\
  <id>1</id>\
  <enabled>true</enabled>\
  <audioCompressionType>G.711alaw</audioCompressionType>\
  <audioInputType>LineIn</audioInputType>\
  <speakerVolume>50</speakerVolume>\
  <microphoneVolume>100</microphoneVolume>\
  <noisereduce>false</noisereduce>\
</TwoWayAudioChannel>\
"
	/* 2. register a listen port */
	memset(&in_data, 0, sizeof(in_data));
	memset(&buf_json, 0, sizeof(buf_json));
	memset(recv_buf, 0, sizeof(recv_buf));

	in_buf_len += snprintf(buf_json, sizeof(buf_json), HEOP_VOICE_BODY);
    in_data.data_type = FORM_TYPE_JSON;
    in_data.p_content = buf_json;
    in_data.content_len = in_buf_len;
	
	/*Send a protocol below:*/
	ret = GetMethodIsapiRetContent("PUT /ISAPI/System/TwoWayAudio/channels/%d", &in_data, recv_buf, sizeof(recv_buf));
	if(0 != ret)
	{
		opdevsdk_write_log(OPDEVSDK_LOG_ERROR,"[GetMethodIsapiRetContent] executed failed!\n");
		return -1;
	}

	
	opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "[HTTPServer] get TwoWayAudio info: %s\n", recv_buf);

	return 0;
}

/* @fn int audio_play_init(void)
 * @brief 语音播报初始化
 * @param[in]
 * @param[in]
 * @return 0 正常/-1 异常
 */
int audio_play_init(void)
{
    int ret = 0;
	int index = 0;
	char ip_str[IP_STR_LEN];
    OP_DEVSDK_NET_CFG net_cfg;	

	memset(ip_str, 0, sizeof(ip_str));
    memset(&net_cfg, 0, sizeof(net_cfg));
	
	/*设置音频参数*/
	ret = SetMechineVoiceParam();
	if (0 != ret)
	{
		printf("Set_Mechine_Image_Param failed.\n");
		return ret;
	}
	    
    ret = opdevsdk_get_network_cfg(&net_cfg);
	if (0 != ret)
	{
	    opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "opdevsdk_get_network_cfg failed\n");
        return -1;
	}
	
    inet_ntop4((const unsigned char *)&net_cfg.net_addr.host_local_ip.v4, ip_str, IP_STR_LEN);
	memcpy(g_audioplay_server_ip_str, ip_str, IP_STR_LEN);
    opdevsdk_write_log(OPDEVSDK_LOG_DEBUG, "ipV4=%d, ip_str=%s\n", net_cfg.net_addr.host_local_ip.v4, ip_str);
    if (-1 == audio_upload(ip_str, &index))
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "audio_upload error\n");
		
        return -1;
    }

	g_audioplay_index = index;

    return 0;
}

